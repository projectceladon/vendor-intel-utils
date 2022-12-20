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

#include <chrono>
#include <string>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using namespace std::chrono;

typedef uint64_t uint64;
typedef uint32_t uint32;

namespace IL
{
/**
* @brief E2ELatencyBase class, base class for Client and Server classes
* Prototypes to Create, Print and Parse a protobuf message (i.e. latency message)
* handled by the GA Client and GA Server.
* Either GA Server or Client adds timestamps to the protobuf message at various points of insertion,
* e.g. upon sending or receiving the message. The E2E latency is determined at the Client side,
* which originates the message and receives the updated message from the Server via a side data channel.
*/
class E2ELatencyBase
{
public:
    virtual ~E2ELatencyBase() {}

    virtual void Destroy()
    {
        delete this;
    }
    /**
     * @brief Virtual prototype for E2E latency CreateLatencyMsg
     * Returns nullptr by default
     */
    virtual void* CreateLatencyMsg() { return (nullptr); }
    /**
     * @brief Current time in nanoseconds
     * Returns current time in nanoseconds (long long)
     */
    static long long GetTimeInNanos()
    {
        auto now = high_resolution_clock::now();
        auto nanos = duration_cast<nanoseconds>(now.time_since_epoch()).count();
        return (nanos);
    }
    /**
     * @brief Virtual prototype to set frame ID
     * @param Input: pointer to binary message (void *)
     * @param Input: frame ID to set
     */
    virtual void SetFrameId(void* pMsg, const uint32 frameId) {}
    /**
     * @brief Virtual prototype to set currently processing frame ID
     * Mainly used in GA Server.
     * The latency message is sent from the Server when this frame is being processed.
     * @param Input: pointer to binary message (void *)
     * @param Input: frame ID to set
     */
    virtual void SetProcessingFrameId(void* pMsg, const uint32 frameId) {}
    /**
     * @brief Virtual prototype to set last processed frame ID
     * Mainly used in GA Server.
     * The latency message is received at the Server when this frame is being processed.
     * @param Input: pointer to binary message (void *)
     * @param Input: frame ID to set
     */
    virtual void SetLastProcessedFrameId(void* pMsg, const uint32 frameId) {}
    /**
     * @brief Virtual prototype to set input time
     * Mainly used in GA Client when a latency message is created.
     * @param Input: pointer to binary message (void *)
     */
    virtual void SetInputTime(void* pMsg) {}
    /**
     * @brief Virtual prototype to set input time
     * Used in both GA Client and GA Server when a latency message is sent.
     * @param Input: pointer to binary message (void *)
     */
    virtual void SetSendTime(void* pMsg) {}
    /**
     * @brief Virtual prototype to set input time
     * Used in both GA Client and GA Server when a latency message is received.
     * @param Input: pointer to binary message (void *)
     */
    virtual void SetReceivedTime(void* pMsg) {}
    /**
     * @brief Virtual prototype to set last processed frame ID
     * Mainly used in GA Server.
     * The client input time is set from the GA Server into the latency message.
     * @param Input: pointer to binary message (void *)
     * @param Input: timestamp to set
     */
    virtual void SetClientInputTime(void* pMsg, uint64 timestamp) {}
    /**
     * @brief Virtual prototype to set decode time
     * Sets frame decode duration at GA Client into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: decode duration to set
     */
    virtual void SetDecodeTime(void* pMsg, const uint64 decodeDuration) {}
    /**
     * @brief Virtual prototype to set render time
     * Sets frame render duration at GA Client or Server into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: render duration to set
     */
    virtual void SetRenderTime(void* pMsg, const uint64 renderDuration) {}
    /**
     * @brief Virtual prototype to set decode time
     * Sets frame encode duration at GA Server into the message
     * @param Input: pointer to latency binary message (void *)
     * @param Input: encode duration to set
     */
    virtual void SetEncodeTime(void* pMsg, const uint32 encodeDuration) {}
    /**
     * @brief Virtual prototype to get frame ID
     * Gets the GA Client or GA Server frame ID from the message
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint32 GetFrameId(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype to get processing frame ID
     * Gets the frame ID from the message when GA server sent the message out
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint32 GetProcessingFrameId(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype to get processing frame ID
     * Gets the frame ID from the message when GA server received the message
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint32 GetLastProcessedFrameId(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype to get input time
     * Gets the GA Client message input time from the message
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint64 GetInputTime(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype to get send time
     * Gets the GA Client or GA Server message send time from the message
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint64 GetSendTime(void* pMsg) const { return 0; }
    /**
    * @brief Virtual prototype to get received time
    * Gets the GA Client or GA Server message received time from the message
    * @param Input: pointer to latency binary message (void *)
    * Returns 0 by default.
    */
    virtual uint64 GetReceivedTime(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype to get decode time
     * Gets the GA Client frame decode time from the message
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint64 GetDecodeTime(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype to get render time
     * Gets the GA Client or GA Server frame render time from the message
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint64 GetRenderTime(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype to get encode time
     * Gets the GA Server frame encode time from the message
     * @param Input: pointer to latency binary message (void *)
     * Returns 0 by default.
     */
    virtual uint64 GetEncodeTime(void* pMsg) const { return 0; }
    /**
     * @brief Virtual prototype for PrintMessage
     * Used in both GA Client and GA Server
     * @param Input: pointer to binary message (void *)
     * Returns empty string by default.
     */
    virtual std::string PrintMessage(void* pMsg) { return ""; }
    /**
     * @brief Virtual prototype for ParseMessage
     * Used in both GA Client and GA Server
     * @param Input: std::string buffer
     * Returns nullptr (void *) by default
     */
    virtual void* ParseMessage(std::string buf) { return nullptr; }
    /**
     * @brief Virtual prototype for FreeMessage
     * Used in both GA Client and GA Server
     * @param Input: pointer to binary message (void *)
     */
    virtual void FreeMessage(void* pMsg) const {}
};
}
