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

#include <future>

#include "p2p-socket-signaling-channel.h"

#ifdef WIN32
#include "ga-controller-sdl.h"
#include "ga-audio-input.h"
#else
#include "ga-controller-android.h"
#endif
#include "ga-video-input.h"
#include "ga-module.h"

#ifndef WIN32
#include "aic-vhal-client/RemoteStreamHandler.h"
#include "aic-vhal-client/EncodedVideoDispatcher.h"
#include "aic-vhal-client/SensorHandler.h"
#include "libvhal/virtual_gps_receiver.h"
#include "aic-vhal-client/CommandChannelHandler.h"
#include "aic-vhal-client/CameraClientHandler.h"
#endif

#include "owt/base/clock.h"
#include "owt/base/globalconfiguration.h"
#include "owt/p2p/p2pclient.h"
#ifdef E2ELATENCY_TELEMETRY_ENABLED
#include "E2ELatencyServer.h"
#include "E2ELatencyFactory.h"
#ifndef WIN32
#include <climits>
#endif
#endif

using namespace owt::base;
using namespace owt::p2p;

namespace ga {
namespace webrtc {
class ICSP2PClient : public owt::p2p::P2PClientObserver,
                     public owt::base::EncoderObserver,
                     public PublicationObserver,
                     public std::enable_shared_from_this<ga ::webrtc ::ICSP2PClient> {
 public:
  ~ICSP2PClient() = default;

  ICSP2PClient& operator=(const ICSP2PClient&) = delete;
#ifdef WIN32
  GAAudioFrameGenerator* audioGenerator;
#endif

  int Init(void *arg);
  void Deinit();
  int Start();
  void InsertFrame(ga_packet_t *packet);
  void SendCursor(std::shared_ptr<CURSOR_DATA> cursor_data);
  void SendQoS(std::shared_ptr<QosInfo> qos_info);
  // Returns the bytes sent by the server since last call.
  int64_t GetCreditBytes();
  int64_t GetMaxBitrate();
  //Encoder observer impl.
  void OnStarted();
  void OnStopped();
  void OnKeyFrameRequest();
  void OnRateUpdate(uint64_t bitrate_bps, uint32_t frame_rate);
  // Publication observer
  void OnEnded();
  void OnMute(TrackKind track_kind) {}
  void OnUnmute(TrackKind track_kind) {}
  virtual void OnError(std::unique_ptr<Exception> failure) {}
  uint32_t GetConnectedClientCount() const { return connected_client_count; }
#ifdef E2ELATENCY_TELEMETRY_ENABLED
  // E2Elatency
  IL::E2ELatencyServer * LatencyServer() {
      return reinterpret_cast<IL::E2ELatencyServer*>(CreateE2ELatencyServer());
  }

    void ReleaseServer(IL::E2ELatencyServer * latencyServerInstance) {
        if (latencyServerInstance) {
            delete latencyServerInstance;
            latencyServerInstance = nullptr;
        }
    }

    unsigned int updateFrameNumber() {
        if (frameNumber > ((1ULL << sizeof(unsigned int) * CHAR_BIT) - 1))
            frameNumber = 0; // roll-over
        else
            ++frameNumber; // increment

        return frameNumber;
    }

    unsigned int getFrameNumber() {
        return frameNumber;
    }

    void HandleLatencyMessage(const std::string &message);
#endif
protected:
  virtual void OnMessageReceived(const std::string &remote_user_id,
                              const std::string message) override;
  virtual void OnStreamAdded(std::shared_ptr<owt::base::RemoteStream> stream) override;
  virtual void OnStreamStopped(const std::string& remote_user_id) override;
  virtual void OnLossNotification(DependencyNotification notification) override {};

private:
  void RegisterCallbacks();
  void ConnectCallback(bool is_fail, const std::string &error);
  void CreateStream();
  void RequestCursorShape();

  std::shared_ptr<owt::p2p::P2PClient> p2pclient_;
  std::shared_ptr<owt::base::LocalStream> local_stream_;
  std::shared_ptr<owt::base::LocalStream> local_audio_stream;
  std::shared_ptr<owt::p2p::Publication> publication_;
#ifndef WIN32
  std::shared_ptr<RemoteStreamHandler> mRemoteStreamHandler;
  std::unique_ptr<SensorHandler> mSensorHandler;
  std::shared_ptr<CameraClientHandler> mCameraClientHandler;
  std::unique_ptr<VirtualGpsReceiver> mVirtualGpsReceiver;
  std::unique_ptr<CommandChannelHandler> mCommandChannelHandler;
#endif
  std::shared_ptr<owt::base::EncodedStreamProvider> stream_provider_;
  std::promise<int> connect_status_;
  std::shared_ptr<GAVideoEncoder> ga_encoder_;
  std::unique_ptr<Controller> controller_;
  int64_t bytes_sent_on_last_stat_call_;
  int64_t bytes_sent_on_last_credit_call_;
  int64_t credit_bytes_;
  int64_t current_available_bandwidth_;
  std::string remote_user_id_;
  bool streaming_;
  unsigned char cursor_shape_[4096];  // The latest cursor shape.
  bool first_cursor_info_;
  bool capturer_started_ = false;
  std::unique_ptr<owt::base::Clock> clock_;
  bool enable_dump_ = false;
  FILE* dump_file_ = nullptr;
  uint64_t last_timestamp_;
  uint64_t send_failures_ = 0;
  bool send_blocked_ = true;
#ifdef E2ELATENCY_TELEMETRY_ENABLED
  // E2ELatency
  void* gpMsg = nullptr;
  static unsigned int frameNumber;
  uint64_t gClientTimestamp = 0;
  unsigned int currentFrame = 0;
#endif
  bool enable_render_drc = false;
  uint32_t connected_client_count = 0;
  std::function<void(uint32_t)> pHookClientStatus;
};
} // namespace webrtc
} // namespace ga
