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

#ifdef WIN32

#include "ga-cursor.h"
#include "nlohmann\json.hpp"

using json = nlohmann::json;

namespace ga {
  namespace webrtc {
    // Format is:
//  {visible: true, colored: true, framePos: {x, y}, hotspot: {x, y},
//     srcRect: {left: x, top: y, right: z, bottom: v},
//     dstRect: {left: x, top: y, right: z, bottom: v},
//     width: x,  height: y, pitch: z,
//     cursorData: xxxxxxxxxxxxxx}  //base64 encoded data.
    std::string CursorUtils::GetJsonForCursorInfo(CURSOR_DATA& cursor_data) {
      CURSOR_INFO cursor_info = cursor_data.cursorInfo;
      json cursor_json;
      cursor_json["type"] = "cursor";
      cursor_json["visible"] = static_cast<bool>(cursor_info.isVisible);
      // If invisible, only send "invisible"
      if (!cursor_info.isVisible) {
        return cursor_json.dump();
      }
      json src;
      src["left"] = cursor_info.srcRect.left;
      src["top"] = cursor_info.srcRect.top;
      src["right"] = cursor_info.srcRect.right;
      src["bottom"] = cursor_info.srcRect.bottom;
      cursor_json["srcRect"] = src;
      json dst;
      dst["left"] = cursor_info.dstRect.left;
      dst["top"] = cursor_info.dstRect.top;
      dst["right"] = cursor_info.dstRect.right;
      dst["bottom"] = cursor_info.dstRect.bottom;
      cursor_json["dstRect"] = dst;
      cursor_json["width"] = cursor_info.width;
      cursor_json["height"] =cursor_info.height;
      cursor_json["pitch"] = cursor_info.pitch;
      cursor_json["noShapeChange"] = true;

      if (cursor_data.cursorDataUpdate) {
        cursor_json["cursorData"] = cursor_data.cursorData;
        cursor_json["noShapeChange"] = false;
      }
      return cursor_json.dump();
    }
  }
}

#endif
