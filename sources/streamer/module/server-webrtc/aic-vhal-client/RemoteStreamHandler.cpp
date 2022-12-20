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
 * @brief Handles lifecycle of a Remote stream.
 */

#include "RemoteStreamHandler.h"
#include "ga-common.h"

const std::string RemoteStreamHandler::startAudioRecMsg =
    "{ \"key\" : \"start-audio-rec\" }";
const std::string RemoteStreamHandler::stopAudioRecMsg =
    "{ \"key\" : \"stop-audio-rec\" }";
const std::string RemoteStreamHandler::startAudioPlayMsg =
    "{ \"key\" : \"start-audio-play\" }";
const std::string RemoteStreamHandler::stopAudioPlayMsg =
    "{ \"key\" : \"stop-audio-play\" }";

RemoteStreamHandler::RemoteStreamHandler(int local_peer_id,
                                         CommandHandler cmd_handler)
    : mLocalPeerId{local_peer_id},
      mAudioPlayer{std::make_shared<AudioPlayer>(local_peer_id, cmd_handler)} {}

owt::base::AudioPlayerInterface &RemoteStreamHandler::getAudioPlayer() const {
  return *mAudioPlayer.get();
}

bool RemoteStreamHandler::HasActiveStream() const {
  ga_logger(Severity::INFO, "[capture] Has active stream: %s\n",
          mRemoteStream ? "yes" : "no");
  return mRemoteStream ? true : false;
}

void RemoteStreamHandler::setStream(
    std::shared_ptr<owt::base::RemoteStream> remote) {
  mRemoteStream = remote;
}

void RemoteStreamHandler::resetStream() { mRemoteStream.reset(); }

void RemoteStreamHandler::subscribeForAudio() {
  mRemoteStream->AttachAudioPlayer(getAudioPlayer());
}

void RemoteStreamHandler::unSubscribeForAudio() {
  mRemoteStream->DetachAudioPlayer();
}
void RemoteStreamHandler::subscribeForVideo() {
  ga_logger(Severity::ERR, "[CAM] This API shouldn't be used for H264 frames\n");
}
void RemoteStreamHandler::unSubscribeForVideo() {
  mRemoteStream->DetachVideoRenderer();
}
