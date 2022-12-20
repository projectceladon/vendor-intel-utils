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

#include "E2ELatencyClient.h"
#include "gae2elatency.pb.h"

#include "google/protobuf/util/json_util.h"

namespace IL
{
    /**
     * E2E latency Client class constructor
     */
    E2ELatencyClient::E2ELatencyClient()
    {
    }

    /**
     * E2E latency Client class destructor
     * Latency message is deleted upon calling destructor
     */
    E2ELatencyClient::~E2ELatencyClient()
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
     * E2E latency Client CreateLatencyMsg
     * Creates a latency message and pushes into message queue
     * This is called every event (e.g. key stroke)
     */
    void* E2ELatencyClient::CreateLatencyMsg()
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
     * E2E latency Client PrintMessage
     * Input: pointer to binary message (void *)
     * Output: content of message in std::string
     */
    std::string E2ELatencyClient::PrintMessage(void* pMsg)
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
     * E2E latency Client ParseMessage
     * Input: std::string buffer
     * Output: pointer to a new latency binary message (void *) containing the input buffer's content
     */
    void* E2ELatencyClient::ParseMessage(std::string buf)
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
     * E2E latency Client SetFrameId
     * Sets client frame ID into the message
     */
    void E2ELatencyClient::SetFrameId(void* pMsg, const uint32 frameId)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_client_frame_id(frameId);
        }
    }

    /**
     * E2E latency Client SetInputTime
     * Sets client input time into the message
     */
    void E2ELatencyClient::SetInputTime(void* pMsg)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_client_input_time(GetTimeInNanos());
        }
    }

    /**
     * E2E latency Client SetSendTime
     * Sets client send time into the message
     */
    void E2ELatencyClient::SetSendTime(void* pMsg)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_client_send_time(GetTimeInNanos());
        }
    }

    /**
     * E2E latency Client SetReceivedTime
     * Sets client received time into the message
     */
    void E2ELatencyClient::SetReceivedTime(void* pMsg)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_client_received_time(GetTimeInNanos());
        }
    }

    /**
     * E2E latency Client SetDecodeTime
     * Sets frame decode duration at Client into the message
     */
    void E2ELatencyClient::SetDecodeTime(void* pMsg, const uint64 decodeDuration)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_client_decode_time(decodeDuration);
        }
    }

    /**
     * E2E latency Client SetRenderTime
     * Sets client render time into the message
     */
    void E2ELatencyClient::SetRenderTime(void* pMsg, const uint64 renderDuration)
    {
        if (pMsg)
        {
            E2ELatency::LatencyMsg* curtMsg = (E2ELatency::LatencyMsg*)pMsg;
            curtMsg->set_client_render_time(renderDuration);
        }
    }

    /**
     * E2E latency Client GetFrameId
     * Gets the client frame ID from the message
     */
    uint32 E2ELatencyClient::GetFrameId(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->client_frame_id();
    }

    /**
     * E2E latency Client GetInputTime
     * Gets the client input time from the message
     */
    uint64 E2ELatencyClient::GetInputTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->client_input_time();
    }

    /**
    * E2E latency Client GetSendTime
    * Gets the client send time from the message
    */
    uint64 E2ELatencyClient::GetSendTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->client_send_time();
    }

    /**
    * E2E latency Client GetReceivedTime
    * Gets the client received time from the message
    */
    uint64 E2ELatencyClient::GetReceivedTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->client_received_time();
    }

    /**
    * E2E latency Client GetDecodeTime
    * Gets the client decode time from the message
    */
    uint64 E2ELatencyClient::GetDecodeTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->client_decode_time();
    }

    /**
    * E2E latency Client GetRenderTime
    * Gets the client render time from the message
    */
    uint64 E2ELatencyClient::GetRenderTime(void* pMsg) const
    {
        return ((E2ELatency::LatencyMsg*)pMsg)->client_render_time();
    }

    /**
    * E2E latency Client FreeMessage
    * Free the message
    */
    void E2ELatencyClient::FreeMessage(void* pMsg) const
    {
        if (pMsg)
        {
            delete (E2ELatency::LatencyMsg*)pMsg;
        }
    }

}
