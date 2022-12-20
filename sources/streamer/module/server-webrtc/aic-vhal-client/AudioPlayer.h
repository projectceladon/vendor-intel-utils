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

/**
 * @brief AudioPlayer implements owt::base::AudioPlayerInterface
 *        which receives OnData() callback when audio record
 *        beigns from client.
 */

#ifndef __AUDIO_PLAYER_H__
#define __AUDIO_PLAYER_H__

#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>

#include <owt/base/audioplayerinterface.h>

#include "audio_sink.h"

class AudioPlayer : public owt::base::AudioPlayerInterface {
 public:
  using CommandHandler = std::function<void(uint32_t cmd)>;
  AudioPlayer(int instance_id = 0, CommandHandler cmd_handler = nullptr);
  virtual ~AudioPlayer() override;

  AudioPlayer(const AudioPlayer&) = delete;
  AudioPlayer& operator=(const AudioPlayer&) = delete;

  virtual void OnData(const void *audio_data, int bits_per_sample,
                      int sample_rate, size_t number_of_channels,
                      size_t number_of_frames) override;
  struct AudioConfig {
        int sampleRate = 8000;
        size_t channelCount = 1;
        size_t bufferSizeInBytes = 320;  // period size = 20ms
  };

 private:
  void MonoToStereo(const uint16_t *in_L,  // mono input buffer (left channel)
                    const uint16_t *in_R,  // mono input buffer (right channel)
                    size_t num_samples);   // number of samples
 protected:
  std::unique_ptr<vhal::client::audio::AudioSink> audio_sink;
  AudioConfig mAudioConfig{};
  CommandHandler mCmdHandler;
  uint16_t* mSockBuffer = nullptr;
};

#endif  //__AUDIO_PLAYER_H__
