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

#include "android-common.h"
#include "ga-conf.h"
#include "nlohmann/json.hpp"
#include "CommandChannelHandler.h"

#define TAG "CommandChannelHandler: "
using json = nlohmann::json;

CommandChannelHandler::CommandChannelHandler(int instance_id,
                                             MessageHandler msg_handler)
  : mMsgHandler(msg_handler) {
  vhal::client::TcpConnectionInfo conn_info ={ android::ip() };
  auto callback = [&](const CommandChannelMessage &command_channel_msg) {
    switch (command_channel_msg.msg_type) {
      case MsgType::kActivityMonitor: {
        ga_logger(Severity::INFO, TAG "MsgType::kActivityMonitor\n");
        std::string msg((char*)command_channel_msg.data, command_channel_msg.data_size);
        mMsgHandler(command_channel_msg.msg_type, msg);
        break;
      }
      case MsgType::kAicCommand: {
        ga_logger(Severity::INFO, TAG "MsgType::kAicCommand\n");
        std::string msg((char*)command_channel_msg.data, command_channel_msg.data_size);
        mMsgHandler(command_channel_msg.msg_type, msg);
        break;
      }
      default:
        ga_logger(Severity::WARNING, TAG "unknown message type\n");
    }
  };
  command_channel_interface_ =
    std::make_unique<vhal::client::CommandChannelInterface>(conn_info, callback);
}

void CommandChannelHandler::processClientMsg(const std::string &json_message) const {
  if (json_message.empty())
    return;

  json j = json::parse(json_message);
  if (!j["data"]["parameters"].is_object())
    return;

  json event_param = j["data"]["parameters"];
  if (event_param["pkg"].is_string()) {
    std::string pkg = event_param["pkg"];
    if (!pkg.empty()) {
      if (ga_conf_readbool("enable-multi-user", 0) != 0) {
	int userId = ga_conf_readint("user");
	Send(MsgType::kActivityMonitor, "0:" + pkg + ":" + std::to_string(userId));
      } else {
        Send(MsgType::kActivityMonitor, "0:" + pkg);
      }
    }
  }

  if (event_param["cmd"].is_string()) {
    std::string cmd = event_param["cmd"];
    if (!cmd.empty())
      Send(MsgType::kAicCommand, cmd);
  }
}

void CommandChannelHandler::Send(const MsgType &type, const std::string &msg) const {
  IOResult ior = command_channel_interface_->SendDataPacket(
    type, reinterpret_cast<const uint8_t*>(msg.c_str()),
    msg.length());
  int sent = std::get<0>(ior);
  if (sent < 0) {
    ga_logger(Severity::ERR,
              "Error in writing payload to Command Channel Server: %s\n",
              std::get<1>(ior).c_str());
  }
}
