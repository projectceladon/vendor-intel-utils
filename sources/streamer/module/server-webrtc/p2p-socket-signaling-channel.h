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

#pragma once

#include <vector>
#include "sio_client.h"
#include "owt/p2p/p2psignalingchannelinterface.h"

using namespace owt::p2p;
class connection_listener
{
    sio::client &handler;
public:
    connection_listener(sio::client& hdl) : handler(hdl) {}
    void on_connected() {}
};
namespace ga {
  namespace webrtc {

class P2PSocketSignalingChannel : public P2PSignalingChannelInterface {
public:
    explicit P2PSocketSignalingChannel();
    virtual void AddObserver(
        P2PSignalingChannelObserver& observer) override;
    virtual void RemoveObserver(
        P2PSignalingChannelObserver& observer) override;
    virtual void Connect(const std::string& host,
        const std::string& token,
        std::function<void(const std::string&)> on_success,
        std::function<void(std::unique_ptr<Exception>)> on_failure) override;
    virtual void Disconnect(std::function<void()> on_success,
        std::function<void(std::unique_ptr<Exception>)> on_failure) override;
    virtual void SendMessage(const std::string& message,
        const std::string& target_id,
        std::function<void()> on_success,
        std::function<void(std::unique_ptr<Exception>)> on_failure) override;

private:
    // Observers bind to this signaling channel instance
    std::vector<P2PSignalingChannelObserver*> observers_;
    std::unique_ptr<sio::client> io_;
    std::function<void(const std::string&)> connect_success_callback_;
    std::unique_ptr<connection_listener> connection_listener_;
};

}  // namespace webrtc
}  // namespace ga
