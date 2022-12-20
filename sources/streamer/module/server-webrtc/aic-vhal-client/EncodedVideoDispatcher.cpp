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

#include "EncodedVideoDispatcher.h"
#include <string.h>
#include <errno.h>

bool
EncodedVideoDispatcher::OnEncodedFrame(std::unique_ptr<owt::base::VideoEncodedFrame> frame)
{

    // Write payload
    auto [sent, error_msg] =
          video_sink_->SendDataPacket(frame->buffer, frame->length);
    if (sent < 0) {
        ga_logger(Severity::ERR, "[video_capture] Failed to send frame of size %zu to Camera VHal\n", frame->length);
        return false;
    }
    ga_logger(Severity::DBG, "[video_capture] Successfully written encoded frame of size %zu to Camera VHal\n", frame->length);
    return true;
}
