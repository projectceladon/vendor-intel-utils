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

#ifndef CTCAEWRAPPER_H
#define CTCAEWRAPPER_H

#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <memory>
#include <stdio.h>
#include "enc_frame_settings_predictor.h"

class CTcaeWrapper
{
public:
    CTcaeWrapper();
    ~CTcaeWrapper();

    int Initialize(uint32_t targetDelay = 60, uint32_t maxFrameSize = 0);

    int UpdateClientFeedback(uint32_t delay, uint32_t size);

    int UpdateEncodedSize(uint32_t encodedSize);

    uint32_t GetTargetSize();

    void setTcaeLogPath(const char* path) { m_tcaeLogPath = path; };

    void makeLogEntry();

protected:
    std::unique_ptr<PredictorTcaeImpl> m_tcae;

    //User-provided path capturing extra logs in CSV format
    const char* m_tcaeLogPath = nullptr;
    FILE* m_logFilePtr = nullptr;

    uint32_t m_log_delay;
    uint32_t m_log_size;
    uint32_t m_log_encSize;
    uint32_t m_log_targetSize;
};



#endif /* CTCAEWRAPPER_H */
