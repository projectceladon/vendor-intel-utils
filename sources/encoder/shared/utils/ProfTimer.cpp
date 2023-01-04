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

#include "utils/ProfTimer.h"
extern "C" {
#include <libavutil/time.h>
}
#include <limits.h>

#include <pthread.h>  //phtread_self
#include <sys/types.h> //getpid
#include <unistd.h> //getpid

#define THOUSAND_F 1000.0
#define MILLION_L 1000000


ProfTimer::ProfTimer(bool stat) {
    profTimerReset();
    mStat = stat;
}

ProfTimer::~ProfTimer() {
}

void ProfTimer::setPeriod(int period) {
    if(period>0) {
        mPeriod = (long long)period*MILLION_L; // second to us
    } else {
        mPeriod = period;
    }
    //printf("latencyPeriod=%d, mPeriod=%ld\n", period, mPeriod);
}

long long ProfTimer::profTimerBegin() {
    if(!mStat)
        return 0;

    mStartTime = av_gettime_relative();
    if(!mLastTime)
        mLastTime = mStartTime;
    //printf("starttime=%ld\n", mStartTime);
    return mStartTime;
}

#define MIN_SWAP(val, minVal) \
    if (val < minVal) {\
        minVal = val;\
    }

#define MAX_SWAP(val, maxVal) \
    if (val > maxVal) {\
        maxVal = val;\
    }

long long ProfTimer::profTimerEnd(const char *key, long long userStartTime) {
    if(!mStat)
        return 0;

    // profiling does not begin
    if(!mStartTime)
        return 0;

    long long startTime = mStartTime;
    if(userStartTime>0) {
        //printf("userStartTime=%ld\n", userStartTime);
        startTime = userStartTime;
        if(!mLastTime){
            mLastTime = userStartTime;
        }
    }

    long long curTime = av_gettime_relative();
    long long elapsed = (curTime - startTime);
    mTotalTime += elapsed;
    mTotalCount++;

    //printf("curtime=%ld\n", curTime);

    MIN_SWAP(elapsed, mMin)
    MAX_SWAP(elapsed, mMax)

    if(mPeriod>0){
        long long delta = curTime - mLastTime;
        if(delta>mPeriod){
            //printf("delta=%ld, lasttime=%ld\n", delta, mLastTime);
            profTimerReset(key);
        }
    }
    return elapsed;
}

void ProfTimer::profTimerReset(const char *key) {
    if(key && mStat) {
        printReport(key);
    }
    mStat = false;
    mStartTime = 0;
    mLastTime = 0;
    mTotalTime = 0;
    mTotalCount = 0;
    mMin = LONG_MAX;
    mMax = 0;
    mPeriod = 0;
    return;
}

void ProfTimer::printReport(const char *key){
    if(mTotalCount)
        printf("(%d, %lu)%s stat: min=%.2f, max=%.2f, avg=%.2fms(%lld/%lld us)\n",
            getpid(), pthread_self(), key,
            mMin/THOUSAND_F,
            mMax/THOUSAND_F,
            mTotalTime/mTotalCount/THOUSAND_F,
            mTotalTime,
            mTotalCount);
}

void ProfTimer::enableProf() {
    // enable profiling
    mStat = true;
}

