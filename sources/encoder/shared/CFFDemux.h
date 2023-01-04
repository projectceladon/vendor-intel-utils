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

#ifndef CFFDEMUX_H
#define CFFDEMUX_H

extern "C" {
#include <libavformat/avformat.h>
}
#include "utils/CTransLog.h"
#include "CDemux.h"

/**
 * FFmpeg demuxers
 */
class CFFDemux : private CTransLog, public CDemux {
public:
    CFFDemux(const char *url);
    ~CFFDemux();
    int start(const char *format = nullptr, AVDictionary *pDict = nullptr);
    int getNumStreams();
    CStreamInfo* getStreamInfo(int strIdx);
    int readPacket(AVPacket *avpkt);
    int seek(long offset, int whence);
    int tell();
    int rewind();
    int size();

private:
    CFFDemux() = delete;
    CFFDemux(const CFFDemux& orig) = delete;
    CFFDemux &operator= (const CFFDemux&) = delete;
    int init(const char *url, const char *format, AVDictionary *pDict);

private:
    AVFormatContext *m_pDemux;
    std::map<int, CStreamInfo*> m_mStreamInfos;
    std::string m_sUrl;
};

#endif /* CFFDEMUX_H */

