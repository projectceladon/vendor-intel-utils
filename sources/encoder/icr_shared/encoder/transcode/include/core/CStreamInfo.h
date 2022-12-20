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

#ifndef CSTREAMINFO_H
#define CSTREAMINFO_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

struct CStreamInfo {
    CStreamInfo() {
        m_pCodecPars = avcodec_parameters_alloc();
        m_rFrameRate = (AVRational) {0, 1};
        m_rTimeBase  = AV_TIME_BASE_Q;
    }
    ~CStreamInfo() {
        avcodec_parameters_free(&m_pCodecPars);
    }
    CStreamInfo(const CStreamInfo &orig) {
        m_pCodecPars = avcodec_parameters_alloc();
        avcodec_parameters_copy(m_pCodecPars, orig.m_pCodecPars);
        m_rFrameRate = orig.m_rFrameRate;
        m_rTimeBase  = orig.m_rTimeBase;
    }

    CStreamInfo& operator=(const CStreamInfo &orig) {
        avcodec_parameters_copy(m_pCodecPars, orig.m_pCodecPars);
        m_rFrameRate = orig.m_rFrameRate;
        m_rTimeBase  = orig.m_rTimeBase;
        return *this;
    }

    AVCodecParameters *m_pCodecPars;
    AVRational         m_rFrameRate;
    AVRational         m_rTimeBase;
};

#endif /* CSTREAMINFO_H */

