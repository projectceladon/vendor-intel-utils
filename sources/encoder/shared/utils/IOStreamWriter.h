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

#ifndef IOSTREAM_WRITER_H
#define IOSTREAM_WRITER_H

#include <fstream>
#include <functional>
#include <condition_variable>
#include "utils/CTransLog.h"

using feedInputCallbackFunc = std::function<void(const int width, const int height,
                                                 const void *pixels, const int flip)>;

class IOStreamWriter : public CTransLog
{
private:
    std::ifstream *mInputFile = nullptr;
    std::ofstream *mOutputFile = nullptr;

    /* input parameters */
    int mValidInputFrameNum = -1;
    int mOneVideoFrameSize  = -1;
    int mVideoFrameWidth    = -1;
    int mVideoFrameHeight   = -1;
    void *mInputCbPrivate   = nullptr;
    feedInputCallbackFunc mFeedCbFunc = nullptr;

    /* output parameters */
    int mOutputFrameNumber  = -1;
    int mWrittenFrameCount  = 0;

    std::mutex _mutex;

public:
    IOStreamWriter(/* args */);
    IOStreamWriter(const IOStreamWriter&) = delete;
    IOStreamWriter& operator=(const IOStreamWriter&) = delete;
    virtual ~IOStreamWriter();

    int setInputStream(const std::string &file_path, const int one_frame_size,
                       const int width, const int height, feedInputCallbackFunc func = nullptr);

    int setOutputStream(const std::string &file_path);

    void setOutputFrameNumber(const int frame_num);

    void feedVideoFrameFromInputStream();

    int writeToOutputStream(const char *data, size_t size);
};

#endif
