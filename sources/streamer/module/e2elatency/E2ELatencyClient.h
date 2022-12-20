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
#include "E2ELatencyBase.h"
#include <list>

namespace IL
{
/**
* @brief E2ELatencyClient class, derived from E2ELatencyBase
* Create, Print and Parse a protobuf message (i.e. latency message) that is communicated with the GA Server.
* Either GA Server or Client adds timestamps to the protobuf message at various points of insertion,
* e.g. upon sending or receiving the message. The E2E latency is determined at the Client side,
* which originates the message and receives the updated message from the Server via a side data channel.
*/
class E2ELatencyClient : public E2ELatencyBase
{
public:
    /**
     * @brief E2E latency Client class constructor
     */
    E2ELatencyClient();
    /**
     * @brief E2E latency Client class destructor
     * Latency message is deleted upon calling destructor
     */
    virtual ~E2ELatencyClient();
    /**
     * @brief E2E latency Client CreateLatencyMsg
     * Creates a latency message and pushes into message queue
     * This is called every event (e.g. key stroke)
     */
    void* CreateLatencyMsg() override;
    /**
     * @brief E2E latency Client PrintMessage
     * @param Input: pointer to binary message (void *)
     * Output: content of message in std::string
     */
    std::string PrintMessage(void* pMsg) override;
    /**
     * @brief E2E latency Client ParseMessage
     * @param Input: std::string buffer
     * Output: pointer to a new latency binary message (void *) containing the input buffer's content
     */
    void* ParseMessage(std::string buf) override;
    /**
     * @brief E2E latency Client SetFrameId
     * Sets client frame ID into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: frame ID to set
     */
    void SetFrameId(void* pMsg, const uint32 frameId) override;
    /**
     * @brief E2E latency Client SetInputTime
     * Sets client input time into the message
     * @param Input: pointer to latency binary message (void *)
     */
    void SetInputTime(void* pMsg) override;
    /**
     * @brief E2E latency Client SetSendTime
     * Sets client send time into the message
     * @param Input: pointer to latency binary message (void *)
     */
    void SetSendTime(void* pMsg) override;
    /**
     * @brief E2E latency Client SetReceivedTime
     * Sets client received time into the message
     * @param Input: pointer to latency binary message (void *)
     */
    void SetReceivedTime(void* pMsg) override;
    /**
     * @brief E2E latency Client SetDecodeTime
     * Sets frame decode duration at Client into the message
     * Note: Reuses client_received_time
     * @param Input: pointer to latency binary message (void *)
     * @param Input: decode duration to set
     */
    void SetDecodeTime(void* pMsg, const uint64 decodeDuration) override;
    /**
     * @brief E2E latency Client SetRenderTime
     * Sets client render time into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: render duration to set
     */
    void SetRenderTime(void* pMsg, const uint64 renderDuration) override;
    /**
     * @brief E2E latency Client GetFrameId
     * Gets the client frame ID from the message
     * @param Input: pointer to latency binary message (void *)
     */
    uint32 GetFrameId(void* pMsg) const override;
    /**
     * @brief E2E latency Client GetInputTime
     * Gets the client input time from the message
     * @param Input: pointer to latency binary message (void *)
     */
    uint64 GetInputTime(void* pMsg) const override;
    /**
    * @brief E2E latency Client GetSendTime
    * Gets the client send time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetSendTime(void* pMsg) const override;
    /**
    * @brief E2E latency Client GetReceivedTime
    * Gets the client received time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetReceivedTime(void* pMsg) const override;
    /**
    * @brief E2E latency Client GetDecodeTime
    * Gets the client decode time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetDecodeTime(void* pMsg) const override;
    /**
    * @brief E2E latency Client GetRenderTime
    * Gets the client render time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetRenderTime(void* pMsg) const override;
    /**
    * @brief E2E latency Server FreeMessage
    * Frees the message
    * @param Input: pointer to latency binary message (void *)
    */
    void FreeMessage(void* pMsg) const override;

private:
    bool init_ = false;
    std::list<void*> latency_msg_list_; //list of latency messages, cleared when game session is destroyed.
};
}
