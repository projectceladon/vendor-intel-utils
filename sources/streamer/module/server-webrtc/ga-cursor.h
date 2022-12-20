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

#pragma once

#include "ga-common.h"
#include "cursor.h"
#include <string>

namespace ga {
namespace webrtc {

class CursorUtils {
public:
  // Format is:
  //  {visible: true, colored: true, framePos: {x, y}, hotspot: {x, y},
  //     srcRect: {left: x, top: y, right: z, bottom: v},
  //     dstRect: {left: x, top: y, right: z, bottom: v},
  //     width: x,  height: y, pitch: z,
  //     cursorData: xxxxxxxxxxxxxx}  //base64 encoded data.
  static std::string GetJsonForCursorInfo(CURSOR_DATA &cursor_info);
};
} // namespace webrtc
} // namespace ga

#endif
