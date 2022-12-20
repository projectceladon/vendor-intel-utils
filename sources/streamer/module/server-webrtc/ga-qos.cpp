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

#include "ga-qos.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace ga {
  namespace webrtc {
    std::string QoSUtils::GetJsonForQoSInfo(QosInfo& qos_info) {
      json qos_json;
      qos_json["type"] = "qos";
      qos_json["frameno"] = qos_info.frameno;
      qos_json["framesize"] = qos_info.framesize;
      qos_json["bitrate"] = qos_info.bitrate;
      qos_json["captureFps"] = qos_info.captureFps;
      qos_json["eventime_sec"] = qos_info.eventime.tv_sec;
      qos_json["eventime_usec"] = qos_info.eventime.tv_usec;
      qos_json["capturetime"] = qos_info.capturetime;
      qos_json["encodetime"] = qos_info.encodetime;
      qos_json["estimated_bw"] = qos_info.estimated_bw;
      return qos_json.dump();
    }
  }
}
