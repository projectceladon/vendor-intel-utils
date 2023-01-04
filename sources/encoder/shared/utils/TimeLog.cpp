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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>

#include "utils/TimeLog.h"
#include "encoder_comm.h"

#ifndef LOG_TAG
#define LOG_TAG "IRR_TimeLog"
#endif

// debug.icr.timelog = 0: no print
// debug.icr.timelog = 1: normal print
// debug.icr.timelog = 2: ctime print
// debug.icr.timelog = 3: verbose print
// debug.icr.timelog = 4: all print (ignore diff)
#define TIMELOG_PROP ("debug.icr.timelog")
#define TIMELOG_PROP_ENV ("debug_timelog")

#define gettid() syscall(__NR_gettid)

using namespace std;

int TimeLog::g_timelog_level = 0;
bool TimeLog::g_isInitialized = 0;

TimeLog::TimeLog(const char* name, int mode, unsigned long idx1,  unsigned long idx2)
{
    m_enter = { 0 };
    m_begin = { 0 };
    m_end   = { 0 };
    m_exit  = { 0 };
    m_mode  = mode;
    m_idx1  = idx1;
    m_idx2  = idx2;

#if IRR_TIME_LOG
    if (!g_isInitialized) {
        ALOGI("g_isInitialized! g_timelog_level = %d, addr=%p, timelog_addr=%p", g_timelog_level, &g_isInitialized, &g_timelog_level);
        updateProperty();
    }

    if (g_timelog_level == TIMELOG_LEVEL_NONE) return;

    m_enter_name = string((name != NULL) ? name : "");
    m_begin_name = string("");

    if ((mode & TIME_IRRF) == TIME_IRRF) {
        m_prefix = "IRRF_";
        m_enter_name = m_prefix + m_enter_name;
    }

    if ((g_timelog_level == TIMELOG_LEVEL_VERB) && ((mode & TIME_VERB) == TIME_VERB))  mode = 0;
    if ((g_timelog_level == TIMELOG_LEVEL_ALL) && (mode != TIME_NONE))  mode = 0;

    if (mode != 0 && (mode & (TIME_DIFF | TIME_NONE | TIME_VERB)) == 0) {
        mode = 0;
    }

    m_mode = mode;

    if ((mode & TIME_DIFF) == TIME_DIFF)  gettimeofday(&m_enter, NULL);

    if(!mode) {
        pid_t pid = getpid();
        pid_t tid = gettid();
        if ((mode & TIME_DIFF) != TIME_DIFF) gettimeofday(&m_enter, NULL);

        long long timestamp = (long long)(m_enter.tv_sec)*1000000 + (long long)m_enter.tv_usec;
        if (g_timelog_level == TIMELOG_LEVEL_CTIME) {
            struct tm *ptm = gmtime(&(m_enter.tv_sec));

            ALOGI("pid = %d, tid = %d : %s : enter : idx1 = %ld, idx2 = %ld, timestamp = %lld us, %s",
                    pid, tid, m_enter_name.c_str(), m_idx1, m_idx2, timestamp, (ptm == NULL) ? "" : asctime(ptm));
        } else {
            ALOGI("pid = %d, tid = %d : %s : enter : idx1 = %ld, idx2 = %ld, timestamp = %lld us",
                    pid, tid, m_enter_name.c_str(), m_idx1, m_idx2, timestamp);
        }
    }
#endif
}


TimeLog::~TimeLog()
{
#if IRR_TIME_LOG
    if (g_timelog_level == TIMELOG_LEVEL_NONE) return;

    bool is_big_diff = false;
    long long timestamp_prev = (long long)(m_enter.tv_sec)*1000000 + (long long)m_enter.tv_usec;
    long long timestamp = 0;

    if ((m_mode & TIME_DIFF) == TIME_DIFF) {
        gettimeofday(&m_exit, NULL);
        timestamp = (long long)(m_exit.tv_sec)*1000000 + (long long)m_exit.tv_usec;
        is_big_diff = (timestamp - timestamp_prev) > IRR_TIME_LOG_DIFF_THRESHOLD_US;
        m_mode = is_big_diff ? 0: m_mode;
    }

    if(!m_mode) {
        pid_t pid = getpid();
        pid_t tid = gettid();

        if ((m_mode & TIME_DIFF) != TIME_DIFF) {
            gettimeofday(&m_exit, NULL);
            timestamp = (long long)(m_exit.tv_sec)*1000000 + (long long)m_exit.tv_usec;
        }

        if (is_big_diff) {
            if (g_timelog_level == TIMELOG_LEVEL_CTIME) {
                struct tm *ptm = gmtime(&(m_enter.tv_sec));

                ALOGI("pid = %d, tid = %d : %s : enter : idx1 = %ld, idx2 = %ld, timestamp = %lld us, diff2enter = %lld us, %s",
                        pid, tid, m_enter_name.c_str(), m_idx1, m_idx2, timestamp_prev,
                        timestamp - timestamp_prev, (ptm == NULL) ? "" : asctime(ptm));
            } else {
                ALOGI("pid = %d, tid = %d : %s : enter : idx1 = %ld, idx2 = %ld, timestamp = %lld us, diff2enter = %lld us",
                        pid, tid, m_enter_name.c_str(), m_idx1, m_idx2, timestamp_prev, timestamp - timestamp_prev);
            }
        }

        if (g_timelog_level == TIMELOG_LEVEL_CTIME) {
            struct tm *ptm = gmtime(&(m_exit.tv_sec));

            ALOGI("pid = %d, tid = %d : %s : exit : idx1 = %ld, idx2 = %ld, timestamp = %lld us, diff2enter = %lld us, %s",
                    pid, tid, m_enter_name.c_str(), m_idx1, m_idx2, timestamp,
                    timestamp - timestamp_prev, (ptm == NULL) ? "" : asctime(ptm));
        } else {
            ALOGI("pid = %d, tid = %d : %s : exit : idx1 = %ld, idx2 = %ld, timestamp = %lld us, diff2enter = %lld us",
                    pid, tid, m_enter_name.c_str(), m_idx1, m_idx2, timestamp, timestamp - timestamp_prev);
        }
    }
#endif
}

void TimeLog::begin(const char* name, unsigned long idx1,  unsigned long idx2)
{
#if IRR_TIME_LOG
    if (g_timelog_level == TIMELOG_LEVEL_NONE) return;

    m_begin_name = string((name != NULL) ? name : "");
    m_begin_name.push_back('\0');
    m_idx1 = idx1;
    m_idx2 = idx2;
    pid_t pid = getpid();
    pid_t tid = gettid();
    gettimeofday(&m_begin, NULL);
    long long timestamp = (long long)(m_begin.tv_sec)*1000000 + (long long)m_begin.tv_usec;
    if (g_timelog_level == TIMELOG_LEVEL_CTIME) {
        struct tm *ptm = gmtime(&(m_begin.tv_sec));

        ALOGI("pid = %d, tid = %d : %s : begin : idx1 = %ld, idx2 = %ld, timestamp = %lld us, %s",
                pid, tid, m_begin_name.c_str(), m_idx1, m_idx2, timestamp, (ptm == NULL) ? "" : asctime(ptm));
    } else {
        ALOGI("pid = %d, tid = %d : %s : begin : idx1 = %ld, idx2 = %ld, timestamp = %lld us",
                pid, tid, m_begin_name.c_str(), m_idx1, m_idx2, timestamp);
    }
#endif
}

void TimeLog::end()
{
#if IRR_TIME_LOG
    if (g_timelog_level == TIMELOG_LEVEL_NONE) return;

    gettimeofday(&m_end, NULL);
    long long timestamp = (long long)(m_end.tv_sec)*1000000 + (long long)m_end.tv_usec;
    long long timestamp_prev = (long long)(m_begin.tv_sec)*1000000 + (long long)m_begin.tv_usec;
    pid_t pid = getpid();
    pid_t tid = gettid();
    if (g_timelog_level == TIMELOG_LEVEL_CTIME) {
        struct tm *ptm = gmtime(&(m_end.tv_sec));

        ALOGI("pid = %d, tid = %d : %s : end : idx1 = %ld, idx2 = %ld, timestamp = %lld us, diff2begin = %lld us, %s",
                pid, tid, m_begin_name.c_str(), m_idx1, m_idx2, timestamp,
                timestamp - timestamp_prev, (ptm == NULL) ? "" : asctime(ptm));
    } else {
        ALOGI("pid = %d, tid = %d : %s : end : idx1 = %ld, idx2 = %ld, timestamp = %lld us, diff2begin = %lld us",
                pid, tid, m_begin_name.c_str(), m_idx1, m_idx2, timestamp, timestamp - timestamp_prev);
    }
#endif
}

void TimeLog::updateProperty() {
    g_isInitialized = true;
    g_timelog_level = 0;
#if BUILD_FOR_HOST
    const char *value= getenv(TIMELOG_PROP_ENV);
    if (value != nullptr) g_timelog_level = atoi(value);
    ALOGI("update %s = [%d], addr=%p, timelog_addr=%p", TIMELOG_PROP_ENV, g_timelog_level, &g_isInitialized, &g_timelog_level);
#else
    ICRCommProp comm_prop;
    comm_prop.getSystemPropInt(TIMELOG_PROP, &g_timelog_level);
    ALOGI("update %s = [%d], addr=%p, timelog_addr=%p", TIMELOG_PROP, g_timelog_level, &g_isInitialized, &g_timelog_level);
#endif
}

