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

#ifndef __COMMAND_CHANNEL_HANDLER_H__
#define __COMMAND_CHANNEL_HANDLER_H__

#include "command_channel_interface.h"

using namespace vhal::client;
using MessageHandler = std::function<void(MsgType type, const std::string &msg)>;

class CommandChannelHandler {
public:
  CommandChannelHandler(int instance_id = 0,
                        MessageHandler msg_handler = nullptr);
  virtual ~CommandChannelHandler() {};

  CommandChannelHandler(const CommandChannelHandler&) = delete;
  CommandChannelHandler& operator=(const CommandChannelHandler&) = delete;
  void processClientMsg(const std::string &jsonMessage) const;

private:
  void Send(const MsgType &type, const std::string &msg) const;

private:
  std::unique_ptr<vhal::client::CommandChannelInterface> command_channel_interface_;
  MessageHandler mMsgHandler;
};

#endif  //__COMMAND_CHANNEL_HANDLER_H__
