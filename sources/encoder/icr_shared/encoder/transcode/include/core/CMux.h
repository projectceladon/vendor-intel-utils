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

#ifndef CMUX_H
#define CMUX_H

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <functional>
#include "CStreamInfo.h"
#include "utils/IOStreamWriter.h"

using getMuxerTransmissionAllowedFlag = std::function<bool(void)>;
class CMux {
public:
    CMux() {}
    virtual ~CMux() {}
    virtual int addStream(int, CStreamInfo *) { return 0; }
    virtual int write(AVPacket *) { return 0; }
    virtual int write(uint8_t *data, size_t size, int type) { return 0; }
    virtual bool isIrrv() { return false; }
    virtual int checkNewConn() { return 0; }
    virtual void setIOStreamWriter(const IOStreamWriter *writer) { /* empty */ }
    virtual void setGetTransmissionAllowedFunc(getMuxerTransmissionAllowedFlag func) { }
};

#endif /* CMUX_H */
