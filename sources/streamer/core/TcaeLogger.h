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

#ifndef TCAE_LOGGER_H
#define TCAE_LOGGER_H

#include <ctime>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <memory>
#include <stdio.h>
#include <map>
#include <stdlib.h>
#include <processthreadsapi.h>

using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::system_clock;

#define TCAE_CHECK_RESULT(P, X, ECODE)  { if ((X) > (P)) { ga_logger(Severity::ERR, "Error: %d\n", ECODE); return; } }
#define TCAE_CHECK_POINTER(P, ECODE)    { if (!(P)) { ga_logger(Severity::ERR, "Null Pointer. Error %d\n", ECODE); return; } }

#define TCAE_TARGET_DELAY_MS_DEFAULT 100
#define TCAE_DEFAULT_LOG_BASE "C:\\Temp\\tcaeLog"

struct FrameData_t
{
    uint32_t targetSize;
    uint32_t encodedSize;
    uint32_t delayInUs;
    uint32_t clientPacketSize;
};

class TcaeLogger
{
 public:
    TcaeLogger();
    ~TcaeLogger();

    inline bool LogEnabled() { return m_enabled; };
    inline bool LogsOnlyMode() { return m_runVBRmode; };

    void InitLog(const char* logPath);
    void UpdateClientFeedback(uint32_t delay, uint32_t size);
    void UpdateEncodedSize(qcsAttributes* bts);
    void GetTargetSize(qcsFrame* perFrameSettings);
    void SetZeroTargetSize(qcsFrame* perFrameSettings);

protected:

    void makeLogEntry(const FrameData_t& data, const char* str);

    bool m_enabled = false;
    FILE* m_logFilePtr = nullptr;  // File for capturing extra logs in CSV format

    //Allow mode where TCAE logging structures are enabled, but not actually used for encode
    //This will allow us to study the delay / size relationship for VBR mode, without TCAE/TCBRC involvement in encode
    bool m_runVBRmode = false;

    int m_EncFrameNumber = 0;
    int m_FeedbackFrameNumber = 0;
    long long int m_startTime = 0;

    FrameData_t m_encData;
    std::mutex m_mutex;
};

TcaeLogger::TcaeLogger():
    m_enabled(false),
    m_logFilePtr(nullptr),
    m_EncFrameNumber(0),
    m_FeedbackFrameNumber(0),
    m_startTime(0)
{
    InitLog(TCAE_DEFAULT_LOG_BASE);
}

TcaeLogger::~TcaeLogger()
{
    if (m_logFilePtr)
        fclose(m_logFilePtr);
}

void TcaeLogger::InitLog(const char* logBase)
{
    if (logBase == nullptr)
        return;

    std::string logPath(logBase);

    char* tcaeLogEnable = nullptr;
    char* brcOverrideMode = nullptr;
    
    errno_t err = 0;
    err = _dupenv_s(&tcaeLogEnable, NULL,"TCAE_LOG_ENABLE");

    if (tcaeLogEnable && err == 0)
    {
        if (atoi(tcaeLogEnable) == 1)
        {
            logPath += "_" + std::to_string((int)GetCurrentThreadId()) + ".log";
            fopen_s(&m_logFilePtr, logPath.c_str(), "w");
        }
    }

    if (!m_logFilePtr)
    {
        ga_logger(Severity::INFO, "Could not open file to write TCAE logs: %s\n", logPath.c_str());
        m_enabled = false;
    }
    else
    {
        m_enabled = true;

        //Write Headers
        fprintf(m_logFilePtr, "FrameDelay,FrameSize,EncSize,PredSize,Feedback_FrameNumber,EncoderThread_FrameNumber,RelativeTimeStamp,Function\n");
        fflush(m_logFilePtr);
    }

    if (!m_enabled)
        return;

    err = _dupenv_s(&brcOverrideMode, NULL, "BRC_OVERRIDE_MODE");
    if (brcOverrideMode && err == 0)
    {
        if (atoi(brcOverrideMode) == 1)
        {
            m_runVBRmode = true;
            ga_logger(Severity::WARNING, "TCAELogger Override enabled: TCBRC will be off and VBR mode will run with delay + size logs\n");
        }
    }
}

void TcaeLogger::UpdateClientFeedback(uint32_t delay, uint32_t size)
{
    if (!LogEnabled())
        return;

    //This is the last data point for a given frame in its life-cycle
    //This is accessed from the feedback thread.

    FrameData_t frameData;
    frameData.delayInUs = delay;
    frameData.clientPacketSize = size;
    frameData.targetSize = 0;
    frameData.encodedSize = 0;

    makeLogEntry(frameData, __FUNCTION__);

    m_FeedbackFrameNumber++;
}

void TcaeLogger::UpdateEncodedSize(qcsAttributes* bts)
{
    if (!LogEnabled())
        return;

    qcsU32 bitstreamSize{};
    qcsStatus sts = bts->pvtbl->GetUINT32(bts->pthis, QCS_FRAME_SIZE_IN_BYTES, &bitstreamSize);

    m_encData.encodedSize = bitstreamSize;
    makeLogEntry(m_encData, __FUNCTION__);

    //We are now ready to bump the EncFrameNumber counter
    m_EncFrameNumber++;
}

void TcaeLogger::GetTargetSize(qcsFrame* perFrameSettings)
{
    if (!LogEnabled())
        return;

    qcsU32 targetSize{};

    qcs::SmartPtr<qcs::Attributes> encSettings;
    qcsStatus sts = perFrameSettings->pvtbl->QueryInterface(perFrameSettings->pthis, QCS_IID_IATTRIBUTES, (void**)&encSettings);
    TCAE_CHECK_RESULT(sts, QCS_ERR_NONE, QCS_ERR_INVALID_ARG);
    TCAE_CHECK_POINTER(encSettings, QCS_ERR_INVALID_ARG);

    sts = encSettings->GetUINT32(QCS_FRAME_SIZE_IN_BYTES, &targetSize);

    //Logging. This is the first data point logged for a given frame.
    //Accessed from the Encode thread
    memset(&m_encData, 0, sizeof(FrameData_t));
    m_encData.targetSize = targetSize;
    makeLogEntry(m_encData, __FUNCTION__);
}

void TcaeLogger::SetZeroTargetSize(qcsFrame* perFrameSettings)
{
    if (!LogEnabled() || !m_runVBRmode)
        return;
    
    qcs::SmartPtr<qcs::Attributes> encSettings;
    qcsStatus sts = perFrameSettings->pvtbl->QueryInterface(perFrameSettings->pthis, QCS_IID_IATTRIBUTES, (void**)&encSettings);
    TCAE_CHECK_RESULT(sts, QCS_ERR_NONE, QCS_ERR_INVALID_ARG);
    TCAE_CHECK_POINTER(encSettings, QCS_ERR_INVALID_ARG);

    sts = encSettings->SetUINT32(QCS_FRAME_SIZE_IN_BYTES, 0);
    TCAE_CHECK_RESULT(sts, QCS_ERR_NONE, QCS_ERR_UNDEFINED_BEHAVIOR);
}

void TcaeLogger::makeLogEntry(const FrameData_t& data, const char* str)
{
    if (!LogEnabled())
        return;

    long long int timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

    if (m_startTime == 0)
        m_startTime = timestamp;

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        fprintf(m_logFilePtr, "%d, %d, %d, %d, %d, %d, %lld, %s\n",
                data.delayInUs, data.clientPacketSize, data.encodedSize, data.targetSize,
                m_FeedbackFrameNumber, m_EncFrameNumber, (timestamp - m_startTime), str);
    }

    fflush(m_logFilePtr);
}

#endif /* TCAE_LOGGER_H */
