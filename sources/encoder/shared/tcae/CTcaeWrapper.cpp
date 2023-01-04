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

#include "CTcaeWrapper.h"

CTcaeWrapper::CTcaeWrapper()
{
}

CTcaeWrapper::~CTcaeWrapper()
{
    if (m_logFilePtr)
        fclose(m_logFilePtr);
}

void CTcaeWrapper::makeLogEntry()
{
    if(m_tcaeLogPath == nullptr) //Logging disabled
        return;

    if (m_logFilePtr == nullptr) //Check logfile is open
        return;

    fprintf(m_logFilePtr, "%d, %d, %d, %d\n", m_log_delay, m_log_size, m_log_encSize, m_log_targetSize);
    fflush(m_logFilePtr);
}

int CTcaeWrapper::Initialize(uint32_t targetDelay, uint32_t maxFrameSize)
{
    try
    {
        m_tcae = std::unique_ptr<PredictorTcaeImpl>(new PredictorTcaeImpl());
    }
    catch (const std::bad_alloc& e)
    {
            return ERR_MEMORY_ALLOC;
    }

    TcaeInitParams_t params = {0};
    params.featuresSet          = TCAE_MODE_STANDALONE;
    params.targetDelayInMs      = targetDelay;
    params.bufferedRecordsCount = 100;
    if (maxFrameSize > 0)
        params.maxFrameSizeInBytes = maxFrameSize;

    tcaeStatus sts = m_tcae->Start(&params);
    if (ERR_NONE != sts)
    {
        printf("Failed to start TCAE.\n");
        return sts;
    }
    else
    {
        printf("################TCAE starts success!################\n");
    }

    if(m_tcaeLogPath)
    {
        //Additional Manual logs from this class
        m_logFilePtr = fopen(m_tcaeLogPath, "w");
        if (!m_logFilePtr)
        {
            printf("Could not open file to write TCAE logs: %s\n", m_tcaeLogPath);
            printf("Disabling CTcaeWrapper logs\n");

            //Disable Attempts for logging
            m_tcaeLogPath = nullptr;
        }
        else
        {
            m_log_delay = m_log_size = m_log_encSize = m_log_targetSize = 0;

            //Write Headers
            fprintf(m_logFilePtr, "FrameDelay,FrameSize,EncSize,PredSize\n");
            fflush(m_logFilePtr);
        }
    }

    return 0;
}

int CTcaeWrapper::UpdateClientFeedback(uint32_t delay, uint32_t size)
{
    if (m_tcae == nullptr)
        return ERR_NULL_PTR;

    PerFrameNetworkData_t pfnData = {0};

    pfnData.lastPacketDelayInUs        = delay;
    pfnData.transmittedDataSizeInBytes = size;
    pfnData.packetLossRate             = 0;

    tcaeStatus sts = m_tcae->UpdateNetworkState(&pfnData);
    if (sts != ERR_NONE)
    {
        printf("TCAE: UpdateNetworkState failed with code %d\n", sts);
        return sts;
    }

    m_log_delay = delay;
    m_log_size  = size;
    return 0;
}

int CTcaeWrapper::UpdateEncodedSize(uint32_t encodedSize)
{

    if (m_tcae == nullptr)
        return ERR_NULL_PTR;

    EncodedFrameFeedback_t frameData = {0};
    frameData.encFrameType     = TCAE_FRAMETYPE_UNKNOWN;
    frameData.frameSizeInBytes = encodedSize;

    tcaeStatus sts = m_tcae->BitstreamSent(&frameData);
    if (sts != ERR_NONE)
    {
        printf("TCAE: BitstreamSent is failed with code %d\n", sts);
        return sts;
    }

    m_log_encSize = encodedSize;

    return 0;
}

uint32_t CTcaeWrapper::GetTargetSize()
{
    if (m_tcae == nullptr)
        return ERR_NULL_PTR;

    FrameSettings_t settings = {0};

    tcaeStatus sts = m_tcae->PredictEncSettings(&settings);
    if (sts != ERR_NONE)
    {
        printf("TCAE: Failed to predict encode settings. Error code %d\n", sts);
        return sts;
    }

    uint32_t targetSize = settings.frameSizeInBytes;
    m_log_targetSize = targetSize;
    makeLogEntry();

    return targetSize;
}
