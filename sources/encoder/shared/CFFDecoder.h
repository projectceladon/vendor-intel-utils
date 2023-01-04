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

#ifndef CFFDECODER_H
#define CFFDECODER_H

#include "ffmpeg_compat.h"
#include "utils/CTransLog.h"
#include "CDecoder.h"
#include "CVAAPIDevice.h"
#include "CQSVAPIDevice.h"


class CFFDecoder : public CDecoder, private CTransLog {
public:
    CFFDecoder(CStreamInfo *info, AVDictionary *pDict = nullptr);
    ~CFFDecoder();
    int write(AVPacket *pkt);
    int write(uint8_t *data, int len);
    AVFrame* read(void);
    int getNumFrames(void);
    CStreamInfo* getDecInfo();
    void updateDynamicChangedFramerate(int framerate);

private:
    CFFDecoder(const CFFDecoder&) = delete;
    CFFDecoder() = delete;
    CFFDecoder &operator= (const CFFDecoder&) = delete;
    AVCompat_AVCodec *FindDecoder(AVCodecID id);
    static AVPixelFormat get_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts);

private:
    AVCodecContext *m_pCodec;
    size_t          m_nFrames;
    CStreamInfo     m_Info;
    bool            m_bInited;
};

#endif /* CFFDECODER_H */
