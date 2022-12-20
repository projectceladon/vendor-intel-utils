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

#include "ga-audio-input.h"
#include "asource.h"
#include <iostream>

using namespace ga ::webrtc;

GAAudioFrameGenerator::GAAudioFrameGenerator(int channel_number,
                                             int sample_rate)
    : channel_number_(channel_number), sample_rate_(sample_rate),
      sample_size_(16), recording_frames_in_10_ms_(0),
      recording_buffer_size_in_10ms_(0), recording_buffer_(nullptr),
      audio_buffer_(nullptr) {
  std::cout << "GAAudioFrameGenerator ctor" << std::endl;
}

GAAudioFrameGenerator::~GAAudioFrameGenerator() {
  audio_source_client_unregister(ga_gettid());
  if (audio_buffer_) {
    audio_source_buffer_deinit(audio_buffer_);
  }
}

GAAudioFrameGenerator *GAAudioFrameGenerator::Create(int channel_number,
                                                     int sample_rate) {
  std::unique_ptr<GAAudioFrameGenerator> generator =
      std::make_unique<GAAudioFrameGenerator>(channel_number, sample_rate);
  if (!generator->Init()) {
    return nullptr;
  }
  return generator.release();
}

bool GAAudioFrameGenerator::Init() {
  if ((audio_buffer_ = audio_source_buffer_init()) == NULL) {
    ga_logger(Severity::ERR, "audio encoder: cannot initialize audio source buffer.\n");
    return false;
  }
  recording_frames_in_10_ms_ = sample_rate_ / 100;
  recording_buffer_size_in_10ms_ =
      recording_frames_in_10_ms_ * channel_number_ * sample_size_ / 8;
  if (!recording_buffer_) {
    recording_buffer_ = new uint8_t[recording_buffer_size_in_10ms_];
  }
  audio_source_client_register(ga_gettid(), audio_buffer_);
  audio_source_buffer_purge(audio_buffer_);
  return true;
}

void GAAudioFrameGenerator::ClientConnectionStatus(bool status)
{
    if (audio_buffer_) {
        if (status) {
            ga_logger(Severity::INFO, "Client connect.\n");
        } else {
            ga_logger(Severity::INFO, "Client disconnect.\n");
        }
        audio_buffer_->client_connected = status;
    }
}
uint32_t
GAAudioFrameGenerator::GenerateFramesForNext10Ms(uint8_t *frame_buffer,
                                                 const uint32_t capacity) {
  if (capacity < (uint32_t)recording_buffer_size_in_10ms_) {
    std::cout << "The capacity is too small" << std::endl;
    return 0;
  }
  int frames_read = audio_source_buffer_read(audio_buffer_, frame_buffer,
                                             recording_frames_in_10_ms_);
  if (frames_read == recording_frames_in_10_ms_) {
    return recording_buffer_size_in_10ms_;
  } else if (frames_read < recording_frames_in_10_ms_) {
    int frames_to_read = recording_frames_in_10_ms_ - frames_read;
    if (frames_to_read ==
        audio_source_buffer_read(audio_buffer_, frame_buffer + (channel_number_ * sample_size_ / 8) * frames_read , frames_to_read)) {
      return recording_buffer_size_in_10ms_;
    }
    return frames_read;
  } else {
    return 0;
  }
}

int GAAudioFrameGenerator::GetSampleRate() { return sample_rate_; }

int GAAudioFrameGenerator::GetChannelNumber() { return channel_number_; }
