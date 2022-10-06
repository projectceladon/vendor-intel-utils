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

#ifndef CCALLBACKMUX_H
#define CCALLBACKMUX_H

#include "CMux.h"
#include "CTransLog.h"
#include "utils/IOStreamWriter.h"
#include "utils/IORuntimeWriter.h"
#include <condition_variable>

typedef int (*cbOpen) (void *, int /*w*/, int /*h*/, float /*framerate*/);
typedef int (*cbWrite) (void *, uint8_t *, size_t, unsigned int);
typedef int (*cbWrite2) (void *, uint8_t *, size_t, int /* type */);
typedef void (*cbClose) (void *);
typedef int (*cbCheckNewConn) (void *);
typedef int (*cbSendMessage) (void *, int /*msg*/, unsigned int /*value*/);

class CCallbackMux : public CMux, private CTransLog {
public:
    CCallbackMux(void *opaque, void *opaque2, cbOpen pOpen, cbWrite pWrite,
                cbWrite2 pWrite2, cbClose pClose, cbCheckNewConn pCheckNewConn,
                cbSendMessage pSendMessage = nullptr);
    ~CCallbackMux();
    int addStream(int, CStreamInfo *);
    int write(AVPacket *);
    int write(uint8_t *data, size_t len, int type);
    int checkNewConn();
    int sendMessage(int msg, unsigned int value);
    bool isIrrv() { return true; }
    void setIOStreamWriter(const IOStreamWriter *writer) { m_pWriter = const_cast<IOStreamWriter *>(writer); }
    void setIORuntimeWriter(IORuntimeWriter::Ptr writer) { m_pRuntimeWriter = writer; }
    void setGetTransmissionAllowedFunc(getMuxerTransmissionAllowedFlag func) { getTransmissionAllowed = func;}
private:
    void       *m_Opaque;
    void       *m_Opaque2;
    cbOpen      m_pOpen;
    cbWrite     m_pWrite;
    cbWrite2    m_pWrite2;
    cbClose     m_pClose;
    cbCheckNewConn m_pCheckNewConn;
    cbSendMessage m_pSendMessage;
    bool        m_bInited = false;

    IOStreamWriter *m_pWriter = nullptr;
    IORuntimeWriter::Ptr m_pRuntimeWriter;
    getMuxerTransmissionAllowedFlag getTransmissionAllowed = nullptr;
};

#endif /* CCALLBACKMUX_H */

