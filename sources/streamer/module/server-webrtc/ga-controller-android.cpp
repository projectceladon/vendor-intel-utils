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

#include "ga-controller-android.h"
#include "ga-conf.h"
#include <iostream>
#include "nlohmann/json.hpp"
#include "android-common.h"

using json = nlohmann::json;

ga::webrtc::AndroidController::AndroidController(
  uint32_t game_width, uint32_t game_height,
  uint32_t video_width, uint32_t video_height)
    : game_width_(game_width),
      game_height_(game_height),
      video_width_(video_width),
      video_height_(video_height) {
  int session = ga_conf_readint("android-session");
  if (session < 0)
    session = 0;

  std::string prefix_socket;
  std::string prefix_status;
  vhal::client::UnixConnectionInfo uci;
  int virtualInputNum = ga_conf_readint("virtual-input-num");
  if (virtualInputNum < 1){
    virtualInputNum = 1;
  }
  if (virtualInputNum > 31){
    virtualInputNum = 31;
  }
  if (inputDev.empty()){
    std::unique_ptr<vhal::client::VirtualInputReceiver> tempVirtualInputReceiver = nullptr;
    prefix_socket = ga_conf_readstr("aic-workdir");
    prefix_status = ga_conf_readstr("aic-workdir");
    if (ga_conf_readbool("k8s", 0) == 1) {
      prefix_socket += std::string("/input-pipe");
      prefix_status += std::string("/.input-status");
    } else {
      prefix_socket += "/ipc/input-pipe" + std::to_string(session);
      prefix_status += "/ipc/.input-status" + std::to_string(session);
    }
    if (ga_conf_readbool("enable-multi-user", 0) != 0) {
      int userId = ga_conf_readint("user");
      prefix_socket += "-";
      prefix_socket += std::to_string(userId);
      prefix_status += "-" + std::to_string(userId);
    }
    for (int i = 0; i < virtualInputNum; i++) {
      uci.socket_dir = (prefix_socket + "-" + std::to_string(i));
      uci.status_dir = prefix_status + "-" + std::to_string(i);
      ga_logger(Severity::INFO, "Create %s\n", uci.socket_dir.c_str());
      tempVirtualInputReceiver = std::make_unique<vhal::client::VirtualInputReceiver>(uci);
      inputDev.push_back(std::move(tempVirtualInputReceiver));
    }
  }

  if (getenv("OLD_INPUT_PROTOCOL")) {
    ga_logger(Severity::WARNING, "old input device protocol in use!\n");
    abs_gameWidth = 1920;
    abs_gameHeight = 1080;
    abs_x = abs_gameWidth / 2;
    abs_y = abs_gameHeight / 2;
  }
  is_pressed = false;
  is_mouse = true;
  game_offset_x = 0;
  game_offset_y = 0;
}

void ga::webrtc::AndroidController::PushClientEvent(const std::string &json_message) {
  json j = json::parse(json_message);
  // Data validation.
  if (!j["type"].is_string()) {
    return;
  }
  if (j["type"].get<std::string>() != "control") {
    return;
  }
  if (!j["data"].is_object()) {
    return;
  }
  if (!j["data"]["event"].is_string()) {
    return;
  }
  std::string event_type = j["data"]["event"];
  if (event_type == "mousemove") {
    json event_param = j["data"]["parameters"];
    int32_t x = event_param["x"];
    int32_t y = event_param["y"];
    ga_logger(Severity::DBG, "mousemove: x=%d, y=%d\n", x, y);

    if (is_mouse)
    {
        abs_x += x;
        abs_y += y;
        if (abs_x < 0) abs_x = 0;
        if (abs_y < 0) abs_y = 0;
        if (abs_x > abs_gameWidth) abs_x = abs_gameWidth;
        if (abs_y > abs_gameHeight) abs_y = abs_gameHeight;
    }
    else
    {
        abs_x = x;
        abs_y = y;
    }

    if (is_pressed)
    {
        char cmd[512];
        sprintf(cmd, "m 0 %d %d 255\n",  game_offset_x + TranslateX(), game_offset_y + TranslateY());
        inputDev[0]->onInputMessage(cmd);
        inputDev[0]->onInputMessage("c\n");
    }
  } else if (event_type == "mousedown") {
    // Mouse click.
    // sdlmsg_t m;
    json event_param = j["data"]["parameters"];
    if(event_param.size() < 3)
      return;
    int32_t x = event_param["x"];
    int32_t y = event_param["y"];
    ga_logger(Severity::DBG, "mousedown: x=%d, y=%d\n", x, y);

    if (x > 4 || y > 4)
    {
        is_mouse = false;
    }
    else
    {
        is_mouse = true;
    }

    if (!is_mouse)
    {
        abs_x = x;
        abs_y = y;
    }

    is_pressed = true;

    char cmd[512];

    sprintf(cmd, "d 0 %d %d 255\n", TranslateX() + game_offset_x, TranslateY() + game_offset_y);
    inputDev[0]->onInputMessage(cmd);
    inputDev[0]->onInputMessage("c\n");
  } else if (event_type == "mouseup") {
    ga_logger(Severity::DBG, "mouseup\n");
    is_pressed = false;
    is_mouse = true;
    inputDev[0]->onInputMessage("u 0\n");
    inputDev[0]->onInputMessage("c\n");
  } else if (event_type == "touch") {
    json event_param = j["data"]["parameters"];
    std::string data = event_param["data"];
    ga_logger(Severity::DBG, "touch data: %s\n", data.c_str());
    unsigned int tId = event_param["tID"];
    if (tId >= inputDev.size()){
      ga_logger(Severity::ERR, "tID out of scope. tID = %d\n", tId);
      return;
    }

    if (android::is_atrace_enabled()) {
      std::string::size_type upIndex = data.find("u ");
      if (upIndex != std::string::npos) {
        static int nS1TouchCount = 0;
        nS1TouchCount++;
        std::string str = "atou S1 ID: " + std::to_string(nS1TouchCount);
        android::atrace_begin(str);
        android::atrace_end();
      }
    }
    ssize_t sts = 0;
    std::string err;
    std::tie(sts, err) = inputDev[tId]->onInputMessage(data);
    if (sts < 0)
      ga_logger(Severity::ERR, "Failed to pass 'touch' input message: %s\n", err.c_str());
  } else if (event_type == "joystick") {
    json event_param = j["data"]["parameters"];
    std::string data = event_param["data"];
    ga_logger(Severity::DBG, "joystick data: %s\n", data.c_str());
    unsigned int jId = event_param["jID"];
    if (jId>= inputDev.size() ) {
      ga_logger(Severity::ERR, "jID out of scope. jID = %d\n", jId);
      return;
    }
    ssize_t sts = 0;
    std::string err;
    std::tie(sts, err) = inputDev[jId]->onJoystickMessage(data);
    if (sts < 0)
      ga_logger(Severity::ERR, "Failed to pass 'joystick' input message: %s\n", err.c_str());
  } else {
    ga_logger(Severity::DBG, "unknown event type: %s\n", event_type.c_str());
  }
  return;
}
