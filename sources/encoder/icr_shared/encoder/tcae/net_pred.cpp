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

#include <stdio.h>
#include "net_pred.h"
#include <stdint.h>
#include <random>
#include <chrono>
#include <algorithm>
#include <math.h>
#include <fstream>
#include <cmath>

NetPred::NetPred():
    m_targetDelay(16),
    m_recordedLen(100),
    m_nextTargetSize(0.0),
    m_exceptionThreshold(1.0),
    m_reverseBandWidth(1.0),
    m_propagotionDelay(0.0),
    m_effectiveDataLen(2),
    m_effectiveSizeThreshold(1.0),
#ifdef _DEBUG
    m_dumpflag(true),
#else
    m_dumpflag(false),
#endif
    m_maxTargetSize(1000.0),
    m_minTargetSize(5.0),
    m_fps(30.0),
    m_estimatedThresholdSize(0.0),
    m_timeout(30),
    m_timeToExplore(10),
    m_estimateCounts(0),
    m_estimateAcc(0.0),
    m_limitorUsed(false),
    m_observeCounter(0),
    m_observeCounterThreshold(5),
    m_startEstimate(false),
    m_networkEmulatorHint(false),
    m_filteredTargetSize(0.0),
    m_filterFactor(0.5)
{

    m_forgotRatio = pow(0.01, 1.0 / 2 /m_recordedLen);

    if (m_dumpflag)
    {
        m_dumpfile.open("netpred2.0_dump.csv");
    }

    if (m_dumpflag)
    {
        m_dumpfile << "sizeInK, delay_in_ms, m_reverseBandWidth, m_propagotionDelay, standardError, propagotionDelay, m_targetDelay, m_nextTargetSize, m_networklimitor, reverseBandwidth" << std::endl;
    }
}

NetPred::~NetPred()
{

    if (m_dumpflag)
    {
        m_dumpfile.close();
    }
}

void NetPred::Clear()
{

    m_sizes.clear();
    m_delays.clear();
}

void NetPred::UpdateSizeAndDelay(uint32_t size, uint32_t encoded_size, double delay_in_ms)
{

    HandleNetworkLimitor(encoded_size, delay_in_ms);
    bool inputValid = true;
    if (delay_in_ms < 0)
    {
        inputValid = false;
    }

    if (!inputValid) // invalid input
    {
        m_nextTargetSize = m_nextTargetSize * 0.95;
        if (m_nextTargetSize < m_minTargetSize)
        {
            m_nextTargetSize = m_minTargetSize;
        }

        if (m_dumpfile.good())
        {
            m_dumpfile << (double)size / 1000.0 << ", " << delay_in_ms << ", " << m_reverseBandWidth << ", " << m_propagotionDelay << ", " << 0 << ", " << m_propagotionDelay << ", "
                << m_targetDelay << ", " << m_nextTargetSize << ", " << m_estimatedThresholdSize << ", " << m_reverseBandWidth << std::endl;
        }
        return;
    }

    double sizeInK = (double)size / 1000.0;

    m_delays.push_front(delay_in_ms);
    m_sizes.push_front(sizeInK);

    if (int(m_delays.size()) > m_recordedLen)
    {
        m_delays.pop_back();
        m_sizes.pop_back();
    }

    if (sizeInK >= m_effectiveSizeThreshold)
    {
        m_effectiveSizes.push_front(sizeInK);
        m_effectiveDelays.push_front(delay_in_ms);
        if (m_effectiveDelays.size() > m_effectiveDataLen)
        {
            m_effectiveSizes.pop_back();
            m_effectiveDelays.pop_back();
        }
    }

    UpdateModel();
    double propagotionDelay = m_propagotionDelay;
    double reverseBandWidth = m_reverseBandWidth;
    double standardError = 0.0;

    if (m_sizes.size() >= 0.2 * m_recordedLen)
    {
        double mse = 0.0;
        double count = 0.0;
        double weight = 1.0;
        for (size_t i = 1; i < m_sizes.size(); i++)
        {
            if (m_delays[i] < 1e-6 && m_sizes[i] < 1e-6)
            {
                ;
            }
            else
            {
                double estDelay = m_reverseBandWidth * m_sizes[i] + m_propagotionDelay;
                mse += weight * weight * (m_delays[i] - estDelay) * (m_delays[i] - estDelay);
                count += weight * weight;
            }
            weight *= m_forgotRatio;
        }

        //initially mse = 0 and count = 0
        //they are initialized in the same place in cycle
        //so valid value can be obtained only in case when mse & count != 0
        if (count > 1e-6)
        {
            mse = mse / count;

            standardError = sqrt(mse);

            double rtEstDelay = m_reverseBandWidth * sizeInK + m_propagotionDelay;

            if (delay_in_ms - rtEstDelay > m_exceptionThreshold * standardError)
            {
                propagotionDelay = delay_in_ms - m_reverseBandWidth * sizeInK;
                if (delay_in_ms > 0.9 * m_targetDelay)
                {
                    propagotionDelay = 0.5 * propagotionDelay + 0.5 * m_propagotionDelay;
                    reverseBandWidth = (delay_in_ms - propagotionDelay) / sizeInK;
                }
            }
        }
    }
    
    m_nextTargetSize = (0.9 * m_targetDelay - propagotionDelay) / reverseBandWidth;

    AdjustTarget();

    if ((m_nextTargetSize < m_minTargetSize) || isnan(m_nextTargetSize))
    {
        m_nextTargetSize = m_minTargetSize;
    }

    if (m_nextTargetSize > m_maxTargetSize)
    {
        m_nextTargetSize = m_maxTargetSize;
    }

    if (m_filteredTargetSize < 1.0)
    {
        m_filteredTargetSize = m_nextTargetSize;
    }

    m_filteredTargetSize = m_filteredTargetSize * 0.9 * m_filterFactor + m_nextTargetSize * (1 - m_filterFactor * 0.9);
    m_nextTargetSize = m_filteredTargetSize;


    // dump
    if (m_dumpfile.good())
    {
        m_dumpfile << sizeInK << ", " << delay_in_ms << ", " << m_reverseBandWidth << ", " << m_propagotionDelay << ", " << m_exceptionThreshold * standardError << ", " << propagotionDelay << ", "
            << m_targetDelay << ", " << m_nextTargetSize << ", " << m_estimatedThresholdSize << ", " << reverseBandWidth << std::endl;
    }

    return;
}

void NetPred::UpdateModel()
{

	std::deque<double> delays;
	std::deque<double> sizes;
	delays.clear();
	sizes.clear();
	bool validSequence = false;
	for (size_t i = 0; i < m_sizes.size(); i++)
	{
		delays.push_back(m_delays[i]);
		sizes.push_back(m_sizes[i]);
		if (m_sizes[i] >= m_effectiveSizeThreshold)
		{
			validSequence = true;
		}
	}

	if (!validSequence)
	{
        // original code fron NetPred.cpp
        //for (int i = m_effectiveSizes.size() - 1; i >= 0; i--)
        //{
        //    delays.push_front(m_effectiveDelays[i]);
        //    delays.pop_back();
        //    sizes.push_front(m_effectiveSizes[i]);
        //    sizes.pop_back();
        //}
        // rewrite to reverse interatore to avoid type cast  size_t -> long long -> see i >= 0 
        for (auto it = m_effectiveSizes.crbegin(); it != m_effectiveSizes.crend(); it++)
        {
            delays.push_front(*it);
            delays.pop_back();
            sizes.push_front(*it);
            sizes.pop_back();
        }

	}

    UpdateModelNormal(delays, sizes);
    if (!SanityCheck())
    {
        UpdateModelSafe(delays, sizes);
    }

    if (!SanityCheck())
    {
        UpdateModelSmall(delays, sizes);
    }

    //double mse = 0.0;
}

void NetPred::UpdateModelNormal(std::deque<double>& delays, std::deque<double>& sizes)
{

    if (delays.size() < 0.2 * m_recordedLen)
    {
        return UpdateModelSmall(delays, sizes);
    }

    double meanDelay = WeightedMean(delays);
    double meanSize = WeightedMean(sizes);

    double accD = 0.0;
    double accN = 0.0;
    double weight = 1.0;
    for (size_t i = 0; i < delays.size(); i++)
    {
        if (delays[i] < 1e-6 && sizes[i] < 1e-6)
        {
            ;
        }
        else
        {
            accD += weight * weight * (delays[i] - meanDelay) * (sizes[i] - meanSize);
            accN += weight * weight * (sizes[i] - meanSize) * (sizes[i] - meanSize);
        }
        
        weight *= m_forgotRatio;
    }
    
    if (accN < 1e-6)
    {
        return UpdateModelSmall(delays, sizes);
    }
    else
    {
        m_reverseBandWidth = accD / accN;
        m_propagotionDelay = meanDelay - m_reverseBandWidth * meanSize;
    }
}

void NetPred::UpdateModelSmall(std::deque<double>& delays, std::deque<double>& sizes)
{

    double meanDelay = WeightedMean(delays);
    double meanSize = WeightedMean(sizes);

    if (meanSize < 1e-6 || meanDelay < 1e-6)
    {
        // invalid data set, leave the model unchanged
        return;
    }

    m_reverseBandWidth = meanDelay / meanSize;
    m_propagotionDelay = 0.1;
}

void NetPred::UpdateModelSafe(std::deque<double>& delays, std::deque<double>& sizes)
{

    double meanDelay = WeightedMean(delays);
    double meanSize = WeightedMean(sizes);

    std::deque<double> safeDelays;
    std::deque<double> safeSizes;

    for (size_t i = 0; i < delays.size(); i ++)
    {
        double deltaX = delays[i] - meanDelay;
        double deltaY = sizes[i] - meanSize;
        if (deltaX * deltaY > 0)
        {
            safeDelays.push_front(delays[i]);
            safeSizes.push_front(sizes[i]);
        }
        else
        {
            safeDelays.push_front(0.0);
            safeSizes.push_front(0.0);
        }
    }

    UpdateModelNormal(safeDelays, safeSizes);
}

double NetPred::WeightedMean(std::deque<double>& data)
{

    double accD = 0.0;
    double accN = 0.0;
    double weight = 1.0;
    for (auto ite = data.begin(); ite != data.end(); ite++)
    {
        accD += weight * (*ite);
        accN += weight;
        weight *= m_forgotRatio;
    }
    if (accN < 1e-6)
    {
        return 0.0;
    }
    return accD / accN;

}

void NetPred::HandleNetworkLimitor(uint32_t encoded_size, double delay_in_ms)
{

    if (!m_networkEmulatorHint)
    {
        return;
    }
    bool spikeEnds = false;
    if (delay_in_ms > m_targetDelay || delay_in_ms < 0.0)
    {
        ++m_observeCounter;
    }
    else
    {
        if (m_observeCounter >= m_observeCounterThreshold)
        {
            spikeEnds = true;
        }
        m_observeCounter = 0;
    }

    if (spikeEnds)
    {
        m_startEstimate = true;
        if (m_estimateCounts > 0)
        {
            m_estimatedThresholdSize = m_estimateAcc / m_estimateCounts;
            m_limitorUsed = true;
        }
        m_estimateAcc = 0.0;
        m_estimateCounts = 0;
    }

    if (m_startEstimate)
    {
        ++ m_estimateCounts;
        m_estimateAcc += (double)encoded_size / 1000.0;
    }

    if (m_estimateCounts > m_timeToExplore* m_fps)
    {
        m_estimatedThresholdSize *= 1.05;
    }

    if (m_estimateCounts > m_timeout* m_fps)
    {
        m_estimateAcc = 0.0;
        m_estimateCounts = 0;
        m_limitorUsed = false;
        m_estimatedThresholdSize = 0.0;
        m_startEstimate = false;
    }
}

void NetPred::AdjustTarget()
{

    if (!m_networkEmulatorHint)
    {
        return;
    }
    if (m_limitorUsed && m_estimatedThresholdSize > 1e-6)
    {
        if (m_nextTargetSize > 0.95 * m_estimatedThresholdSize)
        {
            m_nextTargetSize = 0.95 * m_estimatedThresholdSize;
        }
    }
}

void NetPred::SetOutputFilterFactor(double factor)
{
    if (factor > 1.0)
       factor = 1.0;
    if (factor < 0.0)
        factor = 0.0;

    m_filterFactor = factor;
}

uint32_t NetPred::GetNextFrameSize()
{

    return (int)(m_nextTargetSize * 1000);
}

void NetPred::SetRecordedLen(int record_len)
{
    m_recordedLen = record_len;
}

void NetPred::SetTargetDelay(double target_in_ms)
{
    m_targetDelay = target_in_ms;
}

double NetPred::GetTargetDelay()
{

    return m_targetDelay;
}

void NetPred::SetMaxTargetSize(uint32_t maxBytes)
{
    m_maxTargetSize = (double)maxBytes / 1000.0;
}

void NetPred::SetMinTargetSize(uint32_t minBytes)
{
    m_minTargetSize = (double)minBytes / 1000.0;
}

void NetPred::SetFPS(double fps)
{
    m_fps = fps;
}

void NetPred::SetNetworkEmulatorHint(bool hint)
{
    m_networkEmulatorHint = hint;
}

inline int NetPred::CurrentState()
{
    return 0;
}

inline bool NetPred::SanityCheck()
{
    return m_reverseBandWidth > 0.0 && m_propagotionDelay >= 0.0;
}
