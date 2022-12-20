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
 * @brief Implements to establish the communication between client
 * and server for camera module.
 * It helps to receive the client camera capability info and update
 * the same to Android camera vHAL.
 */
#include "CameraClientHandler.h"
#include "ga-conf.h"
#include "nlohmann/json.hpp"

#define TAG "CameraClientHandler: "

using json = nlohmann::json;

const std::string CameraClientHandler::stopPreviewStreamMsg =
    "{ \"key\" : \"stop-camera-preview\" }";

CameraClientHandler::CameraClientHandler(std::shared_ptr<vhal::client::VideoSink> video_sink){
  this->video_sink_ = video_sink;
}

std::vector<vhal::client::VideoSink::camera_info_t>
CameraClientHandler::processClientCameraMsg(const std::string &json_message)
{
  // camera_info would come irrespective of get-hw-capability value.
  bool get_hw_capability = ga_conf_readbool("get-hw-capability", 1);
  int numOfCamerasRequested = 0;

  json j = json::parse(json_message);
  json event_param = j["data"]["parameters"];

  if (get_hw_capability) {
    if (event_param["numOfCameras"].is_number()) {
      numOfCamerasRequested = event_param["numOfCameras"];
    }
    ga_logger(Severity::INFO, TAG "Number of Cameras requested by client = %d\n",
              numOfCamerasRequested);
  } else {
    // This would come when use camera.conf for capability instead of
    // real client camera capability to do customizations in the camera info.
    ga_logger(Severity::WARNING, TAG "Enabled debugging option, "
              "hence read from camera configuration file camera.conf\n");
    numOfCamerasRequested = ga_conf_readint("number-of-cameras-supported");
  }

  if (numOfCamerasRequested == 0) {
    ga_logger(Severity::WARNING, TAG "Number of cameras requested by client is zero, "
              "camera stack will NOT work\n");
    return std::vector<vhal::client::VideoSink::camera_info_t>();
  } else if (numOfCamerasRequested > MAX_NUM_CAMERAS_SUPPORTED) {
    ga_logger(Severity::WARNING, TAG "Number of cameras requested is larger (%d) "
              "than Android vHAL capability, hence requesting max capable value\n",
              numOfCamerasRequested);
    numOfCamerasRequested = MAX_NUM_CAMERAS_SUPPORTED;
  }
  ga_logger(Severity::INFO, TAG "Number Of cameras going to support is %d\n",
            numOfCamerasRequested);

  std::vector<std::string> maxCameraRes(numOfCamerasRequested);
  std::vector<std::string> camOrientation(numOfCamerasRequested);
  std::vector<std::string> camFacing(numOfCamerasRequested);

  if (get_hw_capability) {
    maxCameraRes = event_param["maxCameraRes"].get<std::vector<std::string>>();
    camOrientation = event_param["camOrientation"].get<std::vector<std::string>>();
    camFacing = event_param["camFacing"].get<std::vector<std::string>>();
  }

  std::vector<vhal::client::VideoSink::camera_info_t> camera_info(numOfCamerasRequested);

  // Update camera capability info to Android camera vHAL for each camera(s) seperately.
  for (int i = 0; i < numOfCamerasRequested; i++) {
    int camera_id = 0;
    std::string codec_type;
    std::string camera_res;
    std::string sensor_orientation;
    std::string camera_facing;
    std::string prop;

    // Set Camera ID from Zero to N based on number of cameras.
    camera_id = camera_info[i].cameraId = i;

    // Set codec type of the camera input that needs to be used for camera frame compression.
    codec_type = ga_conf_readstr("video-codec");
    if (codec_type == "h265" || codec_type == "hevc") {
      camera_info[i].codec_type = vhal::client::VideoSink::VideoCodecType::kH265;
    } else if (codec_type == "h264" || codec_type == "avc") {
      camera_info[i].codec_type = vhal::client::VideoSink::VideoCodecType::kH264;
    } else {
      ga_logger(Severity::WARNING, TAG "Unable to read a valid codec_type, "
                "hence use the default, that is H264\n");
      camera_info[i].codec_type = vhal::client::VideoSink::VideoCodecType::kH264;
    }

    // Set max resolution of the camera that needs to be supported based on client request.
    if (maxCameraRes[i] != "NULL" && get_hw_capability) {
      camera_res = maxCameraRes[i];
    } else {
      ga_logger(Severity::INFO, TAG "maxCameraRes is NULL, hence use default value\n");
      // Use default camera configuration if client capability is not valid.
      prop = "camera-res-" + std::to_string(camera_id);
      camera_res = ga_conf_readstr(prop.c_str());
    }
    // Android vHAL currently supports max 1080p resolution, so choose same if client
    // asks more than 1080p.
    if (camera_res == "1080p" || camera_res == "2160p" || camera_res == "4320p") {
      camera_info[i].resolution = vhal::client::VideoSink::FrameResolution::k1080p;
    } else if (camera_res == "720p") {
      camera_info[i].resolution = vhal::client::VideoSink::FrameResolution::k720p;
    } else if (camera_res == "480p") {
      camera_info[i].resolution = vhal::client::VideoSink::FrameResolution::k480p;
    } else {
      ga_logger(Severity::WARNING, TAG "Unable to read a valid resolution, "
                "hence use the default, that is 480p\n");
      camera_info[i].resolution = vhal::client::VideoSink::FrameResolution::k480p;
    }

    // Set image sensor orientation info based on client request.
    if (camOrientation[i] != "NULL" && get_hw_capability) {
      sensor_orientation = camOrientation[i];
    } else {
      // Use default camera configuration if client capability is not valid.
      ga_logger(Severity::INFO, TAG "camOrientation is NULL, hence use default value\n");
      prop = "camera-orientation-" + std::to_string(camera_id);
      sensor_orientation = ga_conf_readstr(prop.c_str());
    }

    if (sensor_orientation == "270") {
      camera_info[i].sensorOrientation = vhal::client::VideoSink::SensorOrientation::ORIENTATION_270;
    } else if (sensor_orientation == "180") {
      camera_info[i].sensorOrientation = vhal::client::VideoSink::SensorOrientation::ORIENTATION_180;
    } else if (sensor_orientation == "90") {
      camera_info[i].sensorOrientation = vhal::client::VideoSink::SensorOrientation::ORIENTATION_90;
    } else if (sensor_orientation == "0") {
      camera_info[i].sensorOrientation = vhal::client::VideoSink::SensorOrientation::ORIENTATION_0;
    } else {
      ga_logger(Severity::WARNING, TAG "Unable to read a valid orientation, "
                "hence use the default, that is zero deg\n");
      camera_info[i].sensorOrientation = vhal::client::VideoSink::SensorOrientation::ORIENTATION_0;
    }

    // Set camera facing info.
    if (camFacing[i] != "NULL" && get_hw_capability) {
      camera_facing = camFacing[i];
    } else {
      // Use default camera configuration if client capability is not valid.
      ga_logger(Severity::INFO, TAG "camFacing is NULL, hence use default value\n");
      prop = "camera-facing-" + std::to_string(camera_id);
      camera_facing = ga_conf_readstr(prop.c_str());
    }

    if (camera_facing == "front") {
      camera_info[i].facing = vhal::client::VideoSink::CameraFacing::FRONT_FACING;
    } else if (camera_facing == "back") {
      camera_info[i].facing = vhal::client::VideoSink::CameraFacing::BACK_FACING;
    } else {
      ga_logger(Severity::WARNING , TAG "Unable to read a valid facing, "
                "hence use the default, that is back facing\n");
      camera_info[i].facing = vhal::client::VideoSink::CameraFacing::BACK_FACING;
    }

    ga_logger(Severity::INFO, TAG "Setting camera info as codec_type = %d, "
              "resolution = %d, sensor_orientation = %d, and facing = %d\n for Camera Id %d\n",
              camera_info[i].codec_type, camera_info[i].resolution, camera_info[i].sensorOrientation,
              camera_info[i].facing, camera_info[i].cameraId);
  }

  return camera_info;
}

void CameraClientHandler::updateCameraInfo(const std::string &message){
    auto hal_capability = video_sink_->GetCameraCapabilty();
    ga_logger(Severity::INFO, "[video_capture] Max number of cameras supported in the Android HAL is %d "
                "and its capable codec_type = %d and resolution = %d\n", hal_capability->maxNumberOfCameras,
                hal_capability->codec_type, hal_capability->resolution);
    // Process camera info message received from client and update it to
    // camera_info_t struct.
    std::vector<vhal::client::VideoSink::camera_info_t>
    camera_info = processClientCameraMsg(message);
    // TODO: Need to pass this capability info of Android server to client. Will consider later,
    // since it is not high priority and it would not make any issues in the functionality.
    bool res = video_sink_->SetCameraCapabilty(camera_info);
    if (res) {
        ga_logger(Severity::INFO, "[video_capture] Camera capability set successfully\n");
    } else {
        ga_logger(Severity::ERR, "[video_capture] Failed to set camera capabilities\n");
    }
}
