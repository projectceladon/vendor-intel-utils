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

#include "E2ELatencyServer.h"
#include "gae2elatency.pb.h"

#include "google/protobuf/util/json_util.h"

namespace IL
{
    /**
     * E2E latency Server class constructor
     */
    E2ELatencyServer::E2ELatencyServer()
    {
    }

    /**
    * E2E latency Server class destructor
    */
    E2ELatencyServer ::~E2ELatencyServer()
    {
        for (auto&& pMsg : latency_msg_list_)
        {
            if (pMsg)
            {
                delete (E2ELatency::LatencyMsg*)pMsg;
            }
        }
        latency_msg_list_.clear();
    }

    /**
     * E2E latency Server CreateLatencyMsg
     * Note: Typically latency message is created in Client side
     * This method allow server side latency message creation for debug purposes
     * Creates a latency message and pushes into message queue
     */
    void* E2ELatencyServer::CreateLatencyMsg()
    {
        E2ELatency::LatencyMsg* pMsg = new E2ELatency::LatencyMsg();

        if (pMsg)
        {
            pMsg->set_client_msg_create_time(GetTimeInNanos());
            latency_msg_list_.push_back(pMsg);
        }
        else
        {
            pMsg = nullptr;
        }

        return (pMsg);
    }

    /**
     * E2E latency Server PrintMessage
     * Server version of PrintMessage
     * Input: pointer to binary message (void *)
     * Output: content of message in std::string
     */
    std::string E2ELatencyServer::PrintMessage(void* pMsg)
    {
        std::string buffer = "";

        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;

            google::protobuf::util::JsonPrintOptions options;
            options.always_print_primitive_fields = false;
            options.preserve_proto_field_names = false;

            auto status = MessageToJsonString(*curtMsg, &buffer, options);
            if (!status.ok())
            {
                throw std::runtime_error(status.ToString());
            }
        }

        return buffer;
    }

    /**
     * E2E latency Server ParseMessage
     * Input: std::string buffer
     * Output: pointer to a new latency binary message (void *) containing the input buffer's content
     */
    void* E2ELatencyServer::ParseMessage(std::string buf)
    {
        if (!buf.empty())
        {
            E2ELatency::LatencyMsg* pMsg = new E2ELatency::LatencyMsg;

            if (pMsg) {
                google::protobuf::util::JsonParseOptions options;
                options.ignore_unknown_fields = false;

                auto status = JsonStringToMessage(buf, pMsg, options);
                // ignore parsing status. We just need the pointer, even if the value is nullptr.
            }
            else {
                pMsg = nullptr;
            }

            return (pMsg);
        }
        else
        {
            return nullptr;
        }
    }

    /**
     * E2E latency Server SetClientInputTime
     * Sets client input time into the server side message
     * Note: client input time comes to the server side via JSON message associated with keyboard event
     */
    void E2ELatencyServer::SetClientInputTime(void* pMsg, uint64 timestamp)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_client_input_time(timestamp);
        }
    }

    /**
     * E2E latency Server SetReceivedTime
     * Sets server received time into the message
     */
    void E2ELatencyServer::SetReceivedTime(void* pMsg)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_server_received_time(GetTimeInNanos());
        }
    }

    /**
     * E2E latency Server SetInputProcessedTime
     * Sets server input processed time into the message
     */
    void E2ELatencyServer::SetInputProcessedTime(void* pMsg)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_server_input_processed_time(GetTimeInNanos());
        }
    }

    /**
     * E2E latency Server SetProcessingFrameId
     * Sets server currently processing frame ID into the message
     * This is set when a frame is sent out from the server
     */
    void E2ELatencyServer::SetProcessingFrameId(void* pMsg, const uint32 frameId)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_server_processing_frame_id(frameId);
        }
    }

    /**
     * E2E latency Server SetLastProcessedFrameId
     * Sets server last processed frame ID into the message
     * This is set when a keyboard event is received at the server
     */
    void E2ELatencyServer::SetLastProcessedFrameId(void* pMsg, const uint32 frameId)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_server_last_processed_frame_id(frameId);
        }
    }

    /**
     * E2E latency Server SetRenderTime
     * Sets server render time into the message
     */
    void E2ELatencyServer::SetRenderTime(void* pMsg, const uint64 renderDuration)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_server_render_time(renderDuration);
        }
    }

    /**
     * E2E latency Server SetEncodeTime
     * Sets frame encode time into the message
     */
    void E2ELatencyServer::SetEncodeTime(void* pMsg, const uint32 encodeDuration)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_server_encode_time(encodeDuration);
        }
    }

    /**
     * E2E latency Server SetSendTime
     * Sets server send time into the message
     */
    void E2ELatencyServer::SetSendTime(void* pMsg)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_server_send_time(GetTimeInNanos());
        }
    }

    /**
     * E2E latency Server GetReceivedTime
     * Get server received time from the message
     */
    uint64 E2ELatencyServer::GetReceivedTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->server_received_time();
    }

    /**
     * E2E latency Server GetInputProcessedTime
     * Get server input processed time from the message
     */
    uint64 E2ELatencyServer::GetInputProcessedTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->server_input_processed_time();
    }

    /**
     * E2E latency Server GetProcessingFrameId
     * Get server processing frame ID from the message
     */
    uint32 E2ELatencyServer::GetProcessingFrameId(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->server_processing_frame_id();
    }

    /**
     * E2E latency Server GetLastProcessedFrameId
     * Get server last processed frame ID from the message
     */
    uint32 E2ELatencyServer::GetLastProcessedFrameId(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->server_last_processed_frame_id();
    }

    /**
     * E2E latency Server GetRenderTime
     * Get server render time from the message
     */
    uint64 E2ELatencyServer::GetRenderTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->server_render_time();
    }

    /**
     * E2E latency Server GetEncodeTime
     * Get encode time from the message
     */
    uint64 E2ELatencyServer::GetEncodeTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->server_encode_time();
    }

    /**
     * E2E latency Server GetSendTime
     * Get server send time from the message
     */
    uint64 E2ELatencyServer::GetSendTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->server_send_time();
    }

    /**
    * E2E latency Server FreeMessage
    * Free the message
    */
    void E2ELatencyServer::FreeMessage(void* pMsg) const
    {
        if (pMsg)
        {
            delete (E2ELatency::LatencyMsg*)pMsg;
        }
    }

}
