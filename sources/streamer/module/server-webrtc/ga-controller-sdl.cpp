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

#include "ga-controller-sdl.h"
#include "ga-conf.h"
#include <iostream>
#include "nlohmann\json.hpp"

using json = nlohmann::json;

static std::map<int, std::pair<SDL_Keycode, unsigned short>> key_map = {
    {8,   {SDLK_BACKSPACE, SDL_SCANCODE_BACKSPACE}},        //"backspace"
    {9,   {SDLK_TAB, SDL_SCANCODE_TAB}},                    //TAB
    {18,  {SDLK_LALT, SDL_SCANCODE_LALT}},                    //LALT
    {19,  {SDLK_PAUSE, SDL_SCANCODE_PAUSE}},                //PAUSE
    {20,  {SDLK_CAPSLOCK, SDL_SCANCODE_CAPSLOCK}},            //CAPS
    {33,  {SDLK_PAGEUP, SDL_SCANCODE_PAGEUP}},                //"pageup"
    {34,  {SDLK_PAGEDOWN, SDL_SCANCODE_PAGEDOWN}},            //"pagedown"
    {35,  {SDLK_END, SDL_SCANCODE_END}},                    //"end"
    {36,  {SDLK_HOME, SDL_SCANCODE_HOME}},                    //"home"
    {44,  {SDLK_PRINTSCREEN, SDL_SCANCODE_PRINTSCREEN}},    //PRINTSCREEN
    {45,  {SDLK_INSERT, SDL_SCANCODE_INSERT}},                //"insert"
    {46,  {SDLK_DELETE, SDL_SCANCODE_DELETE}},                //"delete"
    {91,  {SDLK_LGUI, SDL_SCANCODE_LGUI}},                    //lwin
    {92,  {SDLK_RGUI, SDL_SCANCODE_RGUI}},                    //rwin
    {96,  {SDLK_KP_0, SDL_SCANCODE_KP_0}},                    //keypad 0
    {97,  {SDLK_KP_1, SDL_SCANCODE_KP_1}},                    //keypad 1
    {98,  {SDLK_KP_2, SDL_SCANCODE_KP_2}},                    //keypad 2
    {99,  {SDLK_KP_3, SDL_SCANCODE_KP_3}},                    //keypad 3
    {100, {SDLK_KP_4, SDL_SCANCODE_KP_4}},                    //keypad 4
    {101, {SDLK_KP_5, SDL_SCANCODE_KP_5}},                    //keypad 5
    {102, {SDLK_KP_6, SDL_SCANCODE_KP_6}},                    //keypad 6
    {103, {SDLK_KP_7, SDL_SCANCODE_KP_7}},                    //keypad 7
    {104, {SDLK_KP_8, SDL_SCANCODE_KP_8}},                    //keypad 8
    {105, {SDLK_KP_9, SDL_SCANCODE_KP_9}},                    //keypad 9
    {106, {SDLK_KP_MULTIPLY, SDL_SCANCODE_KP_MULTIPLY}},    //"x"
    {107, {SDLK_KP_PLUS, SDL_SCANCODE_KP_PLUS}},            //"+"
    {108, {SDLK_KP_ENTER, SDL_SCANCODE_KP_ENTER}},            //ENTER
    {109, {SDLK_KP_MINUS, SDL_SCANCODE_KP_MINUS}},            //"-"
    {110, {SDLK_KP_PERIOD, SDL_SCANCODE_KP_PERIOD}},        //"."
    {111, {SDLK_KP_DIVIDE, SDL_SCANCODE_KP_DIVIDE}},        //"/"
    {144, {SDLK_NUMLOCKCLEAR, SDL_SCANCODE_NUMLOCKCLEAR}},  //NULLOCK
    {145, {SDLK_SCROLLLOCK, SDL_SCANCODE_SCROLLLOCK}},        //SCROLL
    {164, {SDLK_LALT, SDL_SCANCODE_LALT}},                    //"lalt"
    {165, {SDLK_RALT, SDL_SCANCODE_RALT}},                    //"ralt"
    {186, {SDLK_SEMICOLON, SDL_SCANCODE_SEMICOLON}},        //";"
    {187, {SDLK_EQUALS, SDL_SCANCODE_EQUALS}},                //"="
    {188, {SDLK_COMMA, SDL_SCANCODE_COMMA}},                //","
    {189, {SDLK_MINUS, SDL_SCANCODE_MINUS}},                //"-"
    {190, {SDLK_PERIOD, SDL_SCANCODE_PERIOD}},                //"."
    {191, {SDLK_SLASH, SDL_SCANCODE_SLASH}},                //"?"
    {192, {SDLK_BACKQUOTE, SDL_SCANCODE_GRAVE}},            //"`"
    {219, {SDLK_LEFTBRACKET, SDL_SCANCODE_LEFTBRACKET}},    //"["
    {220, {SDLK_BACKSLASH, SDL_SCANCODE_BACKSLASH}},        //"\"
    {221, {SDLK_RIGHTBRACKET, SDL_SCANCODE_RIGHTBRACKET}},  //"]"
    {222, {SDLK_QUOTE, SDL_SCANCODE_APOSTROPHE}},           //"'"

    {13, {SDLK_RETURN, SDL_SCANCODE_RETURN}},
    {16, {SDLK_LSHIFT, SDL_SCANCODE_LSHIFT}},
    {17, {SDLK_LCTRL, SDL_SCANCODE_LCTRL}},
    {27, {SDLK_ESCAPE, SDL_SCANCODE_ESCAPE}},
    {32, {SDLK_SPACE, SDL_SCANCODE_SPACE}},
    {37, {SDLK_LEFT, SDL_SCANCODE_LEFT}},
    {38, {SDLK_UP, SDL_SCANCODE_UP}},
    {39, {SDLK_RIGHT, SDL_SCANCODE_RIGHT}},
    {40, {SDLK_DOWN, SDL_SCANCODE_DOWN}},
    {48, {SDLK_0, SDL_SCANCODE_0}},
    {49, {SDLK_1, SDL_SCANCODE_1}},
    {50, {SDLK_2, SDL_SCANCODE_2}},
    {51, {SDLK_3, SDL_SCANCODE_3}},
    {52, {SDLK_4, SDL_SCANCODE_4}},
    {53, {SDLK_5, SDL_SCANCODE_5}},
    {54, {SDLK_6, SDL_SCANCODE_6}},
    {55, {SDLK_7, SDL_SCANCODE_7}},
    {56, {SDLK_8, SDL_SCANCODE_8}},
    {57, {SDLK_9, SDL_SCANCODE_9}},
    {65, {SDLK_a, SDL_SCANCODE_A}},
    {66, {SDLK_b, SDL_SCANCODE_B}},
    {67, {SDLK_c, SDL_SCANCODE_C}},
    {68, {SDLK_d, SDL_SCANCODE_D}},
    {69, {SDLK_e, SDL_SCANCODE_E}},
    {70, {SDLK_f, SDL_SCANCODE_F}},
    {71, {SDLK_g, SDL_SCANCODE_G}},
    {72, {SDLK_h, SDL_SCANCODE_H}},
    {73, {SDLK_i, SDL_SCANCODE_I}},
    {74, {SDLK_j, SDL_SCANCODE_J}},
    {75, {SDLK_k, SDL_SCANCODE_K}},
    {76, {SDLK_l, SDL_SCANCODE_L}},
    {77, {SDLK_m, SDL_SCANCODE_M}},
    {78, {SDLK_n, SDL_SCANCODE_N}},
    {79, {SDLK_o, SDL_SCANCODE_O}},
    {80, {SDLK_p, SDL_SCANCODE_P}},
    {81, {SDLK_q, SDL_SCANCODE_Q}},
    {82, {SDLK_r, SDL_SCANCODE_R}},
    {83, {SDLK_s, SDL_SCANCODE_S}},
    {84, {SDLK_t, SDL_SCANCODE_T}},
    {85, {SDLK_u, SDL_SCANCODE_U}},
    {86, {SDLK_v, SDL_SCANCODE_V}},
    {87, {SDLK_w, SDL_SCANCODE_W}},
    {88, {SDLK_x, SDL_SCANCODE_X}},
    {89, {SDLK_y, SDL_SCANCODE_Y}},
    {90, {SDLK_z, SDL_SCANCODE_Z}},
    {112, {SDLK_F1, SDL_SCANCODE_F1}},
    {113, {SDLK_F2, SDL_SCANCODE_F2}},
    {114, {SDLK_F3, SDL_SCANCODE_F3}},
    {115, {SDLK_F4, SDL_SCANCODE_F4}},
    {116, {SDLK_F5, SDL_SCANCODE_F5}},
    {117, {SDLK_F6, SDL_SCANCODE_F6}},
    {118, {SDLK_F7, SDL_SCANCODE_F7}},
    {119, {SDLK_F8, SDL_SCANCODE_F8}},
    {120, {SDLK_F9, SDL_SCANCODE_F9}},
    {121, {SDLK_F10, SDL_SCANCODE_F10}},
    {122, {SDLK_F11, SDL_SCANCODE_F11}},
    {123, {SDLK_F12, SDL_SCANCODE_F12}}};

ga::webrtc::Controller::MousePosition
CalcuateMousePosition(
  int32_t mouse_x, int32_t mouse_y,
  uint32_t screen_width, uint32_t screen_height,
  int32_t display_width, int32_t display_height) {
  
  ga::webrtc::Controller::MousePosition p;

  if (screen_height == 0 || screen_width == 0) {
    p.x = 0;
    p.y = 0;
    return p;
  }
  ga_logger(Severity::DBG, "[AUTO] Mouse Position from client: mouse_x = %d mouse_y = %d\n", mouse_x, mouse_y);

  //client uses absolute cursor position
  p.x = (mouse_x * display_width) / screen_width;
  p.y = (mouse_y * display_height) / screen_height;

  ga_logger(Severity::DBG, "[AUTO] Mouse Position from server: mouse_x = %d mouse_y = %d\n", p.x, p.y);
  
  return p;
}

ga::webrtc::SdlController::SdlController(
  uint32_t game_width, uint32_t game_height,
  uint32_t video_width, uint32_t video_height)
    : enable_relative_position_(false),
      game_width_(game_width),
      game_height_(game_height),
      renderer_width_(1280),
      renderer_height_(720),
      renderer_padding_x_(0),
      renderer_padding_y_(0),
      video_width_(video_width),
      video_height_(video_height),
      display_width_(0),
      display_height_(0),
      accumulated_wheel_x_(0),
      accumulated_wheel_y_(0),
      mode_(FitMode::Fit) {
  CalculatePaddings();
}

void ga::webrtc::SdlController::PushClientEvent(const std::string &json_message) {
  //std::cout << json_message << std::endl;
  sdlmsg_t msg;
  std::string json_msg = json_message;
  sdlmsg_t *sdl = ConvertToSdlMessage(json_msg, &msg);
  if(sdl != NULL) {
    sdlmsg_replay(sdl);
  }
}

sdlmsg_t *
ga::webrtc::SdlController::ConvertToSdlMessage(const std::string &json_message, sdlmsg_t *m) {
  json j = json::parse(json_message);
  // Data validation.
  if (!j["type"].is_string()) {
    return nullptr;
  }
  if (j["type"].get<std::string>() != "control") {
    return nullptr;
  }
  if (!j["data"].is_object()) {
    return nullptr;
  }
  if (!j["data"]["event"].is_string()) {
    return nullptr;
  }
  std::string event_type = j["data"]["event"];
  if (event_type == "mousemove") {
    // Mouse motion.
    // sdlmsg_t m;
    json event_param = j["data"]["parameters"];
    int32_t x = event_param["x"];
    int32_t y = event_param["y"];
    if (enable_relative_position_) {
      int32_t mx = event_param["movementX"];
      int32_t my = event_param["movementY"];
      return sdlmsg_mousemotion(m, 0, 0, mx, my, 0, 1);
    } else {
        MousePosition p = CalcuateMousePosition(x, y, renderer_width_, renderer_height_, display_width_, display_height_);
        return sdlmsg_mousemotion(m, p.x, p.y, 0, 0, 0, 0);
    }
  } else if (event_type == "mouseup" || event_type == "mousedown") {
    // Mouse click.
    // sdlmsg_t m;
    json event_param = j["data"]["parameters"];
    if(event_param.size() < 3)
      return nullptr;
    int32_t x = event_param["x"];
    int32_t y = event_param["y"];
    MousePosition p = CalcuateMousePosition(x, y, renderer_width_, renderer_height_, display_width_, display_height_);
    UpdateMousePosition(p);
    unsigned int which = event_param["which"];  // Pressed key returns from JavaScript side is the same as SDL defines. 1 for left, 2 for middle and 3 for right.
    mouse_is_pressed_ = (event_type == "mousedown") ? true : false;
    return sdlmsg_mousekey(m, mouse_is_pressed_, which, p.x,
                           p.y);
  } else if (event_type == "keydown" || event_type == "keyup") {
    // Keyboard events.
    // sdlmsg_t m;
    json event_param = j["data"]["parameters"];
#ifdef E2ELATENCY_TELEMETRY_ENABLED
    if (event_param["E2ELatency"].is_number()) {
        m->latency_msg = (uint64_t)(event_param["E2ELatency"]);
    }
    else {
        m->latency_msg = 0;
    }
#endif
    unsigned int which = event_param["which"];
    if (key_map.find(which) != key_map.end()) {
      SDL_Keycode key_code = key_map.find(which)->second.first;
      unsigned int scan_code = key_map.find(which)->second.second;

      if (event_type == "keydown"){
          ga_logger(Severity::DBG, "[AUTO] Keyboard events: SDL scancode = %d\n", scan_code);
      }

      return sdlmsg_keyboard(m, (event_type == "keydown" ? 1 : 0), scan_code,
                             key_code, 0, 0);
    }
    return nullptr;
  } else if (event_type == "vjoykeydown" || event_type == "vjoykeyup") {
      json event_param = j["data"]["parameters"];
      unsigned int which = event_param["which"];//0-15
      int status = (event_type == "vjoykeydown" ? 1 : 0);

      return sdlmsg_vjoykey(m, (event_type == "vjoykeydown" ? 1 : 0), which);
  } else if (event_type == "vjoylstick" || event_type == "vjoyrstick") {
      double x, y;
      json event_param = j["data"]["parameters"];
      if (event_type == "vjoylstick")
      {
          x = event_param["lx"];
          y = event_param["ly"];
      }
      else
      {
          x = event_param["rx"];
          y = event_param["ry"];
      }

      return sdlmsg_vjoystick(m, (event_type == "vjoyrstick" ? 1 : 0), x, y);
  } else if (event_type == "vjoyltrigger" || event_type == "vjoyrtrigger") {
      double trigger;
      json event_param = j["data"]["parameters"];
      if (event_type == "vjoyltrigger")
      {
          trigger = event_param["trigger"];
          //ga_logger(Severity::INFO, "vjoyltrigger: trigger = %.5f\n", trigger);
      }
      else
      {
          trigger = event_param["trigger"];
          //ga_logger(Severity::INFO, "vjoyrtrigger: trigger = %.5f\n", trigger);
      }

      return sdlmsg_vjoytrigger(m, (event_type == "vjoyrtrigger" ? 1 : 0), trigger);
  }
  else if (event_type == "sizechange") {
    // Render target size changed. Update screen size for mouse position calculations.
    json event_param = j["data"]["parameters"];
    json video_size = event_param["rendererSize"];
    this->renderer_height_ = video_size["height"];
    this->renderer_width_ =video_size["width"];
    std::string mode = event_param["mode"];
    if (event_param["mode"] == "stretch") {
      mode_ = FitMode::Stretch;
    } else {
      mode_ = FitMode::Fit;
    }
    CalculatePaddings();
    return nullptr;
  } else if (event_type == "pointerlockchange") {
    json event_param = j["data"]["parameters"];
    bool locked = event_param["locked"];
    if (locked) {
      enable_relative_position_ = true;
    } else {
      enable_relative_position_ = false;
    }
    return nullptr;
  } else if (event_type == "wheel") {
    json event_param = j["data"]["parameters"];
    float x = event_param["deltaX"];
    float y = event_param["deltaY"];
    int integral_x, integral_y;
    // Don't know why the last two arguments for mouse wheel event is unsigned short. Doc for wheel event is broken. https://wiki.libsdl.org/SDL_MouseWheelEvent
    accumulated_wheel_x_ += x;
    if (accumulated_wheel_x_ > 0) {
      integral_x = static_cast<int>(std::floor(accumulated_wheel_x_));
    } else if (accumulated_wheel_x_ < 0) {
      integral_x = static_cast<int>(std::ceil(accumulated_wheel_x_));
    } else {
      integral_x = 0;
    }
    accumulated_wheel_x_ -= integral_x;

    accumulated_wheel_y_ += y;
    if (accumulated_wheel_y_ > 0) {
      integral_y = static_cast<int>(std::floor(accumulated_wheel_y_));
    } else if (accumulated_wheel_y_ < 0) {
      integral_y = static_cast<int>(std::ceil(accumulated_wheel_y_));
    } else {
      integral_y = 0;
    }
    accumulated_wheel_y_ -= integral_y;
    return sdlmsg_mousewheel(m, integral_x, integral_y);
  } else {
        return nullptr;
  }
}

void ga::webrtc::SdlController::CalculatePaddings() {
  
  gaPoint wlt, wrb, clt, crb;
  int display_width, display_height;
  if (ga_window_bounds(display_width, display_height, wlt, wrb, clt, crb) < 0) {
    renderer_padding_x_ = 0;
    renderer_padding_y_ = 0;
    display_width_ = renderer_width_;
    display_height_ = renderer_height_;
  }
  else {
    renderer_padding_x_ = clt.x;
    renderer_padding_y_ = clt.y;
    display_width_ = display_width;
    display_height_ = display_height;
  }
}

void ga::webrtc::SdlController::UpdateMousePosition(
  const ga::webrtc::Controller::MousePosition& position) {
  // std::cout << "Mouse move: (" << p.x << ", " << p.y << std::endl;
  current_mouse_position_ = position;
}
