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

#pragma once

#include <mutex>
#include <memory>
#include <cstdint>
#include "net_pred.h"

enum tcaeStatus
{
    ERR_NONE                 = 0,    /**< no error */
    ERR_UNKNOWN              = -1,   /**< unknown error. */
    ERR_NULL_PTR             = -2,   /**< null pointer */
    ERR_UNSUPPORTED          = -3,   /**< undeveloped feature */
    ERR_MEMORY_ALLOC         = -4,   /**< failed to allocate memory */
    ERR_INVALID_ARG          = -5,   /**< Args not valid */
    ERR_INVALID_HANDLE       = -6,   /**< invalid handle */
    ERR_LOCK_MEMORY          = -7,   /**< failed to lock the memory block */
    ERR_NOT_INITIALIZED      = -8,   /**< member function called before initialization */
    ERR_UNDEFINED_BEHAVIOR   = -9,   /**< Undefined behavior */
    ERR_MORE_DATA            = -10,  /**< expect more data at input */
    ERR_MORE_SURFACE         = -11,  /**< expect more surface at output */
    ERR_ABORTED              = -12,  /**< operation aborted */
};

enum tcaeFeature
{
    TCAE_USE_DEFAULT_VALUE   = 0x0000,   //Use default values if not set
    TCAE_MODE_UNKNOWN        = 0x0000,   /**< No custom feature set was specified. */
    TCAE_PREDICT_FRAME_SIZE  = 0x0001,   /**< Prediction of target frame size. */
    TCAE_FRAME_DROP          = 0x0002,   /**< Drop frame logic. */
    TCAE_MODE_STANDALONE     = 0x0004,
};

enum tcaeFrameTypeEnum
{
    TCAE_FRAMETYPE_UNKNOWN = 0x0000, /**< No frame type is specified. */
    TCAE_FRAMETYPE_I = 0x0001,       /**< This frame is encoded as an I frame */
    TCAE_FRAMETYPE_P = 0x0002,       /**< This frame is encoded as an P frame */
    TCAE_FRAMETYPE_B = 0x0004,       /**< This frame is encoded as an B frame */
    TCAE_FRAMETYPE_S = 0x0008,       /**< This frame is encoded as either an SI- or SP-frame */
    TCAE_FRAMETYPE_REF = 0x0040,     /**< This frame is encoded as a reference */
    TCAE_FRAMETYPE_IDR = 0x0080      /**< This frame is encoded as an IDR */
};


struct TcaeInitParams_t
{
    uint32_t featuresSet;
    uint32_t targetDelayInMs;
    uint32_t bufferedRecordsCount;
    uint16_t maxSequentialDropsCount;
    uint32_t maxFrameSizeInBytes;
    uint16_t minDropFramesDistance;
};

struct PerFrameNetworkData_t
{
    uint32_t firstPacketDelayInUs;
    uint32_t lastPacketDelayInUs;
    uint32_t transmittedDataSizeInBytes;
    uint32_t packetLossRate;
    int32_t  creditBytes;
    bool     idrRequired;
};

struct EncodedFrameFeedback_t
{
    uint16_t encFrameType;
    uint32_t frameSizeInBytes;
    uint64_t frameOrder;
};

struct FrameSettings_t
{
    uint16_t encFrameType;
    uint32_t frameSizeInBytes;
    bool     dropFrame;
};

class StateLogger
{
    //Empty class for now.
    //Logger function to be added later if needed.
};

class PredictorTcaeImpl
{
public:
    PredictorTcaeImpl();
    ~PredictorTcaeImpl();

    tcaeStatus Start(TcaeInitParams_t* params);
    tcaeStatus UpdateNetworkState(PerFrameNetworkData_t* data);
    tcaeStatus BitstreamSent(EncodedFrameFeedback_t* bts);
    tcaeStatus PredictEncSettings(FrameSettings_t* perFrameSettings);
    tcaeStatus Stop();
    void Reset();
    PredictorTcaeImpl& operator=(const PredictorTcaeImpl&) = delete;
private:

    tcaeStatus StartImpl(TcaeInitParams_t* params, StateLogger* log=nullptr);
    tcaeStatus UpdateNetworkStateImpl(PerFrameNetworkData_t* data, StateLogger* log=nullptr);
    tcaeStatus BitstreamSentImpl(EncodedFrameFeedback_t* bts, StateLogger* log=nullptr);
    tcaeStatus PredictEncSettingsImpl(FrameSettings_t* encSettings, StateLogger* log=nullptr);

    //internal objects
    std::unique_ptr<NetPred> m_NetworkPredictor;

    //initial settings
    uint32_t m_features;
    uint32_t m_initialTargetDelayInMs;
    uint16_t m_maxSequentialDropsCount;
    uint32_t m_maxFrameSizeInBytes;
    uint16_t m_minDropFramesDist;

    //counters
    uint64_t m_framesSinceLastDrop;

    //constants for default settings
    const uint32_t DefaultEnabledFeatures = TCAE_PREDICT_FRAME_SIZE;
    const uint32_t DefaultRecordsCount = 100;
    const uint16_t DefaultMaxDropsCountInSequence = 3;
    const uint16_t DefaultMinDropFramesDistance = 1;
    const uint32_t DefaultMaxFrameSizeInBytes = 50000;

    //protected input
    std::mutex m_inputMutex;
    std::deque<uint32_t> m_cachedBitstreamSize;
    std::deque < std::pair<uint32_t /*transmitted size*/, uint32_t /*delay in ms*/> > m_cachedNetworkStats;
    bool m_idrRequested;

    //last proposed enc settings
    uint32_t m_lastTargetFS;
    uint32_t m_frameDropsCount;
    uint32_t m_lastKnownDelayInMs;
    uint32_t m_lastTransmittedSize;

    StateLogger* m_pLogger; //Not supported currently
};
