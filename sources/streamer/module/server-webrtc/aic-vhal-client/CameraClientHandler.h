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

#ifndef __CAMERA_STREAM_HANDLER_H__
#define __CAMERA_STREAM_HANDLER_H__

#include "video_sink.h"

#include <string>
#include <vector>

#define DEFAULT_NUM_OF_CAMERAS_SUPPORTED	1
#define MAX_NUM_CAMERAS_SUPPORTED 	2

class CameraClientHandler {
private:
  std::shared_ptr<vhal::client::VideoSink> video_sink_;
public:
  CameraClientHandler(std::shared_ptr<vhal::client::VideoSink> video_sink);
  ~CameraClientHandler() {};

  std::string startPreviewStreamMsg =
    "{ \"key\" : \"start-camera-preview\", \
       \"cameraRes\" : \"0\", \
       \"cameraId\" : \"0\" \" \
     }";
  static const std::string stopPreviewStreamMsg;

  /**
   * @brief Process camera message from client that contain
   *        camera info for capability negotiation.
   * @param jsonMessage Json data received from client
   */
  std::vector<vhal::client::VideoSink::camera_info_t>
  processClientCameraMsg(const std::string &jsonMessage);
  void updateCameraInfo(const std::string &message);
};

#endif  //__CAMERA_STREAM_HANDLER_H__
