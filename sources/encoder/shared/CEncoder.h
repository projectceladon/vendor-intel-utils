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

#ifndef CENCODER_H
#define CENCODER_H

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
}

#include "CStreamInfo.h"
#include <string>

using namespace std;

class CEncoder {
public:
    CEncoder() {}
    virtual ~CEncoder() {}
    virtual int write(AVFrame *frame) { return 0; }
    virtual int read(AVPacket *pkt) { return 0; }
    virtual CStreamInfo *getStreamInfo() { return 0; }
#ifdef ENABLE_MEMSHARE
    virtual AVBufferRef* createAvBuffer(int size) {return nullptr;}
    virtual void getHwFrame(const void *data, AVFrame **hw_frame) { return; };
#endif
    /**
     * enable latency statistic.
     */
    virtual int setLatencyStats(int nLatencyStats) {return 0;}
    virtual size_t getEncFrames() {return 0;}

    virtual void getProfileNameByValue(std::string &strProfileName, const int iProfileValue) { return; }
    virtual void getLevelNameByValue(std::string &strLevelName, const int iLevelValue) { return; }
};

#endif /* CENCODER_H */

