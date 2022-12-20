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

#ifndef FILEAUDIOFRAMEGENERATOR_H
#define FILEAUDIOFRAMEGENERATOR_H

#include "asource.h"
#include "owt/base/framegeneratorinterface.h"
#include <mutex>

namespace ga {
namespace webrtc {

/// This class generate audio frames from input file.
class GAAudioFrameGenerator : public owt::base::AudioFrameGeneratorInterface {
public:
  static GAAudioFrameGenerator *Create(int channel_number, int sample_rate);

  explicit GAAudioFrameGenerator(int channel_number, int sample_rate);
  virtual ~GAAudioFrameGenerator();

  GAAudioFrameGenerator(const GAAudioFrameGenerator&) = delete;
  GAAudioFrameGenerator& operator=(const GAAudioFrameGenerator&) = delete;

  bool Init();
  void ClientConnectionStatus(bool status);

  virtual uint32_t GenerateFramesForNext10Ms(uint8_t *frame_buffer,
                                             const uint32_t capacity) override;
  virtual int GetSampleRate() override;
  virtual int GetChannelNumber() override;

private:
  int channel_number_;
  int sample_rate_;
  int sample_size_;
  int recording_frames_in_10_ms_;
  int recording_buffer_size_in_10ms_;
  uint8_t *recording_buffer_; // In bytes.
  audio_buffer_t *audio_buffer_;
};
} // namespace webrtc
} // namespace ga

#endif // FILEAUDIOFRAMEGENERATOR_H
