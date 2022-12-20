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

#pragma once

#include <memory>
#include <vector>

#include "ga-controller.h"
#include "virtual_input_receiver.h"

namespace ga {
namespace webrtc {

class AndroidController : public Controller {
public:
  AndroidController(
    uint32_t game_width, uint32_t game_height,
    uint32_t video_width, uint32_t video_height);
  virtual ~AndroidController() = default;

  void PushClientEvent(const std::string &jsonMessage) override;

private:
  inline int TranslateX()
  {
    return (getenv("OLD_INPUT_PROTOCOL"))? (double)abs_x/abs_gameWidth * 32767: abs_x;
  }
  inline int TranslateY()
  {
    return (getenv("OLD_INPUT_PROTOCOL"))? (double)abs_y/abs_gameHeight * 32767: abs_y;
  }

  int abs_x = 0;
  int abs_y = 0;
  int abs_gameWidth = 0;
  int abs_gameHeight = 0;
  bool is_pressed;
  bool is_mouse;
  int game_offset_x;
  int game_offset_y;

  // Game resolution at server side.
  uint32_t game_width_;
  uint32_t game_height_;
  uint32_t video_width_;
  uint32_t video_height_;

  std::vector<std::unique_ptr<vhal::client::VirtualInputReceiver>> inputDev;
};

} // namespace webrtc
} // namespace ga
