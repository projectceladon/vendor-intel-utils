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
 * @brief AudioPlayer implements owt::base::AudioPlayerInterface
 *        which receives OnData() callback when audio record
 *        beigns.
 */

#include "android-common.h"
#include "AudioPlayer.h"
#include "ga-common.h"
#include "libvhal_common.h"
#include "android_audio_core.h"
#include <cstring>
#include <functional>
#include <tuple>

#undef TAG
#define TAG "[capture]"

using namespace std::chrono;

AudioPlayer::AudioPlayer(int instance_id, CommandHandler CmdHandler)
{
    mCmdHandler = CmdHandler;

    vhal::client::TcpConnectionInfo conn_info = { android::ip() };

    auto callback = [&](const vhal::client::audio::CtrlMessage& ctrl_msg) {
        switch (ctrl_msg.cmd) {
        case vhal::client::audio::Command::kOpen:
            ga_logger(Severity::INFO, TAG " audio socket cmd is CommandOpen\n");
            mAudioConfig.sampleRate = ctrl_msg.asci.sample_rate;
            mAudioConfig.channelCount = ctrl_msg.asci.channel_count;
            mAudioConfig.bufferSizeInBytes = ctrl_msg.asci.frame_count *
                                         mAudioConfig.channelCount *
                                         audio_bytes_per_sample(ctrl_msg.asci.format);

            if (!mSockBuffer)
                mSockBuffer = new uint16_t[mAudioConfig.bufferSizeInBytes];

            ga_logger(Severity::INFO,
                TAG " audio channel_count:%d | format:%d | frame_count:%d | "
                "sample_rate:%d | buffer_size:%zu\n",
                ctrl_msg.asci.channel_count, ctrl_msg.asci.format, ctrl_msg.asci.frame_count,
                ctrl_msg.asci.sample_rate, mAudioConfig.bufferSizeInBytes);

            mCmdHandler(static_cast<uint32_t>(ctrl_msg.cmd));
            break;
        case vhal::client::audio::Command::kClose:
            ga_logger(Severity::INFO, TAG " audio socket cmd is CommandClose\n");
            mCmdHandler(static_cast<uint32_t>(ctrl_msg.cmd));
            break;
        default:
            ga_logger(Severity::INFO,
                TAG "Unknown Command received, exiting with failure\n");
            break;
        }
    };
    int user_id = -1;
    if (ga_conf_readbool("enable-multi-user", 0) != 0) {
        user_id = ga_conf_readint("user");
    }
    audio_sink = std::make_unique<vhal::client::audio::AudioSink>(conn_info, callback, user_id);
}

AudioPlayer::~AudioPlayer() {
    delete [] mSockBuffer;
    mSockBuffer = nullptr;
}

void AudioPlayer::MonoToStereo(
    const uint16_t *in_L,  // mono input buffer (left channel)
    const uint16_t *in_R,  // mono input buffer (right channel)
    size_t num_samples)    // number of samples
{
  for (size_t i = 0; i < num_samples; ++i) {
    mSockBuffer[i * 2] = in_L[i];
    mSockBuffer[i * 2 + 1] = in_R[i];
  }
}

void AudioPlayer::OnData(const void *audio_data, int bits_per_sample,
                         int sample_rate, size_t number_of_channels,
                         size_t number_of_frames) {
    ga_logger(Severity::DBG, TAG "WriteData() number_of_frames: %d\n", number_of_frames);
    const uint16_t *data = reinterpret_cast<const uint16_t *>(audio_data);

    const auto sizeInBytes =
        number_of_frames * number_of_channels * bits_per_sample / 8;

    if (mAudioConfig.channelCount == number_of_channels) {
        ga_logger(Severity::DBG, TAG "Same channelCount, copying Original data\n");
        memcpy(mSockBuffer, data, sizeInBytes);
    } else if ((number_of_channels == 1) && (mAudioConfig.channelCount == 2)) {
        ga_logger(Severity::DBG, TAG "Calling MonoToStereo \n");
        MonoToStereo(data, data, number_of_frames * number_of_channels);
    } else {
        ga_logger(Severity::ERR, TAG "Channel conversion not supported\n");
    }

    auto [sent, error_msg] = audio_sink->SendDataPacket(reinterpret_cast<const uint8_t *>(mSockBuffer), sizeInBytes);
    if (sent < 0) {
        ga_logger(Severity::ERR, TAG "SendDataPacket() failed, res: %lu\n", sent);
    }
    else {
        ga_logger(Severity::DBG, TAG "SendDataPacket() success, sent: %lu\n", sent);
    }
}
