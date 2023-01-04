// Copyright (C) 2018-2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "ffmpeg_compat.h"
#include "CFFEncoder.h"
#include "ProfileLevel.h"
#ifdef ENABLE_MEMSHARE
extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}
#endif

using namespace std;

CFFEncoder::CFFEncoder(const char *pCodec, CStreamInfo *info, EncodePluginType plugin) : CTransLog(__func__) {
    AVCompat_AVCodec *codec = avcodec_find_encoder_by_name(pCodec);

    if (!codec)
        codec = FindEncoder(info->m_pCodecPars->codec_id);

    if (!codec)
        Warn("Can't find any encoder by codec %s or codec id %d.\n", pCodec, info->m_pCodecPars->codec_id);

    m_Info    = *info;
    m_pEnc    = avcodec_alloc_context3(codec);

    if (codec)
        m_Info.m_pCodecPars->codec_id = codec->id;
    m_plugin = plugin;
}

CFFEncoder::CFFEncoder(AVCodecID id, CStreamInfo *info) : CTransLog(__func__) {
    AVCompat_AVCodec *codec = FindEncoder(id);

    if (!codec)
        Warn("Can't find any encoder by codec id %d.\n", id);

    m_Info    = *info;
    m_pEnc    = avcodec_alloc_context3(codec);
    m_Info.m_pCodecPars->codec_id = id;
}

void CFFEncoder::init(AVDictionary *pDict) {
    int ret;
    m_bInited = false;
    m_pDict   = nullptr;
    m_nFrames = 0;
    m_nLatencyStats = 0;
    m_g_encode_write_count = 0;
    const std::map<std::string, int> *profiles = nullptr;
    const std::map<std::string, int> *levels   = nullptr;
    const char *p                              = nullptr;
    const char *l                              = nullptr;

    AVDictionaryEntry *tag = nullptr;
    tag  = av_dict_get(pDict, "profile", nullptr, 0);
    if (tag) {
        p = tag->value;
        Info("CFFEncoder::init: profile=%s\n", tag->value);
    }
    else {
        Info("CFFEncoder::init: profile tag=null\n");
    }
    tag  = av_dict_get(pDict, "level", nullptr, 0);
    if (tag) {
        l = tag->value;
		Info("CFFEncoder::init: level=%s\n", tag->value);
	}
    else {
        Info("CFFEncoder::init: level tag=null\n");
	}
    switch (m_Info.m_pCodecPars->codec_id) {
        case AV_CODEC_ID_H264:
            // Profile, Level
            /* default configuration of vaapi h264 encode is Profile High and Level 4.2.
             * Profile and Level configurations will affect the max video bitrate limit.
             * it's essential for BRC.
             *
             * avcodec_alloc_context3() will assign default profile/level configurations,
             * while info->m_pCodecPars doesn't.
             * following funtion call avcodec_parameters_to_context() will override the default configurations,
             * which leads to invalid value of Profile and Level.
             * that's why add the two lines here.
             */
            profiles = &vaapi_encode_h264_profiles;
            levels   = &vaapi_encode_h264_levels;
            Info("CFFEncoder::init by AV_CODEC_ID_H264\n");
            break;
        case AV_CODEC_ID_HEVC:
            profiles = &vaapi_encode_h265_profiles;
            levels   = &vaapi_encode_h265_levels;
            Info("CFFEncoder::init by AV_CODEC_ID_HEVC\n");
            break;
        case AV_CODEC_ID_MJPEG:
            profiles = &vaapi_encode_mjpeg_profiles;
            break;
        case AV_CODEC_ID_AV1:
            profiles = &vaapi_encode_av1_profiles;
            levels   = &vaapi_encode_av1_levels;
            Info("CFFEncoder::init by AV_CODEC_ID_AV1\n");
            break;
        default:
            Warn("unsupported codec id %d.\n", m_Info.m_pCodecPars->codec_id);
            break;
    }

    if (profiles) {
        if (!p || profiles->find(p) == profiles->end()) {
            m_Info.m_pCodecPars->profile = profiles->at("default");
        } else {
            m_Info.m_pCodecPars->profile = profiles->at(p);
        }
    }

    if (levels) {
        if (!l || levels->find(l) == levels->end()) {
            m_Info.m_pCodecPars->level = levels->at("default");
        } else {
            m_Info.m_pCodecPars->level = levels->at(l);
        }
    }

    m_pEnc->framerate     = m_Info.m_rFrameRate;
    m_pEnc->time_base     = m_Info.m_rTimeBase;
    m_pEnc->max_b_frames  = 0;
    m_pEnc->refs          = 1;

    if (QSV_ENCODER == m_plugin) {
        // for hevc, qsv-plugin receives level * 10, while vaapi-plugin receives level * 30
        if (m_Info.m_pCodecPars->codec_id == AV_CODEC_ID_HEVC)
             m_Info.m_pCodecPars->level /= 3;

        const char *h = nullptr;
        tag = av_dict_get(pDict, "hrd_compliance", nullptr, 0);
        if (tag)
            h = tag->value;
        if (h && atoi(h) == 1)
            m_pEnc->strict_std_compliance = FF_COMPLIANCE_STRICT;

        const char *b = nullptr;
        tag = av_dict_get(pDict, "rc_mode", nullptr, 0);
        if (tag)
            b = tag->value;
        if (b && !strcmp(b, "CQP"))
            m_pEnc->flags |= AV_CODEC_FLAG_QSCALE; //for qsv plugin to set CQP mode

    }

    ret = avcodec_parameters_to_context(m_pEnc, m_Info.m_pCodecPars);
    if (ret < 0) {
        Error("Invalid streaminfo. Msg: %s\n", ErrToStr(ret).c_str());
        return;
    }

    tag = nullptr;
    while ((tag = av_dict_get(pDict, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        Info("CHECK --> %s: %s\n", tag->key, tag->value);
    }

    av_dict_copy(&m_pDict, pDict, 0);
}

CFFEncoder::~CFFEncoder() {
    av_dict_free(&m_pDict);
#ifdef ENABLE_MEMSHARE
    hw_frames_ctx_recovery();
#endif
    avcodec_free_context(&m_pEnc);
    Info("Total Encoded frames: %d\n", m_nFrames);
}

AVCompat_AVCodec *CFFEncoder::FindEncoder(AVCodecID id) {
    switch (id) {
        case AV_CODEC_ID_H264:       return avcodec_find_encoder_by_name("h264_vaapi");
        case AV_CODEC_ID_MPEG2VIDEO: return avcodec_find_encoder_by_name("mpeg2_vaapi");
        case AV_CODEC_ID_H265:       return avcodec_find_encoder_by_name("hevc_vaapi");
        case AV_CODEC_ID_MJPEG:      return avcodec_find_encoder_by_name("mjpeg_vaapi");
        case AV_CODEC_ID_AV1:        return avcodec_find_encoder_by_name("av1_vaapi");
        default:                     return avcodec_find_encoder(id);
    }
}

int CFFEncoder::write(AVFrame *pFrame) {
    int ret = 0;

    ATRACE_INDEX("CFFEncoder::write", (int)m_g_encode_write_count, 0);
	TimeLog timelog("IRRB_CFFEncoder_write", 0, m_g_encode_write_count++);

    if (!m_bInited) {
        if (!pFrame) {
            Error("Give a null as first frame ???\n");
            return AVERROR(EINVAL);
        }

        if (!m_pEnc->codec) {
            Error("No encoder is specified, EOF.\n");
            return AVERROR_EOF;
        }

#ifndef ENABLE_MEMSHARE
        if (pFrame->hw_frames_ctx)
            m_pEnc->hw_frames_ctx = av_buffer_ref(pFrame->hw_frames_ctx);
#endif
        ret = avcodec_open2(m_pEnc, nullptr, &m_pDict);
        if (ret < 0) {
            Error("Fail to open encoder. Msg: %s\n", ErrToStr(ret).c_str());
            return ret;
        }

        avcodec_parameters_from_context(m_Info.m_pCodecPars, m_pEnc);
        m_Info.m_rFrameRate = m_pEnc->framerate;
        m_Info.m_rTimeBase  = m_pEnc->time_base;

        m_bInited = true;
    }

    if (pFrame)
        Verbose("Encoder <- Filter: dts %s, time %s\n", TsToStr(pFrame->pts).c_str(),
                TsToTimeStr(pFrame->pts, m_pEnc->time_base.num, m_pEnc->time_base.den).c_str());

    if (pFrame && pFrame->pict_type == AV_PICTURE_TYPE_I) {
        Info("force key frame at pts=%s, ts=%s, framenum=%d\n",
            TsToStr(pFrame->pts).c_str(),
            TsToTimeStr(pFrame->pts, m_pEnc->time_base.num,
                m_pEnc->time_base.den).c_str(), m_nFrames);
    }
    if (m_nLatencyStats) {
        m_mProfTimer["write"]->profTimerBegin();
    }

    ATRACE_BEGIN("avcodec_send_frame");
    TimeLog log1("internal", TIME_NONE);
    log1.begin("IRRB_avcodec_send_frame");

    ret = avcodec_send_frame(m_pEnc, pFrame);

    if (m_nLatencyStats) {
        m_mProfTimer["write"]->profTimerEnd("write");
    }
    log1.end();
    ATRACE_END();

    if (ret < 0) {
        if (ret != AVERROR_EOF)
            Error("Fail to push a frame into encoder. Msg: %s.\n", ErrToStr(ret).c_str());
        return ret;
    }

    return ret;
}

int CFFEncoder::read(AVPacket *pPkt) {
    int ret = avcodec_receive_packet(m_pEnc, pPkt);

    if (ret >= 0)
        m_nFrames++;

    return ret;
}

CStreamInfo *CFFEncoder::getStreamInfo() {
    return &m_Info;
}

int CFFEncoder::GetBestFormat(const char *codec, int format)
{
    AVCompat_AVCodec *pCodec = avcodec_find_encoder_by_name(codec);
    if (!pCodec)
        return -1;

    if (pCodec->type == AVMEDIA_TYPE_VIDEO) {
        const AVPixelFormat *pFormat = pCodec->pix_fmts;
        for (; *pFormat != AV_PIX_FMT_NONE; pFormat++)
            if (*pFormat == format)
                return format;

        return pCodec->pix_fmts[0];
    } else if (pCodec->type == AVMEDIA_TYPE_AUDIO) {
        const AVSampleFormat *pFormat = pCodec->sample_fmts;
        for (; *pFormat != AV_SAMPLE_FMT_NONE; pFormat++)
            if (*pFormat == format)
                return format;

        return pCodec->sample_fmts[0];
    }

    return format;
}


int CFFEncoder::setLatencyStats(int nLatencyStats) {
    m_nLatencyStats = nLatencyStats;
    if (m_nLatencyStats) {
        if (m_mProfTimer.find("write") == m_mProfTimer.end()) {
            m_mProfTimer["write"] = new ProfTimer(true);
        }
        m_mProfTimer["write"]->setPeriod(nLatencyStats);
        m_mProfTimer["write"]->enableProf();
    }else{
        if(m_mProfTimer.find("write")!= m_mProfTimer.end()){
            m_mProfTimer["write"]->profTimerReset("write");
        }
    }
    return 0;
}

void CFFEncoder::updateDynamicChangedFramerate(int framerate) {
    m_pEnc->framerate = (AVRational){framerate, 1};
    m_pEnc->time_base = AV_TIME_BASE_Q;
    m_Info.m_rFrameRate = m_pEnc->framerate;
    m_Info.m_rTimeBase  = m_pEnc->time_base;
}

#ifdef ENABLE_MEMSHARE
void CFFEncoder::set_hw_frames(const void *data, AVFrame *hw_frame) {
    m_mHwFrames[data] = hw_frame;
    return;
}
void CFFEncoder::set_hw_frames_ctx(AVBufferRef *hw_frames_ctx) {
    m_pEnc->hw_frames_ctx = hw_frames_ctx;
    return;
}

void CFFEncoder::hw_frames_ctx_backup() {
    m_hw_frames_ctx_bk = m_pEnc->hw_frames_ctx;
}
void CFFEncoder::hw_frames_ctx_recovery() {
    m_pEnc->hw_frames_ctx = m_hw_frames_ctx_bk;
}

void CFFEncoder::getHwFrame(const void *data, AVFrame **hw_frame) {
    if (data && m_mHwFrames[data]) {
        *hw_frame = av_frame_alloc();
        av_frame_ref(*hw_frame, m_mHwFrames[data]);
    }
}

#endif

void CFFEncoder::getProfileNameByValue(std::string &strProfileName, const int iProfileValue) {
    strProfileName = "";
    std::map<std::string, int>::const_iterator it;
    const std::map<std::string, int> *profiles = nullptr;
    switch (m_Info.m_pCodecPars->codec_id) {
    case AV_CODEC_ID_H264:
        profiles = &vaapi_encode_h264_profiles;
        break;
    case AV_CODEC_ID_HEVC:
        profiles = &vaapi_encode_h265_profiles;
        break;
    case AV_CODEC_ID_MJPEG:
        profiles = &vaapi_encode_mjpeg_profiles;
        break;
    case AV_CODEC_ID_AV1:
        profiles = &vaapi_encode_av1_profiles;
        break;
    default:
        Warn("unsupported codec id %d.\n", m_Info.m_pCodecPars->codec_id);
        break;
    }

    if (profiles) {
        for (it = profiles->begin(); it != profiles->end(); ++it) {
            if (it->second == iProfileValue) {
                strProfileName = it->first;
                break;
            }
        }
    }
    return;
}

void CFFEncoder::getLevelNameByValue(std::string &strLevelName, const int iLevelValue) {
    strLevelName = "";
    std::map<std::string, int>::const_iterator it;
    const std::map<std::string, int> *levels = nullptr;
    switch (m_Info.m_pCodecPars->codec_id) {
    case AV_CODEC_ID_H264:
        levels = &vaapi_encode_h264_levels;
        break;
    case AV_CODEC_ID_HEVC:
        levels = &vaapi_encode_h265_levels;
        break;
    case AV_CODEC_ID_MJPEG:
        break;
    case AV_CODEC_ID_AV1:
        levels = &vaapi_encode_av1_levels;
        break;
    default:
        Warn("unsupported codec id %d.\n", m_Info.m_pCodecPars->codec_id);
        break;
    }

    if (levels) {
        for (it = levels->begin(); it != levels->end(); ++it) {
            if (it->second == iLevelValue) {
                strLevelName = it->first;
                break;
            }
        }
    }
    return;
}
