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

#ifndef CFFMUX_H
#define CFFMUX_H

extern "C" {
#include <libavformat/avformat.h>
}

#include "CMux.h"
#include "utils/CTransLog.h"

class CFFMux : public CMux, private CTransLog {
public:
    CFFMux(const char *url, const char *format = nullptr);
    ~CFFMux();
    int write(AVPacket *pkt);
    int write(uint8_t *data, size_t size, int type) {return 0;};
    int addStream(int idx, CStreamInfo *info);

private:
    AVFormatContext   *m_pFmt;
    std::map<int, int> m_mIdxs;
    bool               m_bInited;
    std::map<int, CStreamInfo> m_mInfo;
    std::string        m_sUrl;
};

#endif /* CFFMUX_H */

