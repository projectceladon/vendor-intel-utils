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

#ifndef CFILTER_H
#define CFILTER_H

extern "C" {
#include <libavfilter/avfilter.h>
}

#include "CStreamInfo.h"

class CFilter {
public:
    CFilter() {}
    virtual ~CFilter() {}
    virtual int push(AVFrame *frame) { return 0; }
    virtual AVFrame* pop(void) { return nullptr; }
    virtual int getNumFrames(void) { return 0; }
    virtual CStreamInfo *getSinkInfo() { return nullptr; }
    virtual CStreamInfo *getSrcInfo() { return nullptr; }
    virtual int getLastError() { return 0; }
    virtual void setLastError(int iErrorCode) { return; }
};

#endif /* CFILTER_H */

