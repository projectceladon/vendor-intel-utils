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
#include <string>
#include <list>

typedef uint64_t uint64;
typedef uint32_t uint32;

namespace IL
{
/**
* @brief E2ELatencyServer class, derived from E2ELatencyBase
* Print and Parse a protobuf message (i.e. latency message) that is communicated with the GA Client.
* (Creating a message in the GA Server is optional, and only used for debug purposes).
* Either GA Server or Client adds timestamps to the protobuf message at various points of insertion,
* e.g. upon sending or receiving the message. The E2E latency is determined at the Client side,
* which originates the message and receives the updated message from the Server via a side data channel.
*/
class E2ELatencyServer : public E2ELatencyBase
{
public:
    /**
     * @brief E2E latency Server class constructor
     */
    E2ELatencyServer();
    /**
     * @brief E2E latency Server class destructor
     * Latency message is deleted upon calling destructor
     */
    virtual ~E2ELatencyServer();
    /**
     * @brief E2E latency Server CreateLatencyMsg (for debug only) - normally created from GA Client
     * Creates a latency message and pushes into message queue
     * This is called every event (e.g. key stroke)
     */
    void* CreateLatencyMsg();
    /**
     * @brief E2E latency Server PrintMessage
     * @param Input: pointer to binary message (void *)
     * Output: content of message in std::string
     */
    std::string PrintMessage(void* pMsg) override;
    /**
     * @brief E2E latency Server ParseMessage
     * @param Input: std::string buffer
     * Output: pointer to a new latency binary message (void *) containing the input buffer's content
     */
    void* ParseMessage(std::string buf) override;
    /**
     * @brief E2E latency Server SetReceivedTime
     * Sets server received time into the message
     * @param Input: pointer to latency binary message (void *)
     */
    void SetReceivedTime(void* pMsg) override;
    /**
     * @brief E2E latency Server SetInputProcessedTime
     * Sets server input processed time into the message
     * @param Input: pointer to latency binary message (void *)
     */
    void SetInputProcessedTime(void* pMsg);
    /**
     * @brief E2E latency server to set currently processing frame ID
     * Mainly used in GA Server.
     * The latency message is sent from the Server when this frame is being processed.
     * @param Input: pointer to binary message (void *)
     * @param Input: frame ID to set
     */
    void SetProcessingFrameId(void* pMsg, const uint32 frameId) override;
    /**
     * @brief E2E latency server to set last processed frame ID
     * Mainly used in GA Server.
     * The latency message is received at the Server when this frame is being processed.
     * @param Input: pointer to binary message (void *)
     * @param Input: frame ID to set
     */
    void SetLastProcessedFrameId(void *pMsg, const uint32 frameId) override;
    /**
     * @brief E2E latency server to set render time
     * Sets frame render duration at GA Server into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: render duration to set
     * Note: currently not using the override, to be fixed later.
     */
    void SetRenderTime(void *pMsg, const uint64 renderDuration) override;
    /**
     * @brief E2E latency server to set encode time
     * Sets frame encode duration at GA Server into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: encode duration to set
     */
    void SetEncodeTime(void* pMsg, const uint32 encodeDuration) override;
    /**
     * @brief E2E latency Server SetSendTime
     * Sets server send time into the message
     * @param Input: pointer to latency binary message (void *)
     */
    void SetSendTime(void* pMsg) override;
    /**
     * @brief E2E latency Server to set the Client's input time
     * Sets client input time from GA server into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: timestamp in nanoseconds received from the client
     */
    void SetClientInputTime(void* pMsg, uint64 timestamp) override;
    /**
    * @brief E2E latency Server GetReceivedTime
    * Gets the server received time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetReceivedTime(void* pMsg) const override;
    /**
    * @brief E2E latency Server GetInputProcessedTime
    * Gets the server input processed time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetInputProcessedTime(void* pMsg) const;
    /**
    * @brief E2E latency Server GetProcessingFrameId
    * Gets the frame ID when the server is sending out the frame from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint32 GetProcessingFrameId(void* pMsg) const override;
    /**
    * @brief E2E latency Server GetLastProcessedFrameId
    * Gets the frame ID when the server has received the message from client
    * @param Input: pointer to latency binary message (void *)
    */
    uint32 GetLastProcessedFrameId(void* pMsg) const override;
    /**
    * @brief E2E latency Server GetRenderTime
    * Gets the server side render time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetRenderTime(void* pMsg) const override;
    /**
    * @brief E2E latency Server GetEncodeTime
    * Gets the frame encode time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetEncodeTime(void* pMsg) const override;
    /**
    * @brief E2E latency Server GetSendTime
    * Gets the server side send time from the message
    * @param Input: pointer to latency binary message (void *)
    */
    uint64 GetSendTime(void* pMsg) const override;
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
