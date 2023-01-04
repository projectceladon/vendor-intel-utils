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

#ifndef __NET_PRED_H__
#define __NET_PRED_H__

#include <stdint.h>
#include <deque>
#include <iostream>
#include <fstream>

class NetPred
{
public:
    NetPred();
    ~NetPred();

    void Clear();

    void UpdateSizeAndDelay(uint32_t size, uint32_t encoded_size, double delay_in_ms);
    uint32_t GetNextFrameSize();

    void SetRecordedLen(int record_len);

    void SetTargetDelay(double target_in_ms);

    double GetTargetDelay();

    void SetMaxTargetSize(uint32_t maxBytes);
    void SetMinTargetSize(uint32_t minBytes);

    void SetFPS(double fps);

    void SetNetworkEmulatorHint(bool hint = true);

    // factor 0.0 ~ 1.0. 0.0 means reacting fastest, but may has spikes. 1.0 means reacting slowest, but most smoothing. 0.5 by default
    void SetOutputFilterFactor(double factor);
protected:
    int CurrentState();
    void UpdateModel();

    void HandleNetworkLimitor(uint32_t encoded_size, double delay_in_ms);
    void AdjustTarget();

    void UpdateModelNormal(std::deque<double>& m_delays, std::deque<double>& m_sizes);
    void UpdateModelSmall(std::deque<double>& m_delays, std::deque<double>& m_sizes);
    void UpdateModelSafe(std::deque<double>& m_delays, std::deque<double>& m_sizes);

    double WeightedMean(std::deque<double>& data);

    bool SanityCheck();

    double m_targetDelay;
    int m_recordedLen;
    double m_nextTargetSize;

    double m_exceptionThreshold;

    double m_reverseBandWidth;  //  in Mbytes/per sec
    double m_propagotionDelay;  // in ms ( 10^(-3))

    double m_forgotRatio;

    std::deque<double> m_delays;
    std::deque<double> m_sizes;

    std::deque<double> m_effectiveDelays;
    std::deque<double> m_effectiveSizes;
    double m_effectiveSizeThreshold;
    uint32_t m_effectiveDataLen;

    bool m_dumpflag;
    std::ofstream m_dumpfile;

    double m_maxTargetSize;
    double m_minTargetSize;

    // extra handling for the net emulator
    double m_fps;
    double m_estimatedThresholdSize;
    uint32_t m_timeout; // seconds that threshold timeout
    uint32_t m_timeToExplore; // seconds that tries to reaching out
    uint32_t m_estimateCounts;
    double m_estimateAcc;
    bool m_limitorUsed;
    uint32_t m_observeCounter;
    uint32_t m_observeCounterThreshold;
    bool m_startEstimate;

    bool m_networkEmulatorHint;

    // output filter, IIR
    double m_filteredTargetSize;
    double m_filterFactor;
};

#endif
