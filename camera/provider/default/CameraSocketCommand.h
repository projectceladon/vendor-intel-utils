/**
 * @file CameraSocketCommand.h
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief  Implementation of protocol between camera vhal and cloud client such
 *         as streamer or cg-proxy.
 * @version 0.1
 * @date 2021-02-15
 *
 * Copyright (c) 2021 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CAMERA_SOCKET_COMMAND_H
#define CAMERA_SOCKET_COMMAND_H

#include <cstdint>
#include <unordered_map>
#include <string>

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

enum class VideoCodecType { kH264 = 1, kH265 = 2,kI420 = 4, kMJPEG = 8, kAll = 15 };
enum class FrameResolution { k480p = 1, k720p = 2, k1080p = 4, kAll = 7 };
#define MAX_NUMBER_OF_SUPPORTED_CAMERAS 2
enum class SensorOrientation {
    ORIENTATION_0 = 0,
    ORIENTATION_90 = 90,
    ORIENTATION_180 = 180,
    ORIENTATION_270 = 270
};

enum class CameraFacing { BACK_FACING = 0, FRONT_FACING = 1 };

enum class CameraSessionState {
    kNone,
    kCameraOpened,
    kCameraClosed,
    kDecodingStarted,
    kDecodingStopped
};

extern const std::unordered_map<CameraSessionState, std::string> kCameraSessionStateNames;

typedef enum _ack_value {
    NACK_CONFIG = 0,
    ACK_CONFIG = 1,
} camera_ack_t;

typedef struct _camera_config {
    uint32_t cameraId;
    uint32_t codec_type;
    uint32_t resolution;
    uint32_t reserved[5];
} camera_config_t;

typedef enum _camera_cmd {
    CMD_OPEN = 11,
    CMD_CLOSE = 12,
} camera_cmd_t;

typedef enum _camera_version {
    CAMERA_VHAL_VERSION_1 = 0,  // decode out of camera vhal
    CAMERA_VHAL_VERSION_2 = 1,  // decode in camera vhal
} camera_version_t;

typedef struct _camera_config_cmd {
    camera_version_t version;
    camera_cmd_t cmd;
    camera_config_t config;
} camera_config_cmd_t;

typedef struct _camera_info {
    uint32_t cameraId;
    uint32_t codec_type;
    uint32_t resolution;
    uint32_t sensorOrientation;
    uint32_t facing;  // '0' for back camera and '1' for front camera
    uint32_t reserved[3];
} camera_info_t;

typedef struct _camera_capability {
    uint32_t codec_type;          // All supported codec_type
    uint32_t resolution;          // All supported resolution
    uint32_t maxNumberOfCameras;  // Max will be restricted to 2
    uint32_t reserved[5];
} camera_capability_t;

typedef enum _camera_packet_type {
    REQUEST_CAPABILITY = 0,
    CAPABILITY = 1,
    CAMERA_CONFIG = 2,
    CAMERA_DATA = 3,
    ACK = 4,
    CAMERA_INFO = 5,
} camera_packet_type_t;

typedef struct _camera_header {
    camera_packet_type_t type;
    uint32_t size;  // number of cameras * sizeof(camera_info_t)
} camera_header_t;

typedef struct _camera_packet {
    camera_header_t header;
    uint8_t payload[0];
} camera_packet_t;

const char* camera_type_to_str(int type);
const char* codec_type_to_str(uint32_t type);
const char* resolution_to_str(uint32_t resolution);
}  // namespace 
}  // namespace android
}
}
}
#endif /* CAMERA_SOCKET_COMMAND_H */
