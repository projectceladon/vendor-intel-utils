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

#ifndef CDEMUX_H
#define CDEMUX_H

#include "data_types.h"
#include "CStreamInfo.h"
#include <va/va.h>

struct IrrPacket {
  AVPacket av_pkt;
  std::unique_ptr<vhal::client::display_control_t> display_ctrl;
};

class CDemux {
public:
    CDemux() :m_isVASurfaceID(false), m_bLatencyOpt(true), m_bRenderFpsEnc(true), m_MinFpsEnc(3), m_isQSVSurfaceID(false), m_bFirstStartEncoding(false), m_iFirstStartEncodingCnt(0){}
    virtual ~CDemux() {}
    /**
     * @param format,pDict Extra informations for demux
     * @return 0 for success.
     */
    virtual int start(const char *format = nullptr, AVDictionary *pDict = nullptr) { return 0; }
    /**
     * @return the number of streams on succuss;
     * otherwise a negative value.
     */
    virtual int getNumStreams() { return 0; }
    /**
     * @return the AVCodecPar of the streams[idx] on succuss,
     * otherwise null pointer.
     */
    virtual CStreamInfo* getStreamInfo(int strIdx) { return nullptr; }
    /**
     * @return 0 if OK, otherwise a negative value.
     */
    virtual int readPacket(IrrPacket *pkt) { return 0; }
    /**
     * The same as fseek/ftell, only workable when file streams.
     */
    virtual int seek(long offset, int whence) { return 0; }
    virtual int tell() { return 0; }
    virtual int size() { return 0; }
    virtual void setVASurfaceFlag(bool bVASurfaceID) { m_isVASurfaceID = bVASurfaceID; }
    virtual bool getVASurfaceFlag() { return m_isVASurfaceID; }
    virtual void setQSVSurfaceFlag(bool bQSVSurfaceID) { m_isQSVSurfaceID = bQSVSurfaceID; }
    virtual bool getQSVSurfaceFlag() { return m_isQSVSurfaceID; }
    virtual void setLatencyOptFlag(bool bLatencyOpt) { m_bLatencyOpt = bLatencyOpt; }
    virtual bool getLatencyOptFlag() { return m_bLatencyOpt; }
    virtual void setRenderFpsEncFlag(bool bRenderFpsEnc) { m_bRenderFpsEnc = bRenderFpsEnc; }
    virtual bool getRenderFpsEncFlag() { return m_bRenderFpsEnc; }
    virtual void setMinFpsEnc(int iMinFpsEnc) { m_MinFpsEnc = iMinFpsEnc; }
    virtual int  getMinFpsEnc() { return m_MinFpsEnc; }
    virtual void  setFirstStartEncoding(bool bFirstStartEncoding) { m_bFirstStartEncoding = bFirstStartEncoding; }

    virtual void stop() { }
private:
    bool m_isVASurfaceID;
    bool m_bLatencyOpt;
    bool m_bRenderFpsEnc;
    int  m_MinFpsEnc;
    bool m_isQSVSurfaceID;

protected:
    bool m_bFirstStartEncoding;
    int  m_iFirstStartEncodingCnt;
};

#endif /* CDEMUX_H */
