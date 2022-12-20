// Copyright (C) 2022 Intel Corporation
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

#include <iostream>
#include <iomanip>
#include <cstring>

#include "android-common.h"
#include "android_audio_core.h"
#include "ga-common.h"
#include "audio-frame-generator.h"
#include "libvhal_common.h"

#define TAG "[playback]"

AudioFrameGenerator::AudioFrameGenerator(int instanceId, CommandHandler cmd_handler) {
    mCmdHandler = cmd_handler;
    vhal::client::TcpConnectionInfo conn_info ={ android::ip() };

    auto callback = [&](const vhal::client::audio::CtrlMessage& ctrl_msg) {
        switch (ctrl_msg.cmd) {
        case vhal::client::audio::Command::kOpen:
            ga_logger(Severity::INFO, TAG "audio socket cmd is CommandOpen\n");
            mSampleRate = ctrl_msg.asci.sample_rate;
            mChannelNumber = ctrl_msg.asci.channel_count;
            mRecBufLen10ms = ctrl_msg.asci.frame_count *
                                         mChannelNumber *
                                         audio_bytes_per_sample(ctrl_msg.asci.format);
            mVhalStreamStopped=true;
            ga_logger(Severity::INFO,
                TAG "audio channel_count:%d | format:%d | frame_count:%d | "
                "sample_rate:%d | buffer_size:%zu\n",
                ctrl_msg.asci.channel_count, ctrl_msg.asci.format, ctrl_msg.asci.frame_count,
                ctrl_msg.asci.sample_rate, mRecBufLen10ms);
            for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
                if (mBufferPool[i]) {
                    delete[] mBufferPool[i];
                    mBufferPool[i] = nullptr;
                }
            }

            if (mSockBuffer) {
                delete[] mSockBuffer;
                mSockBuffer = nullptr;
            }
            if (mRecBufLen10ms > 0) {
                for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
                    mBufferPool[i] = new uint8_t[mRecBufLen10ms];
                }
                mSockBuffer = new uint8_t[mRecBufLen10ms];
            } else {
                ga_logger(Severity::ERR, TAG "Audio buffer length(%d) is incorrect\n",
                mRecBufLen10ms);
                break;
            }
            break;
        case vhal::client::audio::Command::kClose:
            ga_logger(Severity::INFO, TAG "audio socket cmd is CommandClose\n");
            break;
        case vhal::client::audio::Command::kData:
            ga_logger(Severity::DBG, TAG "audio: Command::kData\n");
            if (ctrl_msg.data_size > 0) {
                auto [read, error_msg] = audio_source->ReadDataPacket(mSockBuffer, ctrl_msg.data_size);
                if (mSockBuffer != nullptr && read < 0) {
                    ga_logger(Severity::ERR, TAG "ReadDataPacket() failed\n");
                    break;
                }
                else {
                    ga_logger(Severity::DBG, TAG "ReadDataPacket() success, read: %lu\n", read);
                }
                {// Scope for mBufferIdsQ access with mBufferMutex
                    std::unique_lock<std::mutex> lock(mBufferMutex);
                    if (mBufferIdsQ.size() == MAX_QUEUE_SIZE){
                        if (!mBufferFull) {
                            ga_logger(Severity::WARNING, TAG "Buffer Queue Full, Skip until available\n");
                            mBufferFull = true;
                        }
                        break;
                    }
                    if (mBufferFull) {
                        ga_logger(Severity::INFO, TAG "Buffer Queue available\n");
                        mBufferFull = false;
                    }
                    auto* buffer = mBufferPool[mBufferIdToPush];
                    memcpy(buffer, mSockBuffer, mRecBufLen10ms);
                    mBufferIdsQ.push(mBufferIdToPush);
                    mBufferIdToPush = (mBufferIdToPush + 1) % MAX_QUEUE_SIZE;
                }
            } else {
                ga_logger(Severity::ERR, TAG "Audio data size %d is incorrect\n",
                  ctrl_msg.data_size);
            }
            break;
        case vhal::client::audio::Command::kStartstream:
            ga_logger(Severity::DBG, TAG "Audio Playback Stream start");
            mVhalStreamStopped=false;
            mCmdHandler(ctrl_msg.cmd);
            break;
        case vhal::client::audio::Command::kStopstream:
            ga_logger(Severity::DBG, TAG "Audio Playback Stream stop");
            mVhalStreamStopped = true;
            mCmdHandler(ctrl_msg.cmd);
            break;
        default:
            ga_logger(Severity::ERR, TAG "audio: unknown audio command: %d\n", ctrl_msg.cmd);
            break;
        }
    };
    int user_id = -1;
    if (ga_conf_readbool("enable-multi-user", 0) != 0) {
        user_id = ga_conf_readint("user");
    }
    audio_source = std::make_unique<vhal::client::audio::AudioSource>(conn_info, callback, user_id);
}

AudioFrameGenerator::~AudioFrameGenerator() {
    std::unique_lock<std::mutex> lock(mMutex);
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (mBufferPool[i]) {
            delete[] mBufferPool[i];
            mBufferPool[i] = nullptr;
        }
    }
    if (mSockBuffer) {
        delete[] mSockBuffer;
    }
}

uint32_t AudioFrameGenerator::GenerateFramesForNext10Ms(uint8_t *frame_buffer,
                                                        const uint32_t capacity) {
    if (capacity < (uint32_t)mRecBufLen10ms) {
        ga_logger(Severity::ERR, TAG " audio: the capacity is too small\n");
        return 0;
    }
    {// Scope for mBufferIdsQ access with mBufferMutex
        std::unique_lock<std::mutex> lock(mBufferMutex);
        if ((mBufferIdsQ.size() > 0) && (mRecBufLen10ms > 0)) {
            auto* buffer = mBufferPool[mBufferIdsQ.front()];
            mBufferIdsQ.pop();
            memcpy(frame_buffer, buffer, mRecBufLen10ms);
            return mRecBufLen10ms;
        }
    }
    if (mVhalStreamStopped) {
        memset(frame_buffer, 0, capacity);
        return capacity;
    }
    return 0;
}

int AudioFrameGenerator::GetSampleRate() {
    ga_logger(Severity::DBG, TAG " audio: sample_rate=%d\n", mSampleRate);
    return mSampleRate;
}

int AudioFrameGenerator::GetChannelNumber() {
    ga_logger(Severity::DBG, TAG "audio: channel_number=%d\n", mChannelNumber);
    return mChannelNumber;
}

AudioFrameGenerator* AudioFrameGenerator::Create(int instanceId, CommandHandler cmd_handler) {
    return new AudioFrameGenerator(instanceId, cmd_handler);
}
