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

#include <map>

#include "ffmpeg_compat.h"
#include "CFFDemux.h"

using namespace std;

CFFDemux::CFFDemux(const char *url) : CTransLog(__func__) {
    m_pDemux = nullptr;
    m_sUrl   = url;
}

CFFDemux::~CFFDemux() {
    for (auto it:m_mStreamInfos)
        delete it.second;

    if (m_pDemux) {
        avformat_close_input(&m_pDemux);
    }
}

int CFFDemux::init(const char *url, const char *format, AVDictionary *pDict) {
    int                ret = 0;
    AVCompat_AVInputFormat *iformat = nullptr;

    if (format) {
        iformat = av_find_input_format(format);
        if (!iformat)
            Warn("Requested format %s is not found.\n", format);
    }

    ret = avformat_open_input(&m_pDemux, url, iformat, &pDict);
    if (ret < 0) {
        Error("Open input %s failure. Error: %s.\n", url, ErrToStr(ret).c_str());
        return ret;
    }

    ret = avformat_find_stream_info(m_pDemux, NULL);
    if (ret < 0) {
        Error("Find stream info for input %s failure. Error: %s.\n", url, ErrToStr(ret).c_str());
        return ret;
    }
    av_dump_format(m_pDemux, 0, url, 0);

    for (unsigned idx = 0; idx < m_pDemux->nb_streams; idx ++) {
        CStreamInfo *info = new CStreamInfo();
        avcodec_parameters_copy(info->m_pCodecPars, m_pDemux->streams[idx]->codecpar);
        info->m_rFrameRate = m_pDemux->streams[idx]->avg_frame_rate;
        info->m_rTimeBase  = m_pDemux->streams[idx]->time_base;
        m_mStreamInfos[idx] = info;
    }

    return 0;
}

int CFFDemux::start(const char *format, AVDictionary *pDict) {
    return init(m_sUrl.c_str(), format, pDict);
}

int CFFDemux::getNumStreams() {
    return m_pDemux ? m_pDemux->nb_streams : 0;
}

CStreamInfo* CFFDemux::getStreamInfo(int strIdx) {
    return m_mStreamInfos[strIdx];
}

int CFFDemux::readPacket(AVPacket* avpkt) {
    int ret;

    if (!m_pDemux)
        return AVERROR(EINVAL);

    ret = av_read_frame(m_pDemux, avpkt);
    if (ret < 0)
        return ret;

    if (m_mStreamInfos.find(avpkt->stream_index) == m_mStreamInfos.end()) {
        CStreamInfo *info = new CStreamInfo();
        int           idx = avpkt->stream_index;

        Warn("New stream is found.\n");

        avcodec_parameters_copy(info->m_pCodecPars, m_pDemux->streams[idx]->codecpar);
        info->m_rFrameRate = m_pDemux->streams[idx]->avg_frame_rate;
        info->m_rTimeBase  = m_pDemux->streams[idx]->time_base;
        m_mStreamInfos[avpkt->stream_index] = info;
    }

    Verbose("Demux -> Decoder[%d]: dts %s, time %s\n", avpkt->stream_index,
            TsToStr(avpkt->dts).c_str(),
            TsToTimeStr(avpkt->dts, m_mStreamInfos[avpkt->stream_index]->m_rTimeBase.num,
                     m_mStreamInfos[avpkt->stream_index]->m_rTimeBase.den).c_str());

    return ret;
}

int CFFDemux::seek(long offset, int whence) {
    return m_pDemux ? avio_seek(m_pDemux->pb, offset, whence) : AVERROR(EINVAL);
}

int CFFDemux::tell() {
    return m_pDemux ? avio_tell(m_pDemux->pb) : AVERROR(EINVAL);
}

int CFFDemux::size() {
    return m_pDemux ? avio_size(m_pDemux->pb) : AVERROR(EINVAL);
}
