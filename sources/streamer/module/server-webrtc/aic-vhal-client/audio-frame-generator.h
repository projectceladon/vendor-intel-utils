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

#ifndef AUDIO_FRAME_GENERATOR_H
#define AUDIO_FRAME_GENERATOR_H

#include <mutex>
#include <fstream>
#include <queue>
#include "owt/base/framegeneratorinterface.h"
#include "audio_source.h"
#define MAX_QUEUE_SIZE 3

using CommandHandler = std::function<void(uint32_t cmd)>;

/// This class generate audio frames from input.
class AudioFrameGenerator : public owt::base::AudioFrameGeneratorInterface {
public:
    static AudioFrameGenerator* Create(int instanceId, CommandHandler cmd_handler = nullptr);
    virtual ~AudioFrameGenerator();

    AudioFrameGenerator(const AudioFrameGenerator&) = delete;
    AudioFrameGenerator& operator=(const AudioFrameGenerator&) = delete;

    virtual uint32_t GenerateFramesForNext10Ms(uint8_t *frame_buffer,
                                               const uint32_t capacity) override;
    virtual int GetSampleRate() override;
    virtual int GetChannelNumber() override;

protected:
    explicit AudioFrameGenerator(int instanceId, CommandHandler cmd_handler = nullptr);
    CommandHandler mCmdHandler = nullptr;
    std::unique_ptr<vhal::client::audio::AudioSource> audio_source;
    int mChannelNumber = 2;
    int mSampleRate = 48000;
    int mRecBufLen10ms = 1920;
    uint8_t* mBufferPool[MAX_QUEUE_SIZE] = { nullptr };
    std::queue<int> mBufferIdsQ;
    int mBufferIdToPush = 0;
    uint8_t* mSockBuffer = nullptr;
    std::mutex mBufferMutex;
    std::mutex mMutex;
    //This variable keeps track if the audio stream from the vHAL is in standby mode.
    bool mVhalStreamStopped = true;
    bool mBufferFull = false;
};
#endif // AUDIO_FRAME_GENERATOR_H
