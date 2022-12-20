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
 * @brief Implements owt::base::VideoDecoderInterface.
 * Intention of this implementation is not to provide decoder
 * implementation, but rather to get access to h264 packets
 * from OWT Javascript Client and forward it to AIC Camera VHAL.
 */
#ifndef ENCODED_VIDEO_DISPATCHER_H
#define ENCODED_VIDEO_DISPATCHER_H

#include <memory>
#include <assert.h>
#include <unistd.h>

#include "ga-common.h"
#include "owt/base/videodecoderinterface.h"

#include "video_sink.h"
#include "ga-conf.h"
#include "CameraClientHandler.h"

/**
 * @brief Class that Dispatches Encoded video frames to Camera VHAL.
 *
 */
class EncodedVideoDispatcher : public owt::base::VideoDecoderInterface
{
public:
    using CommandHandler = std::function<void(uint32_t cmd)>;

    EncodedVideoDispatcher(int instance_id, CommandHandler cmd_handler)
      : mCmdHandler(cmd_handler)
    {
        std::string socket_dir = ga_conf_readstr("aic-workdir");
        if (ga_conf_readbool("k8s", 0) == 0) {
            socket_dir += "/ipc";
        }
        ga_logger(Severity::INFO, "[video_capture] VideoSink socketDir:%s instance#%d\n",
                  socket_dir.c_str(), instance_id);
        vhal::client::UnixConnectionInfo conn_info = { socket_dir, instance_id };

        auto callback = [&](const vhal::client::VideoSink::camera_config_cmd_t& ctrl_msg) {
            switch (ctrl_msg.cmd) {
                case vhal::client::VideoSink::camera_cmd_t::CMD_OPEN: {
                    ga_logger(Severity::INFO, "[video_capture] Received Open command from Camera VHal\n");
                    auto camera_config = ctrl_msg.camera_config;
                    ga_logger(Severity::INFO, "[video_capture] camera_id = %d, codec_type = %d, camera_res = %d\n",
                              camera_config.cameraId, camera_config.codec_type, camera_config.resolution);

                    camera_client_handler_->startPreviewStreamMsg =
                       "{ \"key\" : \"start-camera-preview\" , \"cameraRes\" :";
                    camera_client_handler_->startPreviewStreamMsg.append("\"" +
                        std::to_string(camera_config.resolution) + "\", " + "\"cameraId\" :" + "\"" +
                        std::to_string(camera_config.cameraId) + "\"}");
                    mCmdHandler((uint32_t)ctrl_msg.cmd);
                    break;
                }
                case vhal::client::VideoSink::camera_cmd_t::CMD_CLOSE:
                    ga_logger(Severity::INFO, "[video_capture] Received Close command from Camera VHal\n");
                    mCmdHandler((uint32_t)ctrl_msg.cmd);
                    break;
                default:
                    ga_logger(Severity::ERR, "[video_capture] Unknown Command received, exiting with failure\n");
                    break;
            }
        };
        int user_id = -1;
        if (ga_conf_readbool("enable-multi-user", 0) != 0) {
            user_id = ga_conf_readint("user");
        }
        video_sink_ = std::make_shared<vhal::client::VideoSink>(conn_info, callback, user_id);
        camera_client_handler_ = std::make_shared<CameraClientHandler>(video_sink_);
    }

    EncodedVideoDispatcher(std::shared_ptr<vhal::client::VideoSink> videoSink)
      : video_sink_( videoSink )
    {}

    ~EncodedVideoDispatcher() {
        ga_logger(Severity::DBG, "[video_capture] EncodedVideoDispatcher Destructor\n");
    }

    bool InitDecodeContext(owt::base::VideoCodec video_codec) override
    {
        ga_logger(Severity::INFO, "[video_capture] video_codec: %s\n",
                  video_codec == owt::base::VideoCodec::kH264 ? "H264" : "Non-H264");
        video_codec_ = video_codec;
        return true;
    }

    bool OnEncodedFrame(std::unique_ptr<owt::base::VideoEncodedFrame> frame) override;

    bool Release() override
    {
        ga_logger(Severity::DBG, "[video_capture] Release\n");
        return true;
    }

    owt::base::VideoDecoderInterface* Copy() override
    {
        ga_logger(Severity::DBG, "[video_capture] Copy \n");
        return new EncodedVideoDispatcher(video_sink_);
    }

    std::shared_ptr<CameraClientHandler> GetCameraClientHandler()
    {
        return camera_client_handler_;
    }

private:
    owt::base::VideoCodec                    video_codec_ = owt::base::VideoCodec::kH264;
    std::shared_ptr<vhal::client::VideoSink> video_sink_;
    std::shared_ptr<CameraClientHandler>    camera_client_handler_;
    CommandHandler mCmdHandler;
};

#endif /* ENCODED_VIDEO_DISPATCHER_H */

