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

#ifndef _PROFTIMER_H
#define _PROFTIMER_H
#include <iostream>

class ProfTimer {
public:
    ProfTimer(bool stat=false);
    ~ProfTimer();
    long long profTimerBegin();
    // userStartTime
    // user provide the start time of profiling
    long long profTimerEnd(const char *key=NULL, long long userStartTime=0);
    void profTimerReset(const char *key=NULL);
    void printReport(const char *key);
    void setPeriod(int period);
    void enableProf();
private:
    bool mStat;  // true: profile enabled; false: profile disabled
    long long mPeriod; // -1: start latency profiling until user stops explicitly; >0: the profiling period; 0: do not profiling.
    long long mStartTime;  // the start time of one round profiling
    long long mLastTime;   // the start time when profiling begins
    long long mTotalTime;  // the total elapsed time
    long long mTotalCount; // number of rounds profiled
    long long mMax;  // maximum elapsed time for one round
    long long mMin;  // minimum elapsed time  for one round

};
#endif
