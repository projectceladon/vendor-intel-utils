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

#include "enc_frame_settings_predictor.h"
#include <cmath>

#define CHECK_POINTER(P, ERR) {if (!(P)) { printf("CHECK_POINTER: (" #P ") = nil; return " #ERR " : -"); return ERR;}}

PredictorTcaeImpl::PredictorTcaeImpl() :
    m_NetworkPredictor(),
    m_features(),
    m_initialTargetDelayInMs(),
    m_maxSequentialDropsCount(),
    m_minDropFramesDist(),
    m_maxFrameSizeInBytes(),
    m_idrRequested(),
    m_lastTargetFS(),
    m_frameDropsCount(),
    m_lastKnownDelayInMs(),
    m_lastTransmittedSize(),
    m_framesSinceLastDrop(),
    m_pLogger()
{
}

PredictorTcaeImpl::~PredictorTcaeImpl()
{

    (void)Stop();
}


tcaeStatus PredictorTcaeImpl::Start(TcaeInitParams_t* params)
{

    if (m_NetworkPredictor)
        return ERR_UNSUPPORTED;

    tcaeStatus sts = ERR_NONE;

    try
    {
        sts = StartImpl(params);
    }
    catch (...)
    {

    }

    return sts;
}


tcaeStatus PredictorTcaeImpl::StartImpl(TcaeInitParams_t* params, StateLogger* log)
{
    tcaeStatus sts = ERR_NONE;

    //check input
    if (!params)
        return ERR_NULL_PTR;

    //check and set features enabled
    //check only known features requested, and required features are ON
    uint32_t features = params->featuresSet;
    if (params->featuresSet == TCAE_MODE_UNKNOWN)
        features = DefaultEnabledFeatures;
    if (params->featuresSet == TCAE_MODE_STANDALONE) // Only STANDALONE set apply default set as addition
        features = DefaultEnabledFeatures | TCAE_MODE_STANDALONE;
    if (features & (~(TCAE_PREDICT_FRAME_SIZE | TCAE_MODE_UNKNOWN | TCAE_FRAME_DROP | TCAE_MODE_STANDALONE)))
        return ERR_INVALID_ARG;
    if ((features & TCAE_PREDICT_FRAME_SIZE) == 0)
        return ERR_UNSUPPORTED;


    //check and set records count default
    uint32_t recCount = params->bufferedRecordsCount;
    if (recCount == TCAE_USE_DEFAULT_VALUE)
        recCount = DefaultRecordsCount;
    if (!recCount)
        return ERR_INVALID_ARG;


    //check the only required field (no default value) - target delay
    uint32_t delay = params->targetDelayInMs;
    if (!delay)
        return ERR_INVALID_ARG;


    //check and set sequential drops count, if frame drop is enabled
    uint16_t dropsCount = 0;
    if  ((features & TCAE_FRAME_DROP) != 0)
    {
        dropsCount = params->maxSequentialDropsCount;
        if (dropsCount == TCAE_USE_DEFAULT_VALUE)
            dropsCount = DefaultMaxDropsCountInSequence;
        if (!dropsCount)
            return ERR_INVALID_ARG;
    }

    //check and set min drop distance, if frame drop is enabled
    uint16_t dropsDistance = 0;
    if ((features & TCAE_FRAME_DROP) != 0)
    {
        dropsDistance = params->minDropFramesDistance;
        if (dropsDistance == TCAE_USE_DEFAULT_VALUE)
            dropsDistance = DefaultMinDropFramesDistance;
        if (!dropsDistance)
            return ERR_INVALID_ARG;
    }

    //check and set maxFrameSize
    uint32_t maxFrameSize = params->maxFrameSizeInBytes;
    if (maxFrameSize == TCAE_USE_DEFAULT_VALUE)
        maxFrameSize = DefaultMaxFrameSizeInBytes;
    if (!maxFrameSize)
        return ERR_INVALID_ARG;


    //Configure Network Predictor
    std::unique_ptr<NetPred> netPred;
    netPred.reset(new NetPred());
    netPred->SetMaxTargetSize(maxFrameSize);
    netPred->SetMinTargetSize(maxFrameSize/10);

    m_NetworkPredictor = std::move(netPred);
    m_NetworkPredictor->SetRecordedLen(recCount);
    m_NetworkPredictor->SetTargetDelay(delay);

    //Initialize TCAE state
    m_features = features;
    m_initialTargetDelayInMs = delay;
    m_maxSequentialDropsCount = dropsCount;
    m_maxFrameSizeInBytes = maxFrameSize;
    m_minDropFramesDist = dropsDistance;
    m_framesSinceLastDrop = m_minDropFramesDist;

    return ERR_NONE;
}

tcaeStatus PredictorTcaeImpl::Stop()
{

    CHECK_POINTER(m_NetworkPredictor,ERR_NOT_INITIALIZED);

    std::lock_guard<std::mutex> guard(m_inputMutex);

    m_NetworkPredictor.reset();

    m_cachedBitstreamSize.clear();
    m_cachedNetworkStats.clear();
    m_idrRequested = false;
    m_lastTargetFS = 0;
    m_frameDropsCount = 0;
    m_lastKnownDelayInMs = 0;
    m_lastTransmittedSize = 0;

    return ERR_NONE;
}


tcaeStatus PredictorTcaeImpl::UpdateNetworkState(PerFrameNetworkData_t* data)
{
    CHECK_POINTER(m_NetworkPredictor, ERR_NOT_INITIALIZED);
    CHECK_POINTER(data, ERR_NULL_PTR);

    tcaeStatus sts = ERR_UNDEFINED_BEHAVIOR;
    try
    {
        sts = UpdateNetworkStateImpl(data);
    }
    catch (...)
    {

    }

    return sts;
}


tcaeStatus PredictorTcaeImpl::UpdateNetworkStateImpl(PerFrameNetworkData_t* data, StateLogger* log)
{
    CHECK_POINTER(data, ERR_NULL_PTR);

    //IDR
    {
        bool idrRequired = data->idrRequired;

        std::lock_guard<std::mutex> guard(m_inputMutex);
        m_idrRequested = idrRequired;
    }

    //Delay
    {
        uint32_t delayInUs = data->lastPacketDelayInUs;
        uint32_t transmittedDataSize = data->transmittedDataSizeInBytes;


        if (!delayInUs || !transmittedDataSize)
            return ERR_INVALID_ARG;

        uint32_t delayInMs = uint32_t(ceil(double(delayInUs) / 1000.0));

        std::lock_guard<std::mutex> guard(m_inputMutex);
        m_cachedNetworkStats.push_back(std::pair<uint32_t, uint32_t>(transmittedDataSize, delayInMs));
    }

    return ERR_NONE;
}

tcaeStatus PredictorTcaeImpl::BitstreamSent(EncodedFrameFeedback_t* bts)
{

    CHECK_POINTER(m_NetworkPredictor, ERR_NOT_INITIALIZED);
    CHECK_POINTER(bts, ERR_NULL_PTR);


    tcaeStatus sts = ERR_UNDEFINED_BEHAVIOR;
    try
    {
        sts = BitstreamSentImpl(bts);
    }
    catch (...)
    {
    }

    return sts;
}


tcaeStatus PredictorTcaeImpl::BitstreamSentImpl(EncodedFrameFeedback_t* bts, StateLogger* log)
{
    CHECK_POINTER(bts, ERR_NULL_PTR);

    uint32_t bitstreamSize = bts->frameSizeInBytes;
    uint64_t frameOrder    = bts->frameOrder;
    uint16_t frameType     = bts->encFrameType;

    // FRAME_ORDER is optional, but must be provided if not stand alone mode
    if (!(m_features & TCAE_MODE_STANDALONE) && (!bts->frameOrder))
        return ERR_INVALID_ARG;

    {
        std::lock_guard<std::mutex> guard(m_inputMutex);
        if ((frameType & TCAE_FRAMETYPE_IDR) && m_idrRequested)
            m_idrRequested = false;
        m_cachedBitstreamSize.push_back(bitstreamSize);
    }

    return ERR_NONE;
}


tcaeStatus PredictorTcaeImpl::PredictEncSettings(FrameSettings_t* perFrameSettings)
{

    CHECK_POINTER(m_NetworkPredictor, ERR_NOT_INITIALIZED);
    CHECK_POINTER(perFrameSettings, ERR_NULL_PTR);

    tcaeStatus sts = ERR_UNDEFINED_BEHAVIOR;
    try
    {
        sts = PredictEncSettingsImpl(perFrameSettings);
    }
    catch (...)
    {

    }

    return sts;
}



tcaeStatus PredictorTcaeImpl::PredictEncSettingsImpl(FrameSettings_t* encSettings, StateLogger* log)
{
    CHECK_POINTER(encSettings, ERR_INVALID_ARG);
    tcaeStatus sts = ERR_NONE;

    bool insertIDR = false;
    uint32_t bitstreamSize = 0, transmittedSize = 0, delayInMs = 0;
    {
        std::lock_guard<std::mutex> guard(m_inputMutex);
        size_t count = std::min(m_cachedBitstreamSize.size(), m_cachedNetworkStats.size());
        while (count-- > 0)
        {
            bitstreamSize = m_cachedBitstreamSize.front();
            transmittedSize = m_cachedNetworkStats.front().first;
            delayInMs = m_cachedNetworkStats.front().second;

            m_NetworkPredictor->UpdateSizeAndDelay(transmittedSize, bitstreamSize, delayInMs);

            m_cachedBitstreamSize.pop_front();
            m_cachedNetworkStats.pop_front();
        }
        insertIDR = m_idrRequested;
        m_idrRequested = false;

        if (m_cachedNetworkStats.size())
        {
            m_lastKnownDelayInMs = m_cachedNetworkStats.back().second;
            m_lastTransmittedSize = m_cachedNetworkStats.back().first;

            m_cachedNetworkStats.clear();
        }
        else if (delayInMs != 0)
        {
            m_lastKnownDelayInMs = delayInMs;
            m_lastTransmittedSize = transmittedSize;
        }

        while (m_lastKnownDelayInMs && (m_cachedBitstreamSize.size() > m_cachedNetworkStats.size()))
        {
            bitstreamSize = m_cachedBitstreamSize.front();
            m_NetworkPredictor->UpdateSizeAndDelay(m_lastTransmittedSize, bitstreamSize, -1);

            m_cachedBitstreamSize.pop_front();
        }
    }

    //get netPred result if exists
    uint32_t predictedFrameSize = m_NetworkPredictor->GetNextFrameSize();
    if (predictedFrameSize == 0)
    {
        predictedFrameSize = m_lastTargetFS; //may be also 0 -> encoder will work with initial bitrate
    }

    // limit minimum frame size to half of previous, to avoid video quality drops too fast
    predictedFrameSize = std::max(predictedFrameSize, m_lastTargetFS >> 1);

    //make a decision about IDR insertion
    if (insertIDR)
    {
        static const uint32_t IDRSizeIncreaseCoeff = 3;

        encSettings->encFrameType = (TCAE_FRAMETYPE_IDR | TCAE_FRAMETYPE_REF | TCAE_FRAMETYPE_I);

        // when network condition is good, boost target frame size of IDR or scene change frame for better quality
        if (m_lastKnownDelayInMs < m_initialTargetDelayInMs)
        {
            predictedFrameSize = std::min(IDRSizeIncreaseCoeff * predictedFrameSize, m_maxFrameSizeInBytes);
        }
    }
    else if ((m_features & TCAE_FRAME_DROP) && (m_frameDropsCount < m_maxSequentialDropsCount))
    {
        if ((m_frameDropsCount == 0) && (m_framesSinceLastDrop < m_minDropFramesDist)); //new sequence, distance is too short
        else if (m_lastKnownDelayInMs > m_initialTargetDelayInMs)    //either 0 or meaningful value for m_lastKnownDelay is covered
        {
            // set skip count if previous frame is not skipped and current skip count is 0
            uint32_t calcDropsCount = (uint32_t)(round(double(m_lastKnownDelayInMs) / double(m_initialTargetDelayInMs)));
            if (m_frameDropsCount < calcDropsCount)
            {
                encSettings->dropFrame = true;
                m_frameDropsCount++;
                m_framesSinceLastDrop = 0;

                return ERR_NONE;
            }
        }
    }

    //frame is not droppped
    m_frameDropsCount = 0;
    if (m_features & TCAE_FRAME_DROP)
       m_framesSinceLastDrop++;

    encSettings->frameSizeInBytes = predictedFrameSize;

    return ERR_NONE;
}
