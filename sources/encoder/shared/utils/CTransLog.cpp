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

#include <stdarg.h>
#include <map>
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
#include <libavutil/log.h>
}
#include "version.h"
#include "utils/CTransLog.h"

#if BUILD_FOR_HOST != 1
#include <android/log.h>
#define ANDROID_LOG_EN
#endif

using namespace std;

#ifdef ANDROID_LOG_EN
static void log_callback_ffmpeg(void *ptr, int level, const char *fmt, va_list vl) {
    (void)ptr;

    static const std::map<int, int> log_level_map = {
        {AV_LOG_TRACE,   ANDROID_LOG_DEFAULT},
        {AV_LOG_DEBUG,   ANDROID_LOG_DEBUG},
        {AV_LOG_VERBOSE, ANDROID_LOG_VERBOSE},
        {AV_LOG_INFO,    ANDROID_LOG_INFO},
        {AV_LOG_WARNING, ANDROID_LOG_WARN},
        {AV_LOG_ERROR,   ANDROID_LOG_ERROR},
        {AV_LOG_FATAL,   ANDROID_LOG_FATAL},
        {AV_LOG_PANIC,   ANDROID_LOG_FATAL},
    };

    int av_log_level = av_log_get_level();
    if (level > av_log_level)
        return;

    __android_log_vprint(log_level_map.at(level), LOG_TAG, fmt, vl);
}
#endif

CTransLog::CTransLog(const char *name) : m_pClass(nullptr) {
    m_pClass = av_mallocz(sizeof(AVClass));
    ((AVClass *)m_pClass)->class_name = name;
    ((AVClass *)m_pClass)->item_name  = av_default_item_name;
    ((AVClass *)m_pClass)->version    = LIBTRANS_VERSION_INT;

#ifdef ANDROID_LOG_EN
    av_log_set_callback(log_callback_ffmpeg);
#endif
}

CTransLog::~CTransLog() {
    av_freep(&m_pClass);
}

void CTransLog::SetLogLevel(std::string logLevel) {
    const static std::map<std::string, LogLevel> log_map = {
        { "debug",   LL_DEBUG },
        { "verbose", LL_VERBOSE },
        { "info",    LL_INFO },
        { "warn",    LL_WARN },
        { "error",   LL_ERROR },
    };

    auto it = log_map.find(logLevel);
    if (it != log_map.end())
        CTransLog::SetLogLevel(it->second);
}

void CTransLog::SetLogLevel(LogLevel level) {
    int avlevel = AV_LOG_QUIET;

    switch (level) {
        case LL_DEBUG:   avlevel = AV_LOG_DEBUG;   break;
        case LL_VERBOSE: avlevel = AV_LOG_VERBOSE; break;
        case LL_INFO:    avlevel = AV_LOG_INFO;    break;
        case LL_WARN:    avlevel = AV_LOG_WARNING; break;
        case LL_ERROR:   avlevel = AV_LOG_ERROR;   break;
        default: break;
    }

    av_log_set_level(avlevel);
}

string CTransLog::ErrToStr(int ret) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    return av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret);
}

string CTransLog::TsToStr(int64_t ts) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    return av_ts_make_string(buf, ts);
}

string CTransLog::TsToTimeStr(int64_t ts, int num, int den) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    AVRational tb = { num, den };
    return av_ts_make_time_string(buf, ts, &tb);
}

void CTransLog::Verbose(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_VERBOSE, format, list);
    va_end(list);
}

void CTransLog::Debug(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_DEBUG, format, list);
    va_end(list);
}

void CTransLog::Info(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_INFO, format, list);
    va_end(list);
}

void CTransLog::Warn(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_WARNING, format, list);
    va_end(list);
}

void CTransLog::Error(const char *format, ...) {
    va_list list;
    va_start(list, format);
    av_vlog(&m_pClass, AV_LOG_ERROR, format, list);
    va_end(list);
}
