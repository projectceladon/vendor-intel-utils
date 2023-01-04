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

#ifndef CRemoteMux_H
#define CRemoteMux_H

#include "CMux.h"
#include "utils/CTransLog.h"

typedef struct frameInfo {
    AVCodecParameters codecpar;
    AVRational timebase;
    //int extradata_size;
}frameInfo;

typedef int (*rmOpen) (void *, int /*w*/, int /*h*/, float /*framerate*/);
typedef int (*rmWrite) (void *, uint8_t *, size_t);
typedef void (*rmClose) (void *);
typedef int (*rmWriteHead) (void */*opaque*/, void */*frameInfo*/, size_t/*size*/);
typedef int (*rmWritePacket) (void */*opaque*/, uint8_t */*data*/, size_t/*size*/, AVPacket */*pkt*/);

class CRemoteMux : public CMux, private CTransLog {
    public:
        CRemoteMux(void *opaque,
                rmOpen pOpen,
                rmWrite pWrite,
                rmClose pClose,
                rmWritePacket pWritePkt,
                rmWriteHead pWriteHead);
        ~CRemoteMux();
        int addStream(int, CStreamInfo *);
        int write(AVPacket *);

    private:
        void       *m_Opaque;
        rmOpen      m_pOpen;
        rmWrite     m_pWrite;
        rmClose     m_pClose;
        rmWritePacket m_pWritePkt;
        rmWriteHead m_pWriteHead;
        bool        m_bInited;

//        AVCodecParameters *m_vCodecPar;
        frameInfo *m_frameInfo;
};

#endif /* CRemoteMux_H */

