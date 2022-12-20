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

#include "CRemoteMux.h"


CRemoteMux::CRemoteMux(void* opaque,
        rmOpen pOpen, rmWrite pWrite, rmClose pClose,
        rmWritePacket pWritePkt, rmWriteHead pWriteHead)
: CTransLog(__func__), m_Opaque(opaque), m_pOpen(pOpen), m_pWrite(pWrite),
    m_pClose(pClose), m_bInited(false)
{
    m_pWritePkt = pWritePkt;
    m_pWriteHead = pWriteHead;
    m_frameInfo = NULL;
}

CRemoteMux::~CRemoteMux() {
    if (m_pClose)
        m_pClose(m_Opaque);
    if (m_frameInfo) {
        av_freep(&m_frameInfo);
    }
}

int CRemoteMux::addStream(int, CStreamInfo* info) {
    int ret = 0;
    ///< TODO: Deal with multi-channel streams.
    if (m_bInited || info->m_pCodecPars->codec_type != AVMEDIA_TYPE_VIDEO) {
        Warn("Do not support multi-streams or non-video streams so far.\n");
        return AVERROR(EINVAL);
    }

    if(m_frameInfo) {
        // free it first
        av_freep(m_frameInfo);
        m_frameInfo = NULL;
    }
    if(!m_frameInfo) {
        int size = sizeof(frameInfo);

        // send extradata
        if(info->m_pCodecPars->extradata_size) {
            size += (info->m_pCodecPars->extradata_size);
        }

        m_frameInfo = (frameInfo *)av_mallocz(size);
        if(!m_frameInfo) {
            Error("Fail to alloc frameinfo.\n");
            return -1;
        }
    }

    ret = avcodec_parameters_copy(&(m_frameInfo->codecpar), info->m_pCodecPars);
    if (ret < 0) {
        Error("Fail to copy stream infos. Msg: %s\n", ErrToStr(ret).c_str());
        return ret;
    }

    m_frameInfo->timebase = info->m_rTimeBase;
    // send extradata
    if(info->m_pCodecPars->extradata && info->m_pCodecPars->extradata_size) {
        memcpy((uint8_t *)m_frameInfo+sizeof(frameInfo),
                info->m_pCodecPars->extradata,
                info->m_pCodecPars->extradata_size);
    }


    if (m_pOpen) {
        ret = m_pOpen(m_Opaque, info->m_pCodecPars->width, info->m_pCodecPars->height,
                av_q2d(info->m_rFrameRate));
        if (ret < 0) {
            Error("Fail to call cb->open().\n");
            return ret;
        }
    }

    m_bInited = true;

    if (m_pWriteHead && m_frameInfo) {
        size_t head_size = sizeof(AVCodecParameters)+sizeof(AVRational);
        head_size += m_frameInfo->codecpar.extradata_size;
        ret = m_pWriteHead(m_Opaque, (void*)m_frameInfo,
                head_size);
        if (ret < 0) {
            Error("Fail to write back head by calling cb->writeHead(). EC %d.\n", ret);
            return ret;
        }
    }

    return 0;
}

int CRemoteMux::write(AVPacket *pPkt) {
    int ret = 0;
    if (!pPkt)  ///< Do not support flush
        return 0;

    if (m_pWrite) {
        ret = m_pWrite(m_Opaque, pPkt->data, static_cast<size_t>(pPkt->size));
        if (ret < 0) {
            Error("Fail to write back by calling cb->write(). EC %d.\n", ret);
            return ret;
        }
    } else if (m_pWritePkt) {
        ret = m_pWritePkt(m_Opaque, pPkt->data, static_cast<size_t>(pPkt->size), pPkt);
        if (ret < 0) {
            Error("Fail to write back by calling cb->writePacket(). EC %d.\n", ret);
            return ret;
        }
    }

    return ret;
}
