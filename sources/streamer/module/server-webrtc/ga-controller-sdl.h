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

#include "ga-controller.h"
#include "ctrl-sdl/ctrl-sdl.h"

namespace ga {
namespace webrtc {

/// This class convert JSON mouse/keyboard events to SDL format and push these
/// events to SDL module.
class SdlController : public Controller {
public:
  SdlController(
    uint32_t game_width, uint32_t game_height,
    uint32_t video_width, uint32_t video_height);
  virtual ~SdlController() = default;

  void PushClientEvent(const std::string &jsonMessage) override;

private:
  enum class FitMode : unsigned short { Fit, Stretch };

  sdlmsg_t *ConvertToSdlMessage(const std::string &jsonMessage, sdlmsg_t *m);
 // Set padding_x_ and padding_y_ based on game resolution at server side and
 // renderer resolution at client side. We assume the game is always in the
 // center of video element.
  void CalculatePaddings();
  void UpdateMousePosition(const MousePosition& position);

  bool enable_relative_position_;  // Determine whether mouse movement message send to SDL is in relative mode.
  bool mouse_is_pressed_;  // Record whether mouse is pressed since client only sends mouseup and mousedown.
  // Game resolution at server side.
  uint32_t game_width_;
  uint32_t game_height_;
  // Renderer size at client side. It's a video element in most cases.
  uint32_t renderer_width_;
  uint32_t renderer_height_;
  uint32_t renderer_padding_x_;
  uint32_t renderer_padding_y_;
  // Video source size at client side. It's usually equal to the encoder resolution at server side.
  uint32_t video_width_;
  uint32_t video_height_;
  float accumulated_wheel_x_;
  float accumulated_wheel_y_;
  // Display resolution on server side that the game is rendered on.
  int32_t display_width_;
  int32_t display_height_;
  

  MousePosition current_mouse_position_;

  FitMode mode_;
};

} // namespace webrtc
} // namespace ga

