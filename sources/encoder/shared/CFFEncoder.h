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

#ifndef CFFENCODER_H
#define CFFENCODER_H

#include "CEncoder.h"
#include "utils/CTransLog.h"
#include "CStreamInfo.h"

#include <map>
#include "utils/ProfTimer.h"
#include "utils/TimeLog.h"

typedef enum {
    VA_ENCODER = 1,
    QSV_ENCODER = 2,
} EncodePluginType;

class CFFEncoder : public CEncoder, private CTransLog {
public:
    CFFEncoder(const char *pCodec, CStreamInfo *info, EncodePluginType plugin);
    CFFEncoder(AVCodecID id, CStreamInfo *info);
    void init(AVDictionary *pDict);
    ~CFFEncoder();
    int write(AVFrame *);
    int read(AVPacket *);
    CStreamInfo *getStreamInfo();
    static int GetBestFormat(const char *codec, int format);
    /**
     * enable latency statistic.
     */
    int setLatencyStats(int nLatencyStats);
    size_t getEncFrames() { return m_nFrames; }
    void updateDynamicChangedFramerate(int framerate);

#ifdef ENABLE_MEMSHARE
    //AVBufferRef* createAvBuffer(int size);
    //int set_hwframe_ctx(AVBufferRef *hw_device_ctx);
    void getHwFrame(const void *data, AVFrame **hw_frame);
    void set_hw_frames(const void *data, AVFrame *hw_frame);
    void set_hw_frames_ctx(AVBufferRef *hw_frames_ctx);
    void hw_frames_ctx_backup();
    void hw_frames_ctx_recovery();

#endif

    void getProfileNameByValue(std::string &strProfileName, const int iProfileValue);
    void getLevelNameByValue(std::string &strLevelName, const int iLevelValue);
private:
    AVCodecContext *m_pEnc;
    bool            m_bInited;
    size_t          m_nFrames;
    AVDictionary   *m_pDict;
    CStreamInfo     m_Info;
    EncodePluginType m_plugin;

private:
    AVCompat_AVCodec *FindEncoder(AVCodecID id);

private:
    //ProfTimer *mProfTimer;
    std::map<std::string, ProfTimer *> m_mProfTimer;
    int m_nLatencyStats;
#ifdef ENABLE_MEMSHARE
    std::map<const void*, AVFrame *> m_mHwFrames;
    AVBufferRef *m_hw_frames_ctx_bk;
#endif
	long m_g_encode_write_count;
};

#endif /* CFFENCODER_H */
