// Copyright (C) 2017-2022 Intel Corporation
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

#ifndef CDECODER_H
#define CDECODER_H

#include <queue>
extern "C" {
#include <libavcodec/avcodec.h>
}
#include "CStreamInfo.h"

class CDecoder {
public:
    CDecoder() {}
    virtual ~CDecoder() {}

    /**
     * 1. NULL to flush
     * 2. 0 on success, negative on failure
     */
    virtual int write(AVPacket *avpkt) { return AVERROR(EINVAL); }
    virtual int write(uint8_t *data, int size) { return AVERROR(EINVAL); }

    virtual AVFrame* read(void) { return nullptr; }

    ///< Get the number of frames decoded.
    virtual int getNumFrames(void) { return 0; }
    virtual CStreamInfo* getDecInfo() { return nullptr; }
};

#endif /* CDECODER_H */
