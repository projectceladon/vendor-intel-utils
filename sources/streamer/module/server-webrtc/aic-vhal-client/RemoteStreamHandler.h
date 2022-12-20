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

#ifndef __REMOTE_STREAM_HANDLER_H
#define __REMOTE_STREAM_HANDLER_H

#include <memory>
#include <string>

#include "AudioPlayer.h"
#include "owt/base/stream.h"

class RemoteStreamHandler {
 public:
  using CommandHandler = std::function<void(uint32_t cmd)>;
  RemoteStreamHandler(int local_peer_id, CommandHandler cmd_handler = nullptr);

  static const std::string startAudioRecMsg;
  static const std::string stopAudioRecMsg;
  static const std::string startAudioPlayMsg;
  static const std::string stopAudioPlayMsg;

  owt::base::AudioPlayerInterface &getAudioPlayer() const;

  bool HasActiveStream() const;

  void setStream(std::shared_ptr<owt::base::RemoteStream> remote);
  void resetStream();

  void subscribeForAudio();
  void unSubscribeForAudio();

  void subscribeForVideo();
  void unSubscribeForVideo();

 private:
  int mLocalPeerId;
  std::shared_ptr<owt::base::RemoteStream> mRemoteStream = nullptr;
  std::shared_ptr<AudioPlayer> mAudioPlayer = nullptr;
};

#endif
