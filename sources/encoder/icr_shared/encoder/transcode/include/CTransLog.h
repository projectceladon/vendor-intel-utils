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

#ifndef CTRANSLOG_H
#define CTRANSLOG_H

#include <iostream>

class CTransLog {
public:
    enum LogLevel {
        LL_DEBUG,
        LL_VERBOSE,
        LL_INFO,
        LL_WARN,
        LL_ERROR,
        LL_NONE,
        LL_NUM
    };

    CTransLog(const char *name);
    ~CTransLog();
    static void SetLogLevel(LogLevel level);
    static void SetLogLevel(std::string logLevel);
    std::string ErrToStr(int ret);
    std::string TsToStr(int64_t ts);
    std::string TsToTimeStr(int64_t ts, int num, int den);
    void Verbose(const char *format, ...);
    void Debug(const char *format, ...);
    void Info(const char *format, ...);
    void Warn(const char *format, ...);
    void Error(const char *format, ...);

private:
    void *m_pClass;
};

#endif /* CTRANSLOG_H */

