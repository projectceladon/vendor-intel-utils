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

#include "utils/IOStreamWriter.h"

IOStreamWriter::IOStreamWriter() : CTransLog("IOStreamWriter")
{

}

IOStreamWriter::~IOStreamWriter()
{
    if (mInputFile) {
        mInputFile->close();
        delete mInputFile;
        mInputFile = nullptr;
    }

    if (mOutputFile) {
        mOutputFile->close();
        delete mOutputFile;
        mOutputFile = nullptr;
    }
}

int IOStreamWriter::setInputStream(const std::string &file_path, const int one_frame_size,
                                   const int width, const int height, feedInputCallbackFunc func)
{
    int64_t length = 0;

    std::unique_lock<std::mutex> lock(_mutex);

    std::ifstream *is = new std::ifstream(file_path, std::ifstream::binary);
    if (!is->good()) {
        Error("Open input file: %s failed!\n", file_path.c_str());
        goto error;
    }

    // get length of file:
    is->seekg (0, is->end);
    length = is->tellg();
    is->seekg (0, is->beg);

    if (one_frame_size < 0 || width <= 0 || height <= 0) {
        Error("Incorrect video frame size or bad file!\n");
        goto error;
    }

    mInputFile = is;
    mFeedCbFunc = func;
    mVideoFrameWidth = width;
    mVideoFrameHeight = height;
    mOneVideoFrameSize = one_frame_size;
    mValidInputFrameNum = length / one_frame_size;

    Info("Set input file: %s, one frame size: %d valid frames: %d\n",
            file_path.c_str(), one_frame_size, mValidInputFrameNum);

    return 0;

error:
    is->close();
    delete is;
    return -1;
}

int IOStreamWriter::setOutputStream(const std::string &file_path)
{
    std::unique_lock<std::mutex> lock(_mutex);
    std::ofstream *os = new std::ofstream(file_path);
    if (!os->good()) {
        Error("Open output file:%s failed!", file_path.c_str());
        os->close();
        delete os;
        return -1;
    }

    mOutputFile = os;

    Info("Set output file: %s \n", file_path.c_str());

    return 0;
}

void IOStreamWriter::setOutputFrameNumber(const int frame_num)
{
    std::unique_lock<std::mutex> lock(_mutex);

    mOutputFrameNumber = frame_num;

    Info("Set output frame number: %d \n", frame_num);
}

void IOStreamWriter::feedVideoFrameFromInputStream()
{
    std::unique_lock<std::mutex> lock(_mutex);

    // no valid input set or EoF had reached
    if (mValidInputFrameNum <= 0 || mInputFile == nullptr)
        return;

    if (mInputFile->eof()) {
        Info("EoF reached.\n");
        mInputFile->close();
        delete mInputFile;
        mInputFile = nullptr;
        return;
    }

    // allocate memory:
    char *buffer = new char[mOneVideoFrameSize];

    mInputFile->read(buffer, mOneVideoFrameSize);

    // read a complete frame
    if (mInputFile->gcount() == mOneVideoFrameSize && mFeedCbFunc) {
        mFeedCbFunc(mVideoFrameWidth, mVideoFrameHeight, static_cast<const void*>(buffer), 0);
    }

    delete []buffer;
}

int IOStreamWriter::writeToOutputStream(const char *data, size_t size) {
    std::unique_lock<std::mutex> lock(_mutex);

    if (mOutputFile == nullptr || data == nullptr) {
        return 0;
    }

    mOutputFile->write(data, size);
    mWrittenFrameCount++;

    if (mValidInputFrameNum > 0 && mWrittenFrameCount >= mValidInputFrameNum) {
        Info("Have all input frames written.");
        goto enough_frame_written;
    }

    if (mOutputFrameNumber > 0 && mWrittenFrameCount >= mOutputFrameNumber) {
        Info("Have written enough frames.");
        goto enough_frame_written;
    }

    return 0;

enough_frame_written:
    mOutputFile->close();
    delete mOutputFile;
    mOutputFile = nullptr;

    return 1;
}

