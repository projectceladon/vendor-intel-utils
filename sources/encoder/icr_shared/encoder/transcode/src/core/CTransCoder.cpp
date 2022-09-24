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

#include "CTransLog.h"
#include "core/CTransCoder.h"
#include "core/CFFDemux.h"
#include "core/CFFDecoder.h"
#include "core/CFFFilter.h"
#include "core/CFFEncoder.h"
#include "core/CFFMux.h"

#ifdef ENABLE_QSV
#include <vpl/mfxvideo.h>
#endif
#ifdef ENABLE_TCAE
#include "CTcaeWrapper.h"
#endif

extern "C" {
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>
#include <libavutil/parseutils.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/avstring.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
}

using namespace std;

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define FFMPEG_VERSION "4.2.2"

enum {
    SEI_IDENTIFIER     = 0x02,
    SEI_TIMESTAMP      = 0x08,
    SEI_VALID_TYPE_ALL = SEI_IDENTIFIER | SEI_TIMESTAMP,
};

typedef enum {
    NONE = 0,
    MJPEG = AV_CODEC_ID_MJPEG,
    H264 = AV_CODEC_ID_H264,
    AV1 = AV_CODEC_ID_AV1,
} EncodeType;


struct CUSTOMSEI {
    uint8_t uuid[16] = { 0xbe, 0x57, 0xad, 0xd4, 0xc8, 0xf3, 0x47, 0xb4, 0xb1, 0xef, 0xff, 0xfa, 0xfb, 0x7, 0x1f, 0x2 };
    vhal::client::display_control_t ctrl{};
};


CTransCoder::CTransCoder(string sSrcUrl, string sDstUrl, string sDstFormat) {
    m_pDemux     = new CFFDemux(sSrcUrl.c_str());
    m_pMux       = new CFFMux(sDstUrl.c_str(), sDstFormat.c_str());
}

CTransCoder::CTransCoder(std::string sSrcUrl, CMux *pMux) {
    m_pDemux     = new CFFDemux(sSrcUrl.c_str());
    m_pMux       = pMux;
}

CTransCoder::CTransCoder(CDemux *pDemux, CMux *pMux) {
    m_pDemux     = pDemux;
    m_pMux       = pMux;
}

CTransCoder::CTransCoder(CDemux *pDemux, string sDstUrl, string sDstFormat) {
    m_pDemux     = pDemux;
    m_pMux       = new CFFMux(sDstUrl.c_str(), sDstFormat.c_str());
}

CTransCoder::~CTransCoder() {
    stop();

    av_dict_free(&m_pInProp);
    av_dict_free(&m_pOutProp);
    av_dict_free(&m_pExtProp);

    delete m_pMux;
    for (auto it:m_mEncoders)
        delete it.second;
    for (auto it:m_mFilters)
        delete it.second;
    for (auto it:m_mDecoders)
        delete it.second;
    delete m_Log;

    delete m_DyEncodeTimeLog;
    if (m_MJPEGEncoder)
        delete m_MJPEGEncoder;
#ifdef ENABLE_TCAE
    if (m_tcae)
        delete m_tcae;
#endif
    av_buffer_unref(&m_phw_frames_ctx);
    delete m_pDemux;
}

void CTransCoder::init(int id, AVBufferRef *hw_frames_ctx, AVCodecID codec_type) {
    m_id                   = id;
    m_hw_frames_ctx        = hw_frames_ctx;
    m_screenCaptureQuality = DEFAULT_SCREEN_CAPTURE_QUALITY;
    m_nCodecId             = codec_type;
    m_DyEncodeTimeLog      = new TimeLog("IRRB_Dynamic_Encode_setting", 1);
    m_Log                  = new CTransLog("CTransCoder");

    m_Log->Info("FFmpeg version: %s\n", FFMPEG_VERSION);
}

int CTransCoder::start() {
    int ret;
    const char *pVal = getInOptVal("f", "format");
    m_bitrate = getInitBitrate(m_pOutProp);
    m_gopSize = getInitGopSize(m_pOutProp);

    if (m_thread.joinable()) {
        m_Log->Warn("CTranscoder::start m_thread.joinable()");
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = false;
    }

    ret = m_pDemux->start(pVal, m_pInProp);
    if (ret < 0) {
        m_Log->Warn("m_pDemux failed to start!\n");
        return ret;
    }

    // get init frame rate
    pVal = getOutOptVal("r", "framerate");
    if (pVal)
        m_frameRate = atof(pVal);

    bool isVASurface = m_pDemux->getVASurfaceFlag();
    bool isQSVSurface = m_pDemux->getQSVSurfaceFlag();
    if (isVASurface || isQSVSurface) {
        ret = initHwFramesCtx();
        if (ret == 0) {
            CStreamInfo *pDemuxInfo = nullptr;
            int strIdx = 0;
            pDemuxInfo = m_pDemux->getStreamInfo(strIdx);
            int size = pDemuxInfo->m_pCodecPars->width * pDemuxInfo->m_pCodecPars->height * 4;
        }
        else {
            m_Log->Warn("Init hardware frame context fail!\n");
        }
    }

    try {
        m_thread = std::thread([this] {
            while (1) {
                run();
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_stop) {
                        doOutput(true);
                        break;
                    }
                }
            }
            });
    }
    catch (...) {
        m_Log->Warn("Failed to create transcoder thread!\n");
        return -1;
    }

    return 0;
}

void CTransCoder::stop() {
    if (m_thread.joinable()) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_thread.join();
    }
    m_pDemux->stop();
}

int CTransCoder::setInputProp(const char *key, const char *value) {
    return av_dict_set(&m_pInProp, key, value, 0);
}

int CTransCoder::setOutputProp(const char *key, const char *value) {
    return av_dict_set(&m_pOutProp, key, value, 0);
}

void CTransCoder::run() {
    TimeLog timelog("IRRB_CTransCoder_run", 0, m_g_transcode_count++);
    ATRACE_CALL();

    if (m_pMux->isIrrv())
    {
        //get current connected client numbers.
        int clientNum = getClientsNum ? getClientsNum() : 0;

        //get the flag which indicate if the encode is allowed.
        bool bAllowEncode = getRunAllowed ? getRunAllowed() : false;

        //check if set the env variable for allowing encode unconditionally.
        const char * encode_unconditionally = getenv("encode_unlimit");
        bool isEncodeUnconditionally = (encode_unconditionally && strncmp(encode_unconditionally, "1", strlen("1")) == 0);

        //check if set the env variable for enable encode(which equal to start encode message, allow encoding and transmission)
        const char * encode_setting_by_env = getenv("enable_encode");
        bool isEncodeEnableByEnv = (encode_setting_by_env && strncmp(encode_setting_by_env, "1", strlen("1")) == 0);

        if (!isEncodeUnconditionally) {
            //if no clients connected, or have connection but not allow encode, we need to call the checkNewConn to listen to the mesages in here.
            if (clientNum <= 0 || (clientNum > 0 && !bAllowEncode && !isEncodeEnableByEnv)) {
                clientNum = m_pMux->checkNewConn();
            }

            if (clientNum <= 0) {
                //printf("CTransCoder::run: no irrv clients, return directly!\n");
                usleep(3000);
                return; // irr server have no clients, no need to process input and encode.
            }

            if (!bAllowEncode && !isEncodeEnableByEnv) {
                //printf("CTransCoder::run: not sending the start encoding message and also not allow to do encoding by setting env variable, return directly!\n");
                usleep(3000);
                return;
            }
        }
    }

    if (!m_screenCaptureFlag && m_MJPEGEncoder) {
        m_Log->Info("Stop capture screen.\n");
        delete m_MJPEGEncoder;
        m_MJPEGEncoder          = nullptr;
        m_countScreenCapture    = 0;
        m_screenCaptureInterval = 0;
    }

    int ret = processInput();
    if (ret < 0) {
        if (ret == AVERROR_STREAM_NOT_FOUND) {
            m_Log->Info("AVERROR_STREAM_NOT_FOUND.\n");
            return;
        }
        if (ret == AVERROR_EOF)
        {
            m_Log->Info("AVERROR_EOF.\n");
            doOutput(true);
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        return;
    }

    ret = doOutput(false);
    if (ret < 0) {
        if (ret == AVERROR_EXIT || ret == AVERROR(EIO)) {
            m_Log->Info("AVERROR_EXIT || AVERROR(EIO).\n");
            m_isEncHWError = true;
            hwErrorCount();
        }
        return;
    }

#if 0
    if (m_nLatencyStats) {
        m_mProfTimer["transcode"]->profTimerEnd("transcode");
    }
#endif
}

bool CTransCoder::allStreamFound() {
    for (auto it:m_mStreamFound)
        if (!it.second)
            return false;
    return true;
}

int CTransCoder::newInputStream(int strIdx) {
    int ret = 0;
    CStreamInfo *pDemuxInfo;

    if (m_mStreamFound.find(strIdx) == m_mStreamFound.end())
        m_mStreamFound[strIdx] = false;

    if (m_mStreamFound.at(strIdx))
        return 0;

    pDemuxInfo = m_pDemux->getStreamInfo(strIdx);
    switch (pDemuxInfo->m_pCodecPars->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_AUDIO:
            /* New stream found. Init a decoder for it. */
            m_mDecoders[strIdx] = new CFFDecoder(pDemuxInfo, m_pInProp);
            if (!m_mDecoders[strIdx]) {
                m_Log->Error("OOM!!!\n");
                return -1;
            }
            break;

        default:
            /* Force it to found */
            m_mStreamFound[strIdx] = true;
            m_Log->Warn("%s stream found. Discard it.\n",
                        av_get_media_type_string(pDemuxInfo->m_pCodecPars->codec_type));
            break;
    }

    return ret;
}

int CTransCoder::decode(int strIdx) {
    CDecoder       *pDec  = m_mDecoders[strIdx];
    CFilter        *pFilt = nullptr;
    CStreamInfo *pSrcInfo = nullptr;
    CStreamInfo *pDecInfo = pDec->getDecInfo();
    AVFrame *decFrame;
    int ret;
    int filter_nbthreads = 0;

    bool isVASurface = m_pDemux->getVASurfaceFlag();
    bool isQSVSurface = m_pDemux->getQSVSurfaceFlag();

    if (isVASurface || isQSVSurface) {
        if (nullptr == m_phw_frames_ctx) {
            ret = initHwFramesCtx();
            if (ret != 0) {
                m_Log->Warn("CTransCoder::decode: initHwFramesCtx fail.\n");
            }
        }
    }

    if(m_isEncHWError || m_isResolutionChange) {
        erase_filters();
        if(m_isEncHWError) {
            m_Log->Info("Restart Filter as a result of Encoder Hardware Error.\n");
        }

        if(m_isResolutionChange) {
            m_Log->Info("Restart Filter as a result of Resolution Change.\n");
        }
    }

    /* Init filter after first frame is decoded. */
    if (m_mFilters.find(strIdx) == m_mFilters.end()) {
        CStreamInfo EncInfo = *pDecInfo;
        const char *pVal;

        EncInfo.m_pCodecPars->codec_id = AV_CODEC_ID_NONE;
        EncInfo.m_pCodecPars->codec_tag = -1;
        switch (EncInfo.m_pCodecPars->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
#if 0 ///< Not supported by now
                pVal = getOutOptVal("s", "video_size");
                if (pVal) {
                    ret = av_parse_video_size(&EncInfo.m_pCodecPars->width,
                                              &EncInfo.m_pCodecPars->height,
                                              pVal);
                    if (ret < 0) {
                        m_Log->Error("Bad parameters -s/-video_size %s\n", pVal);
                        return ret;
                    }
                }
#endif

                pVal = getOutOptVal("r", "framerate");
                if (pVal) {
                    ret = av_parse_video_rate(&EncInfo.m_rFrameRate,
                                              pVal);
                    if (ret < 0) {
                        m_Log->Error("Bad parameters -r/-framerate %s\n", pVal);
                        return ret;
                    }
                }

                if (m_nCodecId == AV_CODEC_ID_H264 || m_nCodecId == AV_CODEC_ID_NONE) {
                    if (m_vaapiPlugin)
                        pVal = getOutOptVal("c", "codec", "h264_vaapi");
                    else if (m_qsvPlugin)
                        pVal = getOutOptVal("c", "codec", "h264_qsv");
                } else if (m_nCodecId  == AV_CODEC_ID_HEVC) {
                    if (m_vaapiPlugin)
                        pVal = getOutOptVal("c", "codec", "hevc_vaapi");
                    else if (m_qsvPlugin)
                        pVal = getOutOptVal("c", "codec", "hevc_qsv");
                } else if (m_nCodecId  == AV_CODEC_ID_AV1) {
                    if (m_vaapiPlugin)
                        pVal = getOutOptVal("c", "codec", "av1_vaapi");
                    else if (m_qsvPlugin)
                        pVal = getOutOptVal("c", "codec", "av1_qsv");
                }

                if (pVal) {
                    if (av_stristr(pVal, "_vaapi"))
                        EncInfo.m_pCodecPars->format = AV_PIX_FMT_VAAPI;
                    else if (av_stristr(pVal, "_qsv"))
                        EncInfo.m_pCodecPars->format = AV_PIX_FMT_QSV;
                }

                EncInfo.m_pCodecPars->format = CFFEncoder::GetBestFormat(pVal, EncInfo.m_pCodecPars->format);
                if (EncInfo.m_pCodecPars->format < 0) {
                    m_Log->Error("Encoder is not available\n");
                    return -1;
                }
                break;
            case AVMEDIA_TYPE_AUDIO:
                pVal = getOutOptVal("sr", "sample_rate");
                if (pVal)
                    EncInfo.m_pCodecPars->sample_rate = strtol(pVal, nullptr, 10);

                pVal = getOutOptVal("channles", "channles");
                if (pVal)
                    EncInfo.m_pCodecPars->channels = strtol(pVal, nullptr, 10);

                pVal = getOutOptVal("ac", "acodec", "aac");
                EncInfo.m_pCodecPars->format = CFFEncoder::GetBestFormat(pVal, EncInfo.m_pCodecPars->format);
                break;
            default: break;
        }

        if(m_isResolutionChange) {
            m_Log->Info("set new resolution: %d x %d\n", m_newWidth, m_newHeight);

            pDecInfo->m_pCodecPars->width  = m_newWidth;
            pDecInfo->m_pCodecPars->height = m_newHeight;
            EncInfo.m_pCodecPars->width    = m_newWidth;
            EncInfo.m_pCodecPars->height   = m_newHeight;
        }

        pVal = getOutOptVal("filter", "filter_threads");
        if (pVal) {
            filter_nbthreads = atoi(pVal);
        }
        m_mFilters[strIdx] = new CFFFilter(pDecInfo, &EncInfo, isVASurface, isQSVSurface, m_vaapiPlugin, m_qsvPlugin, filter_nbthreads);
    }

    pFilt    = m_mFilters[strIdx];
    pSrcInfo = pFilt->getSrcInfo();
    while ((decFrame = pDec->read()) != nullptr) {
        decFrame->pts = av_rescale_q(decFrame->pts, pDecInfo->m_rTimeBase,
                                     pSrcInfo->m_rTimeBase);
        decFrame->pict_type = AV_PICTURE_TYPE_NONE;

        if (isVASurface || isQSVSurface) {
            void* p1 = static_cast<void*>(decFrame->data[0]);
            if (isVASurface)
            {
                VASurfaceID* p2 = (VASurfaceID*)p1;
                VASurfaceID surface_id = p2[0];

                //in case that surface_id is invalid which mean aic disconnect or no display buffers,
                //so it need to set a default hardware frame for encoder to avoid encode fail.
                if (m_vaapiPlugin && surface_id == VA_INVALID_SURFACE) {
                    surface_id = m_phw_frames_surfaceid;
                }
                // put the VASurfaceID from the data fild to id fild.
                decFrame->data[3] = decFrame->buf[0]->data = (uint8_t*)(uintptr_t)surface_id;
                decFrame->format = (AVPixelFormat)AV_PIX_FMT_VAAPI;
            }
#ifdef ENABLE_QSV
            else if (isQSVSurface)
            {
                mfxFrameSurface1 *mfxSurf = *(mfxFrameSurface1 **)p1;
                decFrame->data[3] = decFrame->buf[0]->data = (uint8_t*)(uintptr_t)mfxSurf;
                decFrame->format = (AVPixelFormat)AV_PIX_FMT_QSV;
            }
#endif

            // if (m_newWidth > 0 && m_newHeight > 0 && m_isAlphaChannelMode) {
            //     decFrame->width = m_newWidth * 2;
            //     decFrame->height = m_newHeight;
            // }
            if (m_buffer_width > 0 && m_buffer_height > 0 && m_isAlphaChannelMode) {
                decFrame->width = m_buffer_width;
                decFrame->height = m_buffer_height;
            }
            //printf("CTransCoder::decode -->pDec->read surface_id=%d\n", surface_id);

            if (m_phw_frames_ctx) {
                decFrame->hw_frames_ctx = av_buffer_ref(m_phw_frames_ctx);
                if (!decFrame->hw_frames_ctx) {
                    return AVERROR(ENOMEM);
                }
            }
        }

        bool bOrigial_portrait = pDecInfo->m_pCodecPars->width < pDecInfo->m_pCodecPars->height ? true : false;
        bool bClient_portrait = m_crop.client_rect_right < m_crop.client_rect_bottom ? true : false;

        // if ((bOrigial_portrait && bClient_portrait && m_crop.client_rect_right > 0) || (!bOrigial_portrait && !bClient_portrait && m_crop.client_rect_right > 0)) {
        //     if (m_crop.client_rect_right != 0 && m_crop.client_rect_bottom != 0 && m_crop.fb_rect_right != 0 && m_crop.fb_rect_bottom != 0) {
        //         int result1 = m_crop.client_rect_right * m_crop.fb_rect_bottom;
        //         int result2 = m_crop.fb_rect_right * m_crop.client_rect_bottom;
        //         if (result1 < result2) {
        //             decFrame->crop_top = 0;
        //             decFrame->crop_bottom = 0;
        //             int crop_left = 0;
        //             int crop_right = 0;
        //             int total_crop = m_crop.fb_rect_right - (m_crop.fb_rect_bottom * m_crop.client_rect_right) / m_crop.client_rect_bottom;
        //             crop_left = total_crop / 2;
        //             crop_right = total_crop - crop_left;

        //             decFrame->crop_left = crop_left;
        //             decFrame->crop_right = crop_right;

        //             //m_Log->Info("Debug: cropping info: left:%d, top:%d, right:%d, bottom:%d\n",
        //                 //+decFrame->crop_left, decFrame->crop_top, decFrame->crop_right, decFrame->crop_bottom);
        //         }
        //         else if (result1 > result2) {
        //             decFrame->crop_left = 0;
        //             decFrame->crop_right = 0;

        //             int crop_top = 0;
        //             int crop_bottom = 0;
        //             int total_crop = m_crop.fb_rect_bottom - (m_crop.fb_rect_right * m_crop.client_rect_bottom) / m_crop.client_rect_right;
        //             crop_top = total_crop / 2;
        //             crop_bottom = total_crop - crop_top;

        //             decFrame->crop_top = crop_top;
        //             decFrame->crop_bottom = crop_bottom;

        //             //m_Log->Info("Debug: cropping info: left:%d, top:%d, right:%d, bottom:%d\n",
        //                 //+decFrame->crop_left, decFrame->crop_top, decFrame->crop_right, decFrame->crop_bottom);
        //         }
        //     }
        // }

        // The crop info are now calculated in CG_Proxy, 
        // so comment the codes above and directly utilize the values of crop_top/bottom/left/right
        if ((bOrigial_portrait && bClient_portrait && m_crop.client_rect_right > 0) || (!bOrigial_portrait && !bClient_portrait && m_crop.client_rect_right > 0)) {
            if (m_crop.crop_top >= 0 && m_crop.crop_bottom >= 0 && m_crop.crop_left >= 0 && m_crop.crop_right >= 0) {
                if (m_crop.crop_top < m_crop.fb_rect_bottom && m_crop.crop_bottom < m_crop.fb_rect_bottom && m_crop.crop_left < m_crop.fb_rect_right && m_crop.crop_right < m_crop.fb_rect_right) {
                    decFrame->crop_top = m_crop.crop_top;
                    decFrame->crop_bottom = m_crop.crop_bottom;
                    decFrame->crop_left = m_crop.crop_left;
                    decFrame->crop_right = m_crop.crop_right;
                    // m_Log->Info("Debug: cropping info: top:%d, bottom:%d, left:%d, right:%d\n", decFrame->crop_top, decFrame->crop_bottom, decFrame->crop_left, decFrame->crop_right);
                }
            }
        }
        ret = pFilt->push(decFrame);
        if (ret < 0) {
            m_Log->Error("Failed to push frame into filters. Msg: %s\n",
                         m_Log->ErrToStr(ret).c_str());
            av_frame_free(&decFrame);
            return ret;
        }
        av_frame_free(&decFrame);
    }

    return 0;
}

int CTransCoder::processInput() {
    int                 ret = 0;
    CDecoder          *pDec = nullptr;
    CStreamInfo   *pDecInfo = nullptr;
    CStreamInfo *pDemuxInfo = nullptr;
    bool bOrigial_portrait = true;
    bool bClient_portrait = true;
    IrrPacket pkt;

    TimeLog timelog("IRRB_CTransCoder_processInput");
    ATRACE_CALL();

    av_init_packet(&pkt.av_pkt);

    if (m_pIOStreamWriter) {
        m_pIOStreamWriter->feedVideoFrameFromInputStream();
    }

    ret = m_pDemux->readPacket(&pkt);

#if 0
    if(m_nLatencyStats) {
        m_mProfTimer["transcode"]->profTimerBegin();
    }
#endif
    if (ret < 0) {
        if (ret == AVERROR_STREAM_NOT_FOUND)
            goto err_out;
        if (ret == AVERROR_EOF) {
            m_Log->Warn("Eof detected. Exiting...\n");
            /* Flush decoder */
            for (auto it:m_mDecoders) {
                it.second->write(nullptr);
                decode(it.first);
            }
        } else
            m_Log->Error("Failed to read a pkt. Msg: %s.\n", m_Log->ErrToStr(ret).c_str());
        goto err_out;
    }

    if (m_mStreamFound.find(pkt.av_pkt.stream_index) == m_mStreamFound.end()) {
        /* New stream found, discard packet */
        m_Log->Warn("New stream found.\n");
        ret = newInputStream(pkt.av_pkt.stream_index);
        if (ret < 0)
            goto err_out;
    }

    pDemuxInfo = m_pDemux->getStreamInfo(pkt.av_pkt.stream_index);
    if (m_mDecoders.find(pkt.av_pkt.stream_index) == m_mDecoders.end()) {
        m_Log->Debug("Disgarding packet from stream[%d], type %s.\n", pkt.av_pkt.stream_index,
                     av_get_media_type_string(pDemuxInfo->m_pCodecPars->codec_type));
        goto err_out;
    }

    pDec     = m_mDecoders[pkt.av_pkt.stream_index];
    pDecInfo = pDec->getDecInfo();

    bOrigial_portrait = pDecInfo->m_pCodecPars->width < pDecInfo->m_pCodecPars->height ? true : false;
    bClient_portrait = m_crop.client_rect_right < m_crop.client_rect_bottom ? true : false;
    if (m_crop.valid_crop == 1 && ((bOrigial_portrait && bClient_portrait && m_crop.client_rect_right > 0) || (!bOrigial_portrait && !bClient_portrait && m_crop.client_rect_right > 0))) {
        if ((m_crop.client_rect_right != m_crop.fb_rect_right && m_prev_client_right != m_crop.client_rect_right) || 
            (m_crop.client_rect_bottom != m_crop.fb_rect_bottom && m_prev_client_bottom != m_crop.client_rect_bottom)) {
            changeResolution(m_crop.client_rect_right, m_crop.client_rect_bottom);
            m_prev_client_bottom = m_crop.client_rect_bottom;
            m_prev_client_right = m_crop.client_rect_right;
            // m_Log->Info("Debug: processInput call irr_stream_change_resolution : Width:%d, Height:%d\n", m_crop.client_rect_right, m_crop.client_rect_bottom);
        }
    }

    // record the original pts for latency profiling, since pts is got from av_gettime_relative
    m_mPktPts[pkt.av_pkt.stream_index] = pkt.av_pkt.pts;

    av_packet_rescale_ts(&pkt.av_pkt, pDemuxInfo->m_rTimeBase, pDecInfo->m_rTimeBase);
    ret = pDec->write(&pkt.av_pkt);
    if (ret < 0) {
        m_Log->Error("Failed to write packet into Decoder[%d]. Msg: %s.\n",
                     pkt.av_pkt.stream_index, m_Log->ErrToStr(ret).c_str());
        goto err_out;
    }

    ret = decode(pkt.av_pkt.stream_index);
    if (ret < 0) {
        m_Log->Error("Decode[%d] packet failure. Msg: %s.\n",
                     pkt.av_pkt.stream_index, m_Log->ErrToStr(ret).c_str());
        goto err_out;
    }

    if (pkt.display_ctrl != nullptr) {
        m_DispCtrlQueue.push(std::move(pkt.display_ctrl));
    }

err_out:
    av_packet_unref(&pkt.av_pkt);
    return ret;
}

#if BUILD_FOR_HOST
#define ICRM_FIFO_PATH   "/tmp/icrm-fifo"
#else
#define ICRM_FIFO_PATH   "/ipc/icrm-fifo"
#endif
/*
 * @id: instance id
 */
int CTransCoder::publishStatusToResourceMonitor(uint32_t id, void * status) {
    int fd, write_bytes = 0;
    unsigned char write_buffer[128];
    char fifo_path[50];
    char *msg;
    const char *dev_dri;
    int data_len, gpuid, len_to_write = 0;
    const int max_msg_len = sizeof(write_buffer) - 4;
    //Currently, we only need to report fps
    float fps = *(float *)status;
    /* get the gpu id we are on */
    dev_dri = getenv("VAAPI_DEVICE");
    if (!dev_dri)
        dev_dri = "/dev/dri/renderD128";
    sscanf(dev_dri, "/dev/dri/renderD%d", &gpuid);
    if (gpuid > 0 && gpuid <= 256)
    {
        gpuid -= 128;
    }
    else
    {
        return 0;
    }
    snprintf(fifo_path, sizeof(fifo_path), ICRM_FIFO_PATH "-gpu%02d", gpuid);
    fifo_path[sizeof(fifo_path)-1] = '\0';
    //We are fifo producer
    if((fd = open(fifo_path, O_WRONLY | O_NONBLOCK)) <= 0) {
        /*
         * This can be a result that no ICR monitor instance is running
         * or it is merely not interested in the local instance info
         * (so that it's not waiting data coming out of the fifo)
         */
        return fd;
    }
    msg = (char *)write_buffer + 4;
    /*
     * Note: We need to make sure data len will not exeed 254,
     *      Otherwise the package len may conflict with
     *      the sync(start of msg) flag
     *
     * 0xff | id | len | msg
     * e.g.
     * 0xff | 1  |  11 | "gfps=30.00"
     */
    //start of msg
    write_buffer[0] = 0xff;
    /*
     * id to be transmitted is truncated to 16 bit with MSB followed by LSB
     */
    write_buffer[1] = (unsigned char)((id >> 8) & 0xfful);
    write_buffer[2] = (unsigned char)(id & 0xfful);
    //snprintf will leave at least one byte for string ending character
    snprintf(msg, max_msg_len, "vfps=%.2f", fps);
    data_len = strlen(msg) + 1;
    if(data_len + 1 >= 0xff) {
        printf("IRR resource monitor frontend: msg lenth too long!!!\n");
        fflush(stdout);
        close(fd);
        return 0;
    }
    write_buffer[3] = strlen(msg) + 1; //including '\0'
    len_to_write = strlen(msg) + 5;
#if 0
    for(int i = 0; i < len_to_write; i++){
        printf("%02x ",(unsigned int)write_buffer[i]);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
#endif
    write_bytes = write(fd, write_buffer, len_to_write);

    close(fd);

    return write_bytes;

}

static inline uint64_t getUs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ll + ts.tv_nsec / 1000;
}

void CTransCoder::hwErrorCount() {
    uint64_t currTimeInMs  = getUs() / 1000;
    uint64_t deltaTimeInMs = currTimeInMs - m_hwErrorOccurTimeInMs;

    if(deltaTimeInMs >= HW_ERROR_INTERVAL) {
        // If the delta time between two errors is greater than HW_ERROR_INTERVAL,
        // this error and the last one are two errors.
        m_hwErrorCnt = 0;
        m_hwErrorDurationInMs = 0;
    } else {
        // If the delta time between two errors is less than HW_ERROR_INTERVAL,
        // we assume they are the same error.
        m_hwErrorCnt++;
        m_hwErrorDurationInMs += deltaTimeInMs;
    }
    m_hwErrorOccurTimeInMs = currTimeInMs;

    if(m_hwErrorDurationInMs > HW_ERROR_DURATION_MAX || m_hwErrorCnt > HW_ERROR_COUNT_MAX) {
        // Exit encoder when the hardware error lasted too long. And it will be restarted by system.
        m_Log->Info("Exit!! A Hardwre error lasts %ld ms, be detected %d times.\n", m_hwErrorDurationInMs, m_hwErrorCnt);
        int pid = getpid();
        kill(pid, SIGINT);
    }
}

int CTransCoder::doOutput(bool flush) {

	TimeLog timelog("IRRB_CTransCoder_doOutput");
    ATRACE_CALL();

    if(m_mFilters.size()>0)
    for (auto it:m_mFilters) {
        int        idx = it.first;
        CFilter *pFilt = it.second;
        CEncoder *pEnc = nullptr;
        AVFrame *pFrame;
        AVFrame *pFrameEnc = nullptr;
        AVPacket pkt;
        int ret;

        if (m_isEncHWError || m_isResolutionChange || m_isCodecChange || m_isProfileLevelChange) {
            erase_encoders();

            if(m_isEncHWError) {
                m_Log->Info("Restart Encoder as a result of Encoder Hardware Error.\n");
                m_isEncHWError = false;
            }
            if(m_isResolutionChange) {
                m_Log->Info("Restart Encoder as a result of Resolution Change.\n");
                m_isResolutionChange = false;
            }

            if (m_isCodecChange) {
                m_Log->Info("Restart Encoder as a result of codec Change.\n");
                m_isCodecChange = false;
            }

            if (m_isProfileLevelChange) {
                m_Log->Info("Restart Encoder as a result of profile or level Change.\n");
                m_isProfileLevelChange = false;
            }
        }

        if (m_mEncoders.find(idx) == m_mEncoders.end()) {
            const char *pVal = "";
            CStreamInfo *pSinkInfo = pFilt->getSinkInfo();

            EncodePluginType encode_plugin = m_qsvPlugin ? QSV_ENCODER : VA_ENCODER;

            if (pSinkInfo->m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO) {
                if (m_nCodecId == AV_CODEC_ID_H264 || m_nCodecId == AV_CODEC_ID_NONE) {
                    if (m_qsvPlugin)
                        pVal = getOutOptVal("c", "codec", "h264_qsv");
                    else if (m_vaapiPlugin)
                        pVal = getOutOptVal("c", "codec", "h264_vaapi");
                } else if (m_nCodecId == AV_CODEC_ID_HEVC) {
                    if (m_qsvPlugin)
                        pVal = getOutOptVal("c", "codec", "hevc_qsv");
                    else if (m_vaapiPlugin)
                        pVal = getOutOptVal("c", "codec", "hevc_vaapi");
                } else if (m_nCodecId == AV_CODEC_ID_AV1) {
                    if (m_qsvPlugin)
                        pVal = getOutOptVal("c", "codec", "av1_qsv");
                    else if (m_vaapiPlugin)
                        pVal = getOutOptVal("c", "codec", "av1_vaapi");
                }
            } else {
                pVal = getOutOptVal("ac", "acodec", "aac");
            }
            m_mEncoders[idx] = new CFFEncoder(pVal, pSinkInfo, encode_plugin);
            ((CFFEncoder*)m_mEncoders[idx])->init(m_pOutProp);

#ifdef ENABLE_MEMSHARE
            for(auto it : m_mHwFrames) {
                ((CFFEncoder*)m_mEncoders[idx])->set_hw_frames(it.first, it.second);
            }

            ((CFFEncoder*)m_mEncoders[idx])->hw_frames_ctx_backup();
            ((CFFEncoder*)m_mEncoders[idx])->set_hw_frames_ctx(m_hw_frames_ctx);
#endif
            if(pSinkInfo->m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO
                    && m_nLatencyStats) {
                m_mEncoders[idx]->setLatencyStats(m_nLatencyStats);
            }
        }

        if (m_screenCaptureFlag && !m_MJPEGEncoder && m_screenCaptureInterval > 0) {
            CStreamInfo *pSinkInfo = pFilt->getSinkInfo();
            if (pSinkInfo->m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_MJPEGEncoder = new CFFEncoder(AV_CODEC_ID_MJPEG, pSinkInfo);
                if (!m_MJPEGEncoder) {
                    m_Log->Error("Failed to create MJPEG encoder to capture screen!\n");
                } else {
                    AVDictionary *m_MJPEGDict = nullptr;
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d", m_screenCaptureQuality);
                    av_dict_set(&m_MJPEGDict, "global_quality", buf, 0);
                    m_Log->Info("Start capture screen and the interval = %d frames.\n", m_screenCaptureInterval);
                    ((CFFEncoder*)m_MJPEGEncoder)->init(m_MJPEGDict);
                    m_countScreenCapture = 0;
                    av_dict_free(&m_MJPEGDict);
                }
            }
        }

        bool bPopFrame = false;
      
        pEnc = m_mEncoders[idx];
        while ((pFrame = pFilt->pop()) != nullptr || flush) {  
            av_init_packet(&pkt);
            if (pFrame) {
                bPopFrame = true;
            }

            if (!pFrame && flush) {
                pEnc->write(nullptr);
            } else {
                dynamicSetEncParameters(pEnc, pFrame, &pFrameEnc);
                if (m_isFramerateChange) {
                    updateDynamicChangedFramerate(idx);
                }
                ret = pEnc->write(pFrameEnc);
                if (ret < 0) {
                    if (ret != AVERROR_EOF)
                        m_Log->Error("Failed to encode a frame. Msg: %s.\n",
                                     m_Log->ErrToStr(ret).c_str());
                    else
                        m_Log->Warn("Eof detected. Exiting...\n");

                    av_frame_free(&pFrameEnc);
                    return ret;
                }
                // av_frame_free(&pFrameEnc);

                if (!m_mStreamFound.at(idx)) {
                    ret = m_pMux->addStream(idx, pEnc->getStreamInfo());
                    if (ret < 0) {
                        m_Log->Error("Failed to add stream.\n");
                        return ret;
                    }
                    m_mStreamFound[idx] = true;
                }

                if (!allStreamFound()) continue;
            }

            while ((ret = pEnc->read(&pkt)) >= 0) {
                /* dynamic encode setting time log
                 * the time interval is from libtrans received the dynamic encode setting
                 * to related encoded frame output.
                 */
                if (m_setDynamicEncode) {
                    m_DyEncodeTimeLog->end();
                    m_setDynamicEncode = false;
                }
                /*
                 * Calculate encode FPS and publish it
                 */
                if (m_fpsStats) {
                    static size_t lastEncFrames = 0;
                    size_t curEncFrames = pEnc->getEncFrames();
                    uint64_t currTimeInMs = getUs() / 1000;
                    if (currTimeInMs - m_statsStartTimeInMs >= 1000) {
                        float dt = (float)(currTimeInMs - m_statsStartTimeInMs) / 1000.f;
                        /*
                         * Just skip FPS calculation when size_t overflow happens
                         * It will soon be resolved by continuously increasing
                         * encode frames
                         * */
                        if (likely(curEncFrames > lastEncFrames)) {
                            float fps = (curEncFrames - lastEncFrames) / dt;
                            publishStatusToResourceMonitor(m_id, &fps);
                            m_Log->Info("ICR encoder frame=%zu fps=%.2f\n", curEncFrames, fps);
                        }
                        m_statsStartTimeInMs = currTimeInMs;
                        lastEncFrames = curEncFrames;
                    }
                }
                // calculate the real-time bitrate at stable fps
                if (!m_pDemux->getRenderFpsEncFlag()) {
                    // calculate bitrate based on framerate and total frame size in framerate cycle
                    m_totalFrameSizeInFrameRate += pkt.size;
                    if (m_frameRate > 0 && ++m_frameNumInFrameRate >= m_frameRate) {
                        m_Log->Info("ICR encoder current bitrate=%.2f(Kbps)\n", (m_totalFrameSizeInFrameRate / 1024.0 * 8));
                        m_totalFrameSizeInFrameRate = 0;
                        m_frameNumInFrameRate = 0;
                    }
                }

#ifdef ENABLE_TCAE
                if (m_tcaeEnabled && m_tcae)
                    m_tcae->UpdateEncodedSize(pkt.size);
#endif

                pkt.stream_index = idx;
                ret = m_pMux->write(&pkt);
                if (m_nLatencyStats) {
                    m_mProfTimer["cycle_trans"]->profTimerEnd("cycle_trans", m_mPktPts[idx]);
                }
                if (ret < 0) {
                    av_frame_free(&pFrameEnc);
                    av_packet_unref(&pkt);
                    m_Log->Error("Failed to output a frame.\n");
                    return ret;
                }

                // check endoced size changing
                TimeLog log("IRRB_CTransCoder_SizeChanging", 1);
                int size = pkt.size;
                int diff = size - m_nLastPktSize;
                //printf("------------------> received pkt diff=%d, size=%d, lastSize=%d\n", diff, size, m_nLastPktSize);
                if (diff > SIZE_CHANGE_THRESHOLD) {
                    //printf("------------------> received pkt: diff=%d, size=%d, lastSize=%d\n", diff, size, m_nLastPktSize);
                    log.begin("IRRB_CTransCoder_SizeChanging", diff, size);
                    log.end();
                    ATRACE_INDEX("CTransCoder::SizeChanging", diff, size);
                }
                m_nLastPktSize = size;

                av_packet_unref(&pkt);
            }

            if (ret == AVERROR(EIO)) {
                av_frame_free(&pFrameEnc);
                av_packet_unref(&pkt);
                m_Log->Error("Failed to read the encoding output frame. ret =%d\n", ret);
                return ret;
            }

            // screen capture
            if (m_MJPEGEncoder && m_screenCaptureInterval > 0 && m_countScreenCapture % m_screenCaptureInterval == 0) {
                pkt.stream_index = idx;
                ret = m_MJPEGEncoder->write(pFrameEnc);
                if (ret < 0) {
                    if (ret != AVERROR_EOF) {
                        m_Log->Warn("MJPEG encoder: Failed to encode a frame Msg: %s.\n",
                                    m_Log->ErrToStr(ret).c_str());
                    } else {
                        m_Log->Warn("MJPEG encoder: Eof detected.\n");
                    }
                    av_frame_free(&pFrameEnc);
                    return ret;
                }

                while ((ret = m_MJPEGEncoder->read(&pkt)) >= 0) {
                    ret = m_pMux->write(pkt.data, pkt.size, MJPEG);
                    if (ret < 0) {
                        av_frame_free(&pFrameEnc);
                        av_packet_unref(&pkt);
                        m_Log->Warn("MJPEG encoder: Failed to output a frame.\n");
                        return ret;
                    }

                    av_packet_unref(&pkt);
                }
                m_countScreenCapture = 0;
            }

            av_frame_free(&pFrameEnc);

            if (m_screenCaptureFlag)
                ++m_countScreenCapture;

            if (!pFrame && flush) break;
        }

        if (!bPopFrame) {
            int iLastErrorCode = pFilt->getLastError();
            if (iLastErrorCode == AVERROR(EIO)) {
                std::map<int, CFilter *>::iterator it;
                for (it = m_mFilters.begin(); it != m_mFilters.end();) {
                    delete it->second;
                    m_mFilters.erase(it++);
                }
            }
        }
    }

    return 0;
}

const char* CTransCoder::getInOptVal(const char *short_name, const char *long_name,
                                   const char *default_value) {
    AVDictionaryEntry *dEntry = nullptr;

    if ((dEntry = av_dict_get(m_pInProp, short_name, nullptr, 0)) != nullptr ||
        (dEntry = av_dict_get(m_pInProp, long_name, nullptr, 0)) != nullptr)
        return dEntry->value;

    return default_value;
}

const char* CTransCoder::getOutOptVal(const char *short_name, const char *long_name,
                                   const char *default_value) {
    AVDictionaryEntry *dEntry = nullptr;

    if ((dEntry = av_dict_get(m_pOutProp, short_name, nullptr, 0)) != nullptr ||
        (dEntry = av_dict_get(m_pOutProp, long_name, nullptr, 0)) != nullptr)
        return dEntry->value;

    return default_value;
}

void CTransCoder::dynamicSetEncParameters(CEncoder *pEnc, AVFrame *pFrame, AVFrame **pFrameEnc) {
    int curEncFrames = 0;

    if (pEnc == nullptr || pFrame == nullptr)
        return;

#ifdef ENABLE_MEMSHARE
    AVFrame *hw_frame = nullptr;
    if (pEnc) {
        pEnc->getHwFrame(pFrame->data[0], &hw_frame);
    }
    if (hw_frame != nullptr) {
        *pFrameEnc = hw_frame;
        (*pFrameEnc)->pts = pFrame->pts;
        av_frame_free(&pFrame);
    }
    else {
        *pFrameEnc = pFrame;
    }
#else
    if (pFrameEnc)
    {
        *pFrameEnc = pFrame;
    }
#endif

    if (!pFrameEnc || !(*pFrameEnc))
        return;

    curEncFrames = pEnc->getEncFrames();

    if(m_forceKeyFrame) {
        m_Log->Info("Force key frame at framenum=%d\n", curEncFrames);
        (*pFrameEnc)->pict_type = AV_PICTURE_TYPE_I;
        m_forceKeyFrame = 0;
    }
    else {
        if (m_qsvPlugin)
            // for qsv plugin, this pict_type needs to be set explicitly
            (*pFrameEnc)->pict_type = AV_PICTURE_TYPE_P;
    }

    if(m_setQP) {
        m_Log->Info("set qp at framenum=%d, qp=%d\n", curEncFrames, m_setQP);
#ifdef FFMPEG_v42
        AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_QP);
        if (NULL == fside) {
            fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_QP, sizeof(int));
        }
        if (fside) {
            m_Log->Info("Set qp: %d successfully!\n", m_setQP);
            memcpy(fside->data, &m_setQP, sizeof(int));
        } else {
            m_Log->Warn("Failed to set qp side-data \n");
        }
#else
        m_Log->Warn("Failed to set dynamic qp side-data\n");
#endif
        m_setQP = 0;
    }
    if(m_setBitrate) {
        m_Log->Info("set dynamic bitrate at framenum=%d, bitrate=%d\n", curEncFrames, m_setBitrate);
#ifdef FFMPEG_v42
        m_Log->Info("FFMPEG_v42: use ffmpeg version 4.2. Config_br!\n");
        AVRateControl rc = {static_cast<uint64_t>(m_setBitrate), static_cast<uint32_t>(m_minQP), static_cast<uint32_t>(m_maxQP)};
        AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_BITRATE);
        if (NULL == fside) {
            fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_BITRATE, sizeof(AVRateControl));
        }
        if (fside) {
            m_Log->Info("FFMPEG_v42 set dynamic bitrate: %d successfully!\n", m_setBitrate);
            memcpy(fside->data, &rc, sizeof(AVRateControl));
        } else {
            m_Log->Warn("Failed to set dynamic bitrate side-data \n");
        }
#else
        m_Log->Warn("Failed to set dynamic bitrate side-data\n");
#endif

        m_setBitrate = 0;
    }

    if (m_setMaxBitrate) {
        m_Log->Info("set dynamic max bitrate at framenum=%d, max_bitrate=%d\n", curEncFrames, m_maxBitrate);
#ifdef FFMPEG_v42
        AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_MAX_BITRATE);
        if (NULL == fside) {
            fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_MAX_BITRATE, sizeof(int));
        }
        if (fside) {
            m_Log->Info("FFMPEG_v42 set dynamic max bitrate: %d successfully!\n", m_setMaxBitrate);
            memcpy(fside->data, &m_setMaxBitrate, sizeof(int));
        }
#else
        m_Log->Warn("This FFmepg version doesn't support for setting dynamicly max bitrate.");
#endif
        m_setMaxBitrate = 0;
    }

    if (m_setFramerate) {
        m_Log->Info("set dynamic frame rate at framenum=%d, framerate=%.2f\n", curEncFrames, m_setFramerate);
#ifdef FFMPEG_v42
        AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_FRAME_RATE);
        if (NULL == fside) {
            fside =av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_FRAME_RATE, sizeof(float));
        }

        if (fside) {
            m_Log->Info("set dynamic frame rate side_data successfully! m_setFramerate=%.2f\n", m_setFramerate);
            memcpy(fside->data, &m_setFramerate, sizeof(float));
        } else {
            m_Log->Warn("Failed to set dynamic frame rate side_data\n");
        }
#else
        m_Log->Warn("This FFmepg version doesn't support for setting dynamicly frame rate.");
#endif
        m_setFramerate = 0;
        m_totalFrameSizeInFrameRate = 0;
        m_frameNumInFrameRate = 0;
        m_isFramerateChange = true;
    }

    if (m_setMaxframeSize) {
        m_Log->Info("set dynamic max frame size at framenum=%d, max_frame_size=%d bytes\n", curEncFrames, m_setMaxframeSize);
#ifdef FFMPEG_v42
        m_Log->Info("FFMPEG_v42: use ffmpeg version 4.2 config max frame size!\n");
        AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_MAX_FRAME_SIZE);
        if (NULL == fside) {
            fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_MAX_FRAME_SIZE, sizeof(int));
        }
        if (fside) {
            memcpy(fside->data, &m_setMaxframeSize, sizeof(int));
            m_Log->Info("FFMPEG_v42: set dynamic max frame size: %d bytes successfully!\n", m_setMaxframeSize);
        } else {
            m_Log->Warn("Failed to set dynamic max frame size side-data \n");
        }
#else
        m_Log->Warn("This ffmpeg version doesn't support setting dynamic max frame size!\n");
#endif
        m_setMaxframeSize = 0;
    }

    if (m_setRIR) {
        m_Log->Info("set dynamic rolling intra refresh at framenum=%d, type=%d, cycle_size=%d, delta_qp=%d\n",
                curEncFrames, m_IntRefType, m_IntRefCycleSize, m_IntRefQPDelta);
#ifdef FFMPEG_v42
        if ((m_IntRefType == 1) || (m_IntRefType == 2)) {
            std::string rir_type = (m_IntRefType == 1) ? "vertical" : "horizontal";
            if ((m_IntRefCycleSize > 0) && ((m_IntRefQPDelta >= -51) && (m_IntRefQPDelta <= 51))) {
                m_Log->Info("set dynamic RIR, the RIR type is %s, cycle size is %d, delta qp is %d.\n",
                        rir_type.c_str(), m_IntRefCycleSize, m_IntRefQPDelta);

                AVRollingIntraRefresh rir_config = {static_cast<uint16_t>(m_IntRefType), static_cast<uint16_t>(m_IntRefCycleSize), static_cast<uint8_t>(m_IntRefQPDelta)};
                AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_RIR_FRAME);
                if (NULL == fside) {
                    fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_RIR_FRAME, sizeof(AVRollingIntraRefresh));
                }
                if (fside) {
                    memcpy(fside->data, &rir_config, sizeof(AVRollingIntraRefresh));
                    m_Log->Info("FFMPEG_v42: set dynamic RIR successfully!\n");
                } else {
                    m_Log->Warn("Failed to set dynamic RIR side-data \n");
                }
            } else {
                m_Log->Warn("Failed to set RIR. The cycle size should be greater than 0 and the delta qp should be in range [-51, 51] \n");
            }

        } else {
            m_Log->Warn("Failed to set RIR. The RIR type only be set 1 or 2, 1 means vertical and 2 means horizontal.\n");
        }
#else
        m_Log->Warn("This ffmpeg version doesn't support setting dynamic RIR!\n");
#endif
        m_setRIR = false;
    }

    if(m_setROI) {
#ifdef FFMPEG_v42
        m_Log->Info("set regions of interest at frame_number: %d with following parameters:\n", curEncFrames);
        for(int i=0; i < m_numROI; i++) {
            m_Log->Info("roi index=%d, position x=%d, position y=%d, width=%d, height=%d, roi_value=%d\n", i,
                        m_roiPara[i].x, m_roiPara[i].y, m_roiPara[i].width, m_roiPara[i].height, m_roiPara[i].roi_value);
        }
        // set roi enabled when dynamic setting
        setOutputProp("roi_enabled", "1");
        AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_REGIONS_OF_INTEREST);
        if (NULL == fside) {
            fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_REGIONS_OF_INTEREST, m_numROI*sizeof(AVRoI));
        }
        if(fside) {
            memcpy(fside->data, m_roiPara, m_numROI * sizeof(m_roiPara[0]));
            m_Log->Info("FFMPEG_v42: set dynamic ROI successfully!\n");
        }
#else
        m_Log->Warn("This ffmpeg version doesn't support setting dynamic ROI!\n");
#endif
    m_setROI = false;
    }

    if (m_setMinMaxQP) {
        m_Log->Info("set dynamic min/max qp at framenum=%d, min_qp=%d, max_qp=%d\n", curEncFrames, m_minQP, m_maxQP);
#ifdef FFMPEG_v42
        // setting min/max qp is supported in case bitrate set
        if (m_bitrate > 0) {
            if (m_minQP < 1 || m_maxQP > 51 || m_minQP > m_maxQP) {
                m_Log->Warn("qp should be in [1, 51] and min qp shouldn't be greater than max qp.\n");
            } else {
                AVRateControl rc = {static_cast<uint64_t>(m_bitrate), static_cast<uint32_t>(m_minQP), static_cast<uint32_t>(m_maxQP)};
                AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_BITRATE);
                if (NULL == fside) {
                    fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_BITRATE, sizeof(AVRateControl));
                }
                if (fside) {
                    m_Log->Info("FFMPEG_v42 set dynamic min qp %d and max qp %d successfully!\n", m_minQP, m_maxQP);
                    memcpy(fside->data, &rc, sizeof(AVRateControl));
                } else {
                    m_Log->Warn("Failed to set dynamic min/max qp side-data \n");
                }
            }
        } else {
            AVDictionaryEntry *tag = NULL;
            tag = av_dict_get(m_pOutProp, "rc_mode", NULL, 0);
            if (tag) {
                m_Log->Warn("The %s mode doesn't support setting min/max qp", tag->value);
            } else {
                m_Log->Warn("The CQP and ICQ mode doesn't support setting min/max qp");
            }
        }
#else
        m_Log->Warn("This ffmpeg version doesn't support setting dynamic min/max qp!\n");
#endif
        m_setMinMaxQP = 0;
    }

    if (!m_DispCtrlQueue.empty())
    {
        CUSTOMSEI sei;
        sei.ctrl = *m_DispCtrlQueue.front();
        m_DispCtrlQueue.pop();
        AVFrameSideData* fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_SEI_UNREGISTERED);
        if (NULL == fside) {
            fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_SEI_UNREGISTERED, sizeof(CUSTOMSEI));
        }
        if (fside) {
            m_Log->Info("set CUSTMOM SEI successfully!\n");
            m_Log->Debug("alpha=%d, top_layer=%d, rotation=%d, viewport: l=%d t=%d r=%d b=%d\n",
                sei.ctrl.alpha, sei.ctrl.top_layer, sei.ctrl.rotation,
                sei.ctrl.viewport.l, sei.ctrl.viewport.t, sei.ctrl.viewport.r, sei.ctrl.viewport.b);
            memcpy(fside->data, &sei, sizeof(CUSTOMSEI));
        }
        else {
            m_Log->Warn("Failed to set CUSTOM SEI side-data \n");
        }
    }

    if (m_setSEI) {
#ifdef FFMPEG_v42
        m_Log->Info("set sei at framenum=%d, sei_type=%d, sei_user_id=%d\n",
                     curEncFrames, m_SEIType, m_SEIUserId);
        if (!(m_SEIType & SEI_VALID_TYPE_ALL)) {
            m_Log->Warn("Failed to set sei, the sei type is invalid!\n");
        } else {
            AVSEI sei = {m_SEIType, m_SEIUserId};
            AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_SEI);
            if (NULL == fside) {
                fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_SEI, sizeof(AVSEI));
            }
            if (fside) {
                m_Log->Info("set SEI successfully!\n");
                memcpy(fside->data, &sei, sizeof(AVSEI));
            } else {
                m_Log->Warn("Failed to set SEI side-data \n");
            }
        }
#else
        m_Log->Warn("This ffmpeg version doesn't support setting dynamic SEI side data!\n");
#endif
        m_setSEI = false;
    }

#ifdef FFMPEG_v42
    if (m_setGopSize) {
        if (m_gopSize < 1) {
            m_Log->Warn("Failed to set gop size to %d, gop size should be greater than 0\n", m_gopSize);
        } else {
            m_Log->Info("set gop size at framenum=%d, new gop_size=%d\n", curEncFrames, m_gopSize);
            AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_GOP_SIZE);
            if (!fside) {
                fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_GOP_SIZE, sizeof(int));
            }
            if (fside) {
                memcpy(fside->data, &m_gopSize, sizeof(int));
                (*pFrameEnc)->pict_type = AV_PICTURE_TYPE_I;
                m_Log->Info("set gop size successfully!\n");
            } else {
                m_Log->Warn("Failed to set gop size side-data.\n");
            }
        }
        m_setGopSize = false;
    }
#endif

#ifdef FFMPEG_v42
    if (m_bSkipFrame) {
        updateFrameSkipped();
        AVFrameSideData *fside = av_frame_get_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_SKIP_FRAME);
        if (NULL == fside)
            fside = av_frame_new_side_data((*pFrameEnc), AV_FRAME_DATA_CONFIG_SKIP_FRAME, sizeof(int));
        if (fside) {
            memcpy(fside->data, &m_nFrameskipped, sizeof(int));
            m_Log->Debug("The skip_frames number is %d\n", m_nFrameskipped);
        }
        else {
            m_Log->Warn("Failed to set skip frame\n");
        }
    }
#endif

#ifdef ENABLE_TCAE
    if (m_tcaeEnabled && m_tcae)
    {
        uint32_t targetSize = m_tcae->GetTargetSize();
        //printf("TargetSize get from tcae is %d\n", targetSize);
        if (targetSize > 0 && m_qsvPlugin)
        {
            char value[32];
            snprintf(value, sizeof(value), "%d", targetSize);
            int ret = av_dict_set(&((*pFrameEnc)->metadata), "tcbrc_target_size",
                value, 0);
            if (ret < 0) {
                m_Log->Warn("Failed to set dict targetSize, ret=%d\n", ret);
            }
        }
    }
#endif
}

void CTransCoder::updateFrameSkipped() {
    if (m_bSkipFrame) {
        CStreamInfo *pDemuxInfo = nullptr;
        int strIdx = 0;
        pDemuxInfo = m_pDemux->getStreamInfo(strIdx);
        ///< 1 frame's time if const FPS
        int64_t frame_mcs = av_rescale_q(1, av_inv_q(pDemuxInfo->m_rFrameRate), pDemuxInfo->m_rTimeBase);
        int64_t delta = 0;

        struct timeval timeCurr;
        gettimeofday(&timeCurr, NULL);
        long long timestamp = (long long)(timeCurr.tv_sec) * 1000000 + (long long)timeCurr.tv_usec;
        m_curTimestampUS = timestamp;
        //The start point of 1 second period for calculating the skip frammes cycle when m_startTimestampUS is 0.
        //or if current time stamp is greater than the start time stamp 3 seconds, reset the start time, this mainly avoid in case
        //when encode received stop message then start again and the start time is not reset at before.
        if (m_startTimestampUS == 0 || (m_curTimestampUS - m_startTimestampUS) >= 3000000) {
            m_Log->Debug("in CTransCoder::updateFrameSkipped: the new start cycle m_startTimestampUS=%lld, m_curTimestampUS - m_startTimestampUS=%d, m_nTotalFrameskipped=%d\n",
                         m_startTimestampUS, m_curTimestampUS - m_startTimestampUS, m_nTotalFrameskipped);
            m_startTimestampUS = timestamp;
            //reset the encoded frame count
            m_nFrameEncoded = 0;
            m_nTotalFrameskipped = 0;
        }
        else {
            //A new encoded frame coming
            m_nFrameEncoded++;
        }


        delta = m_curTimestampUS - (m_startTimestampUS + m_nFrameEncoded * frame_mcs);
        m_nFrameskipped = (delta > 0) ? (delta / frame_mcs - m_nTotalFrameskipped) : 0;
        m_Log->Debug("in CTransCoder::updateFrameSkipped: m_curTimestampUS=%lld, m_startTimestampUS=%lld, encoded frames number: %d, delta / frame_mcs: %d, m_nTotalFrameskipped: %d, skip_frames number: %d\n",
                     m_curTimestampUS, m_startTimestampUS, m_nFrameEncoded, delta / frame_mcs, m_nTotalFrameskipped, m_nFrameskipped);
        m_nTotalFrameskipped = m_nTotalFrameskipped + m_nFrameskipped;

        // time period greater than or equal to 1 second, the calculating the skip frammes cycle end.
        if ((m_curTimestampUS - m_startTimestampUS) >= 1000000) {
            m_Log->Debug("in CTransCoder::updateFrameSkipped: the end cycle m_curTimestampUS - m_startTimestampUS=%d, m_nTotalFrameskipped=%d\n", m_curTimestampUS - m_startTimestampUS, m_nTotalFrameskipped);
            //reassignment the start time to current
            m_startTimestampUS = timestamp;
            //reset the encoded frame count
            m_nFrameEncoded = 0;
            m_nTotalFrameskipped = 0;
        }
    }
}

int CTransCoder::forceKeyFrame(int force) {
    m_forceKeyFrame = force;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_ForceKeyFrame_setting");
    return 0;
}

int CTransCoder::setQP(int qp) {
    m_setQP = qp;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_QP_setting");
    return 0;
}

int CTransCoder::setBitrate(int bitrate) {
    m_setBitrate = bitrate;
    m_bitrate    = bitrate;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_Bitrate_setting");
    return 0;
}

int CTransCoder::setMaxBitrate(int max_bitrate) {
    m_setMaxBitrate = max_bitrate;
    m_maxBitrate    = max_bitrate;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_MaxBitrate_setting");
    return 0;
}


int CTransCoder::setFramerate(float framerate) {
    m_setFramerate = framerate;
    m_frameRate    = framerate;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_Framerate_setting");
    return 0;
}

int CTransCoder::setMaxFrameSize(int size) {
    m_setMaxframeSize = size;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_MaxFrameSize_setting");
    return 0;
}

int CTransCoder::setRollingIntraRefresh(int type, int cycle_size, int qp_delta) {
    m_setRIR          = true;
    m_IntRefType      = type;
    m_IntRefCycleSize = cycle_size;
    m_IntRefQPDelta   = qp_delta;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_RollingIntraRefresh_setting");
    return 0;
}

#ifdef FFMPEG_v42
int CTransCoder::setRegionOfInterest(int roi_num, AVRoI roi_para[]) {
    if(roi_num <= 0 || roi_num > MAX_ROI_NUM) {
        m_Log->Warn("the number of ROI regions must be positive and less than %d\n!", MAX_ROI_NUM);
        return 0;
    }
    m_numROI = roi_num;
    CDecoder    *pDec     = m_mDecoders[0];
    CStreamInfo *pDecInfo = pDec->getDecInfo();
    int width  = pDecInfo->m_pCodecPars->width;
    int height = pDecInfo->m_pCodecPars->height;

    for(int loop = 0; loop < m_numROI; loop++) {
        if(roi_para[loop].x < 0      || roi_para[loop].y < 0 ||
           roi_para[loop].width <= 0 || roi_para[loop].height <= 0) {
            m_Log->Warn("the position parameters of ROI can NOT be negative!\n");
            return 0;
        }
        if(roi_para[loop].x + roi_para[loop].width > width ||
           roi_para[loop].y + roi_para[loop].height > height ) {
            m_Log->Warn("the ROI region can NOT beyond the resolution(%dx%d)!\n", width, height);
            return 0;
        }
        if(roi_para[loop].roi_value < -51 || roi_para[loop].roi_value > 51) {
            m_Log->Warn("the ROI value can NOT be out of range [-51, 51]!\n");
            return 0;
        }
        m_roiPara[loop].x         = roi_para[loop].x;
        m_roiPara[loop].y         = roi_para[loop].y;
        m_roiPara[loop].width     = roi_para[loop].width;
        m_roiPara[loop].height    = roi_para[loop].height;
        m_roiPara[loop].roi_value = roi_para[loop].roi_value;
    }

    m_setROI = true;

    return 0;
}
#endif

int CTransCoder::setMinMaxQP(int min_qp, int max_qp) {
    m_setMinMaxQP = 1;
    m_minQP       = min_qp;
    m_maxQP       = max_qp;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_MinMaxQP_setting");
    return 0;
}

int CTransCoder::changeResolution(int width, int height) {
    if( width < RESOLUTION_WIDTH_MIN   || width > RESOLUTION_WIDTH_MAX ||
        height < RESOLUTION_HEIGHT_MIN || height > RESOLUTION_HEIGHT_MAX) {
        m_Log->Warn("the resolution can NOT be changed to %dx%d, constraints: width %d-%d, height %d-%d.\n",
                     width, height, RESOLUTION_WIDTH_MIN, RESOLUTION_WIDTH_MAX, RESOLUTION_HEIGHT_MIN, RESOLUTION_HEIGHT_MAX);
            return -1;
    }

    m_isResolutionChange = true;
    m_newWidth  = width;
    m_newHeight = height;
    return 0;
}

int CTransCoder::changeCodec(AVCodecID codec_type) {
    m_Log->Info("CTransCoder::changeCodec m_nCodecId= %d, codec_type %d.\n",
        m_nCodecId, codec_type);
    if (codec_type != AV_CODEC_ID_H264 && codec_type != AV_CODEC_ID_HEVC) {
        m_Log->Warn("CTransCoder::changeCodec: codec_type must be AV_CODEC_ID_H264 or AV_CODEC_ID_HEVC.\n");
        return -1;
    }

    if (m_nCodecId != codec_type) {
        m_nCodecId = codec_type;
        m_isCodecChange = true;
        setOutputProp("profile", nullptr);
        setOutputProp("level", nullptr);
        if (m_nCodecId == AV_CODEC_ID_H264) {
            if (m_vaapiPlugin) {
                setOutputProp("c", "h264_vaapi");
            } else if (m_qsvPlugin) {
                setOutputProp("c", "h264_qsv");
            }
        } else if (m_nCodecId == AV_CODEC_ID_HEVC) {
            if (m_vaapiPlugin) {
                setOutputProp("c", "hevc_vaapi");
            } else if (m_qsvPlugin) {
                setOutputProp("c", "hevc_qsv");
            }
        } else if (m_nCodecId == AV_CODEC_ID_AV1) {
            if (m_vaapiPlugin) {
                setOutputProp("c", "av1_vaapi");
            } else if (m_qsvPlugin) {
                setOutputProp("c", "av1_qsv");
            }

        }
        return 0;
    }
    else {
        m_Log->Warn("CTransCoder::changeCodec: codec_type is same as the current m_nCodecId, codec_type=%d\n", codec_type);
    }

    return 0;
}

int CTransCoder::setSEI(int sei_type, int sei_user_id) {
    m_setSEI           = true;
    m_SEIType          = sei_type;
    m_SEIUserId        = sei_user_id;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_SEI_setting");
    return 0;
}

int CTransCoder::setGopSize(int size) {
    if (size < 1) {
        m_Log->Warn("the gop_size should be greater to 0.\n");
        return -1;
    }

    if (size == m_gopSize) {
        m_Log->Info("the current gop_size is already %d.\n", m_gopSize);
        return 0;
    }

    m_setGopSize       = true;
    m_gopSize          = size;
    m_setDynamicEncode = true;
    m_DyEncodeTimeLog->begin("IRRB_Dynamic_Encode_gop_size_setting");

    return 0;
}

int CTransCoder::getInitGopSize(AVDictionary *dict) {
    int gop_size = 0;
    if (dict) {
        AVDictionaryEntry *tag = NULL;
        tag = av_dict_get(dict, "g", NULL, 0);

        if (tag)
            gop_size = atoi(tag->value);
    }

    return gop_size;
}

int CTransCoder::getInitBitrate(AVDictionary *dict) {
    int bitrate = 0;
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(dict, "b", NULL, 0);

    if (tag) {
        double b = atof(tag->value);
        char c = tag->value[strlen(tag->value) - 1];
        if (c == 'M') {
            b = b * 1000000;
        } else if ( c == 'K') {
            b = b * 1000;
        }
        bitrate = b;
    }

    return bitrate;
}

int CTransCoder::getMaxBitrate(AVDictionary *dict) {
    int bitrate = 0;
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(dict, "maxrate", NULL, 0);

    if (tag) {
        double b = atof(tag->value);
        char c = tag->value[strlen(tag->value) - 1];
        if (c == 'M') {
            b = b * 1000000;
        } else if ( c == 'K') {
            b = b * 1000;
        }
        bitrate = b;
    }

    return bitrate;

}

void CTransCoder::setSkipFrameFlag(bool bSkipFrame) {
    m_bSkipFrame = bSkipFrame;
}

bool CTransCoder::getSkipFrameFlag() {
    return m_bSkipFrame;
}

void CTransCoder::setScreenCaptureFlag(bool bAllowCapture) {
    m_screenCaptureFlag = bAllowCapture;
}

void CTransCoder::setScreenCaptureInterval(int capture_interval) {
    m_screenCaptureInterval = capture_interval;
}

void CTransCoder::setScreenCaptureQuality(int qualityFactor) {
    if (qualityFactor < 1 || qualityFactor > 100) {
        m_Log->Info("Invalid quality value %d (must be in 1-100), use default quality %d.\n",
                     qualityFactor, DEFAULT_SCREEN_CAPTURE_QUALITY);
        m_screenCaptureQuality = DEFAULT_SCREEN_CAPTURE_QUALITY;
        return;
    }

    m_screenCaptureQuality = qualityFactor;
}

void CTransCoder::setIOStreamWriter(IOStreamWriter *writer) {
    m_pMux->setIOStreamWriter(writer);
    m_pIOStreamWriter = writer;
}

void CTransCoder::set_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                           int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop) {
    m_crop.client_rect_right = client_rect_right;
    m_crop.client_rect_bottom = client_rect_bottom;
    m_crop.fb_rect_right = fb_rect_right;
    m_crop.fb_rect_bottom = fb_rect_bottom;
    m_crop.crop_top = crop_top;
    m_crop.crop_bottom = crop_bottom;
    m_crop.crop_left = crop_left;
    m_crop.crop_right = crop_right;
    m_crop.valid_crop = valid_crop;
}

void CTransCoder::erase_encoders() {
    if( m_mEncoders.size() ) {
        std::map<int, CEncoder *>::iterator it;
        for(it = m_mEncoders.begin(); it != m_mEncoders.end(); ) {
            delete it->second;
            m_mEncoders.erase(it++);
        }
    }
}

void CTransCoder::erase_filters() {
    if( m_mFilters.size() ) {
        std::map<int, CFilter *>::iterator it;
        for(it = m_mFilters.begin(); it != m_mFilters.end();) {
            delete it->second;
            m_mFilters.erase(it++);
        }
    }
}

int CTransCoder::setLatencyStats(int nLatencyStats) {
    printf("transcoder nLatencyStats=%d\n", nLatencyStats);
    m_nLatencyStats = nLatencyStats;
    if(m_nLatencyStats) {
#if 0
        if (m_mProfTimer.find("transcode") == m_mProfTimer.end()) {
            m_mProfTimer["transcode"] = new ProfTimer(true);
        }
        m_mProfTimer["transcode"]->setPeriod(nLatencyStats);
        m_mProfTimer["transcode"]->enableProf();
#endif
        if (m_mProfTimer.find("cycle_trans") == m_mProfTimer.end()) {
            m_mProfTimer["cycle_trans"] = new ProfTimer(true);
        }
        m_mProfTimer["cycle_trans"]->setPeriod(nLatencyStats);
        m_mProfTimer["cycle_trans"]->enableProf();
        m_mProfTimer["cycle_trans"]->profTimerBegin();

    }else{
#if 0
        if(m_mProfTimer.find("transcode")!= m_mProfTimer.end()){
            m_mProfTimer["transcode"]->profTimerReset("transcode");
        }
#endif
        if(m_mProfTimer.find("cycle_trans")!= m_mProfTimer.end()){
            m_mProfTimer["cycle_trans"]->profTimerReset("cycle_trans");
        }
    }

    //Encoder
    for (auto it:m_mFilters) {
        int idx = it.first;
        CFilter *pFilt = it.second;
        if (m_mEncoders.find(idx) != m_mEncoders.end()) {
            CStreamInfo *pSinkInfo = pFilt->getSinkInfo();
            if(pSinkInfo->m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_mEncoders[idx]->setLatencyStats(m_nLatencyStats);
                break;
            }
        }
    }

    return 0;
}

int CTransCoder::initHwFramesCtx() {
    int ret = 0;

    if (nullptr == m_phw_frames_ctx) {
        CStreamInfo *pDemuxInfo;
        pDemuxInfo = m_pDemux->getStreamInfo(0);

        AVBufferRef *hw_device_ctx = nullptr;
#ifdef ENABLE_QSV
        if (m_qsvPlugin)
            hw_device_ctx = CQSVAPIDevice::getInstance()->getQSVapiDev();
        else
#endif
            hw_device_ctx = CVAAPIDevice::getInstance()->getVaapiDev();

        if (hw_device_ctx == NULL) {
            m_Log->Error("Failed to create a VAAPI/QSV device. Error code: %d\n", ret);
            return -1;
        }
        /* set hw_frames_ctx for TransCoder's AVCodecContext */
        AVBufferRef *hw_frames_ref;
        AVHWFramesContext *frames = NULL;

        if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
            m_Log->Error("Failed to create VAAPI frame context.\n");
            return -1;
        }
        frames = (AVHWFramesContext *)(hw_frames_ref->data);
        frames->format = m_qsvPlugin ? (AVPixelFormat)AV_PIX_FMT_QSV : (AVPixelFormat)AV_PIX_FMT_VAAPI;
        frames->sw_format = m_qsvPlugin ? (AVPixelFormat)AV_PIX_FMT_BGRA : (AVPixelFormat)AV_PIX_FMT_RGBA;
        frames->width = pDemuxInfo->m_pCodecPars->width;
        frames->height = pDemuxInfo->m_pCodecPars->height;
        frames->initial_pool_size = m_qsvPlugin ? 4 : 0;
        m_Log->Debug("frames, format=%d, sw_format =%d, size %dx%d\n",
            frames->format,
            frames->sw_format,
            frames->width,
            frames->height);
        if ((ret = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
            m_Log->Error("Failed to initialize VAAPI/QSV frame context."
                "Error code: %d\n", ret);
            return ret;
        }
        m_phw_frames_ctx = av_buffer_ref(hw_frames_ref);
        if (!m_phw_frames_ctx)
            ret = AVERROR(ENOMEM);

        av_buffer_unref(&hw_frames_ref);
        if (ret < 0) {
            m_Log->Error("Failed to set hwframe context.\n");
            av_buffer_unref(&hw_device_ctx);
            return ret;
        }
    }
    return ret;
}

static void freeHwFrame(void *opaque, uint8_t *data) {
    AVFrame *hw_frame = (AVFrame *)opaque;
    if (hw_frame) {
        av_frame_free(&hw_frame);
        printf("free buffer %p\n", hw_frame);
    }
}

AVBufferRef* CTransCoder::createHwFrameBuffer(int size, VASurfaceID *vaSurfaceId) {
    if (!m_phw_frames_ctx) {
        m_Log->Error("CTransCoder::createHwFrameBuffer: no hw_frames_ctx\n");
        return nullptr;
    }

    int ret = 0;
    AVFrame *sw_frame = nullptr, *hw_frame = nullptr;
    //AVHWFramesContext *hwfc = nullptr;

    if (!(hw_frame = av_frame_alloc())) {
        ret = AVERROR(ENOMEM);
        m_Log->Error("CTransCoder::createHwFrameBuffer: Failed to alloc a hw AVFrame");
        goto close;
    }
    if ((ret = av_hwframe_get_buffer(m_phw_frames_ctx, hw_frame, 0)) < 0) {
        m_Log->Error("CTransCoder::createHwFrameBuffer: Failed to get a hwframe buffer, error code: %d.\n", ret);
        goto close;
    }
    if (!hw_frame->hw_frames_ctx) {
        ret = AVERROR(ENOMEM);
        m_Log->Error("CTransCoder::createHwFrameBuffer: No hw frame context");
        goto close;
    }

    if (!(sw_frame = av_frame_alloc())) {
        ret = AVERROR(ENOMEM);
        m_Log->Error("CTransCoder::createHwFrameBuffer: Failed to alloc a sw AVFrame");
        goto close;
    }
#if 0
    hwfc = (AVHWFramesContext*)(hw_frame->hw_frames_ctx->data);
    m_Log->Info("hw_frames_ctx, format=%d, sw_format=%d, %dx%d, poolsize=%d\n",
        hwfc->format,
        hwfc->sw_format,
        hwfc->width,
        hwfc->height,
        hwfc->initial_pool_size);
#endif
    ret = av_hwframe_map(sw_frame, hw_frame, AV_HWFRAME_MAP_WRITE | AV_HWFRAME_MAP_OVERWRITE);

    if (ret) {
        m_Log->Error("CTransCoder::createHwFrameBuffer: Failed to map frame into derived "
            "frame context: %d.\n", ret);
        goto close;
    }

#if 0
    Info("++++++ECTransCoder::createHwFrameBuffer: hwfc =%p\n", hwfc);
    Info("++++++ECTransCoder::createHwFrameBuffer: hwframe =%p\n", hw_frame);

    Info("++++++ECTransCoder::createHwFrameBuffer: hwframe data[0]=%p\n", hw_frame->data[0]);
    Info("++++++ECTransCoder::createHwFrameBuffer: hwframe data[1]=%p\n", hw_frame->data[1]);
    Info("++++++ECTransCoder::createHwFrameBuffer: hwframe data[2]=%p\n", hw_frame->data[2]);
    Info("++++++ECTransCoder::createHwFrameBuffer: hwframe data[3]=%p\n", hw_frame->data[3]);
    Info("++++++ECTransCoder::createHwFrameBuffer: hwframe buf[0]=%p\n", hw_frame->buf[0]);

    Info("++++++ECTransCoder::createHwFrameBuffer: swframe data[0]=%p\n", sw_frame->data[0]);
    Info("++++++ECTransCoder::createHwFrameBuffer: swframe data[1]=%p\n", sw_frame->data[1]);
    Info("++++++ECTransCoder::createHwFrameBuffer: swframe data[2]=%p\n", sw_frame->data[2]);
    Info("++++++ECTransCoder::createHwFrameBuffer: swframe data[3]=%p\n", sw_frame->data[3]);
#endif

    AVBufferRef *ref;
    ref = av_buffer_create(sw_frame->data[0], size, freeHwFrame, hw_frame, 0);

    if (!ref) {
        m_Log->Error("CTransCoder::createHwFrameBuffer: Failed to create av buffer\n");
        goto close;
    }

    *vaSurfaceId = (VASurfaceID)(uintptr_t)hw_frame->data[3];

    return ref;

close:
    if (hw_frame)
        av_frame_free(&hw_frame);
    if (sw_frame)
        av_frame_free(&sw_frame);
    return nullptr;
}

#ifdef ENABLE_MEMSHARE
AVBufferRef* CTransCoder::createAvBuffer(int size) {
    //Encoder
    for (auto it:m_mFilters) {
        int idx = it.first;
        CFilter *pFilt = it.second;
        if (m_mEncoders.find(idx) != m_mEncoders.end()) {
            CStreamInfo *pSinkInfo = pFilt->getSinkInfo();
            if(pSinkInfo->m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO) {
                //find encoder
                return m_mEncoders[idx]->createAvBuffer(size);

            }
        }
    }
    return nullptr;
}

void CTransCoder::set_hw_frames(uint8_t *data, AVFrame *hw_frame) {
    m_mHwFrames[data] = hw_frame;
    return;
}

void CTransCoder::set_hw_frames_ctx(AVBufferRef *hw_frames_ctx) {
    m_hw_frames_ctx = hw_frames_ctx;
    return;
}

#endif


void CTransCoder::updateDynamicChangedFramerate(int idx) {
    ((CFFFilter*)m_mFilters[idx])->updateDynamicChangedFramerate((int)m_frameRate);
    ((CFFDecoder*)m_mDecoders[idx])->updateDynamicChangedFramerate((int)m_frameRate);
    ((CFFEncoder*)m_mEncoders[idx])->updateDynamicChangedFramerate((int)m_frameRate);
    m_isFramerateChange = false;
}

void CTransCoder::setVideoMode(bool isAlpha) {
    m_isAlphaChannelMode = isAlpha;
}

void CTransCoder::setFrameBufferSize(int width, int height) {
    m_buffer_width = width;
    m_buffer_height = height;
}

int CTransCoder::getEncodeNewWidth() {
    return m_newWidth;
}

int CTransCoder::getEncodeNewHeight() {
    return m_newHeight;
}

int CTransCoder::changeEncoderProfileLevel(const int iProfile, const int iLevel) {
    std::string strProfileName = "";
    std::string strLevelName = "";
    m_mEncoders[0]->getProfileNameByValue(strProfileName, iProfile);
    m_mEncoders[0]->getLevelNameByValue(strLevelName, iLevel);
    if (strProfileName == "" && strLevelName == "") {
        m_Log->Info("CTransCoder::changeEncoderProfileLevel: both the change profile and level are not support: iProfile=%d, iLevel=%d.\n", iProfile, iLevel);
        return -1;
    }

    if (strProfileName != "") {
        setOutputProp("profile", strProfileName.c_str());
        m_Log->Info("CTransCoder::changeEncoderProfileLevel: change profile = %s.\n", strProfileName.c_str());
    }

    if (strLevelName != "") {
        setOutputProp("level", strLevelName.c_str());
        m_Log->Info("CTransCoder::changeEncoderProfileLevel: change level = %s.\n", strLevelName.c_str());
    }

    m_isProfileLevelChange = true;

    return 0;
}

#ifdef ENABLE_TCAE
int CTransCoder::setClientFeedback(unsigned int delay, unsigned int size) {
    if (m_tcaeEnabled && m_tcae)
    {
        return m_tcae->UpdateClientFeedback(delay, size);
    }
    return 0;
}
#endif

bool CTransCoder::enableTcae(const char* tcaeLogPath)
{
#ifdef ENABLE_TCAE
    m_tcae = new CTcaeWrapper();
    if (m_tcae)
    {
        uint32_t maxSize = 0;
        uint32_t maxBitrate = getMaxBitrate(m_pOutProp);
        // get init frame rate
        const char *pVal = getOutOptVal("r", "framerate");
        float framerate = 0.0;
        if (pVal)
            framerate = atof(pVal);
        if (maxBitrate > 0 && framerate > 1e-6)
        {
            maxSize = maxBitrate / 8 / framerate;
        }

        if (tcaeLogPath)
            m_tcae->setTcaeLogPath(tcaeLogPath);

        int ret = m_tcae->Initialize(100, maxSize); // 100ms latency as results of experiments
        if (ret < 0)
        {
            delete m_tcae;
            m_tcae = nullptr;
        }
        else
        {
            m_tcaeEnabled = true;
            if (tcaeLogPath)
                m_tcaeLogPath = m_tcaeLogPath;
        }
    }
    return m_tcaeEnabled;
#else
    return false;
#endif
}



void CTransCoder::setRenderFpsEncFlag(bool bRenderFpsEnc) {
    m_pDemux->setRenderFpsEncFlag(bRenderFpsEnc);
    return;
}

bool CTransCoder::getRenderFpsEncFlag(void) {
    return m_pDemux->getRenderFpsEncFlag();
}

