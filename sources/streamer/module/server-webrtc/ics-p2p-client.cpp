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

#include "ics-p2p-client.h"
#include "ga-video-input.h"
#include "ga-conf.h"
#include "ga-cursor.h"
#include "ga-qos.h"
#include "encoder-common.h"
#ifdef WIN32
#include "ga-audio-input.h"
#else
#include "android-common.h"
#include "audio-frame-generator.h"
#include "audio_common.h"
#include "video_sink.h"
#endif
#include "rtspconf.h"
#include <iostream>

#include "owt/base/deviceutils.h"
#include "owt/base/stream.h"
#include "owt/base/logging.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include "nlohmann/json.hpp"

#ifdef WIN32
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "msdmo.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#endif

using json = nlohmann::json;
using namespace ga::webrtc;
#ifdef E2ELATENCY_TELEMETRY_ENABLED
//Latency message using google protobuf
IL::E2ELatencyServer * gLatencyServerInstance = nullptr;
#endif

// If send fails 100 times consecutively, block sending
// cursor and QoS info until receiving further message from
// client.
#define OWT_MAX_SEND_FAILURES 100

static const bool g_bEnableOwtStats = false;

static std::string get_p2p_server() {
  std::string host = ga_conf_readstr("signaling-server-host");
  std::string port = ga_conf_readstr("signaling-server-port");

  if (host.empty()) {
    host = "127.0.0.1";
    ga_logger(Severity::INFO, "*** no signaling server host specified, default to %s.\n", host.c_str());
  }
  if (port.empty()) {
    port = "8095";
    ga_logger(Severity::INFO, "*** no signaling server port specified, default to %s.\n", port.c_str());
  }

  std::string p2p = "http://" + host + ":" + port;
  return p2p;
}

static int get_android_session() {
  int session = -1;
  if (ga_conf_readbool("k8s", 0) == 0) {
    session = ga_conf_readint("android-session");
    if (session < 0)
      session = 0;
  }
  return session;
}

void ICSP2PClient::RegisterCallbacks() {
#ifndef WIN32
  int session = get_android_session();

  auto cmd_handler = [this](uint32_t cmd) {
    if (cmd == vhal::client::audio::Command::kOpen) {
      p2pclient_->Send(remote_user_id_, RemoteStreamHandler::startAudioRecMsg.c_str(),
                       nullptr, nullptr);
      ga_logger(Severity::INFO, "RemoteCmd Send message: %s\n",
              RemoteStreamHandler::startAudioRecMsg.c_str());

    } else if (cmd == vhal::client::audio::Command::kClose &&
               mRemoteStreamHandler->HasActiveStream()) {
      p2pclient_->Send(remote_user_id_, RemoteStreamHandler::stopAudioRecMsg.c_str(),
                       nullptr, nullptr);
      mRemoteStreamHandler->unSubscribeForAudio();
      mRemoteStreamHandler->resetStream();
      ga_logger(Severity::INFO, "RemoteCmd Send message: %s\n",
              RemoteStreamHandler::stopAudioRecMsg.c_str());
    } else if (cmd == vhal::client::audio::Command::kStartstream) {
      p2pclient_->Send(remote_user_id_, RemoteStreamHandler::startAudioPlayMsg.c_str(),
                       nullptr, nullptr);
      ga_logger(Severity::INFO, "RemoteCmd Send message: %s\n",
              RemoteStreamHandler::startAudioPlayMsg.c_str());

    } else if (cmd == vhal::client::audio::Command::kStopstream) {
      p2pclient_->Send(remote_user_id_, RemoteStreamHandler::stopAudioPlayMsg.c_str(),
                       nullptr, nullptr);
      ga_logger(Severity::INFO, "RemoteCmd Send message: %s\n",
              RemoteStreamHandler::stopAudioPlayMsg.c_str());
    } else if (cmd == (uint32_t)vhal::client::VideoSink::camera_cmd_t::CMD_OPEN) {
      ga_logger(Severity::INFO, "RemoteCmd Send message: %s\n",
              mCameraClientHandler->startPreviewStreamMsg.c_str());
      p2pclient_->Send(remote_user_id_, mCameraClientHandler->startPreviewStreamMsg.c_str(),
                       nullptr, nullptr);

    } else if (cmd == (uint32_t)vhal::client::VideoSink::camera_cmd_t::CMD_CLOSE) {
      ga_logger(Severity::INFO, "RemoteCmd Send message: %s\n",
              mCameraClientHandler->stopPreviewStreamMsg.c_str());
      p2pclient_->Send(remote_user_id_, mCameraClientHandler->stopPreviewStreamMsg.c_str(),
                       nullptr, nullptr);

    } else if (cmd == SensorHandler::Command::kSensorStart) {
      ga_logger(Severity::INFO, "Send message: %s\n",
           SensorHandler::sensorStartMsg.c_str());
      p2pclient_->Send(remote_user_id_, SensorHandler::sensorStartMsg.c_str(),
                       nullptr, nullptr);
    } else if (cmd == SensorHandler::Command::kSensorStop) {
      ga_logger(Severity::INFO, "Send message: %s\n",
              SensorHandler::sensorStopMsg.c_str());
      p2pclient_->Send(remote_user_id_, SensorHandler::sensorStopMsg.c_str(),
                       nullptr, nullptr);
    } else if (cmd == VirtualGpsReceiver::Command::kGpsStart) {
      ga_logger(Severity::INFO, "Send message: %s\n",
              VirtualGpsReceiver::gpsStartMsg.c_str());
      p2pclient_->Send(remote_user_id_, VirtualGpsReceiver::gpsStartMsg.c_str(),
                       nullptr, nullptr);
    } else if (cmd == VirtualGpsReceiver::Command::kGpsStop) {
      ga_logger(Severity::INFO, "Send message: %s\n",
              VirtualGpsReceiver::gpsStopMsg.c_str());
      p2pclient_->Send(remote_user_id_, VirtualGpsReceiver::gpsStopMsg.c_str(),
                       nullptr, nullptr);
    } else if (cmd == VirtualGpsReceiver::Command::kGpsQuit) {
      ga_logger(Severity::INFO, "Send message: %s\n",
              VirtualGpsReceiver::gpsQuitMsg.c_str());
      p2pclient_->Send(remote_user_id_, VirtualGpsReceiver::gpsQuitMsg.c_str(),
                       nullptr, nullptr);
    } else {
      // not sure now
    }
  };
  std::unique_ptr<AudioFrameGenerator> generator(
    AudioFrameGenerator::Create(get_android_session(), cmd_handler));
  GlobalConfiguration::SetCustomizedAudioInputEnabled(true,
                                                      std::move(generator));
  mRemoteStreamHandler = std::make_shared<RemoteStreamHandler>(session, cmd_handler);
  ga_logger(Severity::INFO, "RemoteStreamHandler Created !!!.\n");
  mSensorHandler = std::make_unique<SensorHandler>(session, cmd_handler);
  ga_logger(Severity::INFO, "SensorHandler Created !!!.\n");

  vhal::client::TcpConnectionInfo conn_info = { android::ip() };

  mVirtualGpsReceiver = std::make_unique<VirtualGpsReceiver>(conn_info, cmd_handler);
  ga_logger(Severity::INFO, "VirtualGpsReceiver Created !!!.\n");

  auto encodedVideoDispatcher = std::make_unique<EncodedVideoDispatcher>(session, cmd_handler);
  mCameraClientHandler = encodedVideoDispatcher->GetCameraClientHandler();
  GlobalConfiguration::SetCustomizedVideoDecoderEnabled(std::move(encodedVideoDispatcher));
  ga_logger(Severity::INFO, "SetCustomizedVideoDecoderEnabled !!!.\n");

  auto cmd_channel_msg_handler =
    [this](vhal::client::MsgType type, const std::string& msg) {
    if (msg.empty())
      return;

    std::string msg_json;
    if (type == vhal::client::MsgType::kActivityMonitor) {
      msg_json = "{\"key\":\"activity-switch\",\"val\":\"" + msg + "\"}";
    } else if (type == vhal::client::MsgType::kAicCommand) {
      msg_json = "{\"key\":\"cmd-output\",\"val\":" + msg + "}";
    }
    p2pclient_->Send(remote_user_id_, msg_json.c_str(),
                     nullptr, nullptr);
  };
  mCommandChannelHandler =
    std::make_unique<CommandChannelHandler>(session, cmd_channel_msg_handler);
#endif
}

#ifdef E2ELATENCY_TELEMETRY_ENABLED
//E2ELatency tracks frame number
unsigned int ICSP2PClient::frameNumber = 0;

void ICSP2PClient::HandleLatencyMessage(const std::string &message) {
  ga_logger(Severity::DBG, "ics-p2p-client: HandleLatencyMessage: latency message: %s\n",
      message.c_str());

  if (gClientTimestamp > 0 && gLatencyServerInstance != nullptr) {
    if (gpMsg) {
      return;
    }

    gpMsg = gLatencyServerInstance->ParseMessage(message);
    gLatencyServerInstance->SetClientInputTime(gpMsg, gClientTimestamp);
    gLatencyServerInstance->SetReceivedTime(gpMsg);

    currentFrame = getFrameNumber();
    if (Severity::DBG == ga_get_loglevel()) {
      gLatencyServerInstance->SetLastProcessedFrameId(gpMsg, currentFrame);
    }

    std::string msgString = gLatencyServerInstance->PrintMessage(gpMsg);
    ga_logger(Severity::DBG, "ics-p2p-client: HandleLatencyMessage: Frame %u, Latency message: %s\n",
        currentFrame, msgString.c_str());
  }
}
#endif

int ICSP2PClient::Init(void *arg) {
  memset(cursor_shape_, 0, sizeof(cursor_shape_));
  first_cursor_info_ = true;
  streaming_ = false;

#ifdef E2ELATENCY_TELEMETRY_ENABLED
  //E2ELatency server instance
  if (gLatencyServerInstance == nullptr) {
      gLatencyServerInstance = LatencyServer();
      if (gLatencyServerInstance == nullptr) {
          // No latency server instance - proceed, but don't use latency messages
          ga_logger(Severity::WARNING, "ics-p2p-client: Init: failed to create latency server instance.\n");
      }
      else {
          ga_logger(Severity::INFO, "ics-p2p-client: Init: created latency server instance.\n");
      }
  }
  //end E2ELatency
#endif

  unsigned short game_width = 1280;
  unsigned short game_height = 720;
#ifdef WIN32
  if (arg != NULL) {
    struct ServerConfig *webrtcCfg = (struct ServerConfig*) arg;
    if (webrtcCfg->pHookClientStatus != NULL) {
      pHookClientStatus = webrtcCfg->pHookClientStatus;
    }
    struct gaRect *rect = (struct gaRect*) webrtcCfg->prect;
    game_width = rect->right - rect->left + 1;
    game_height = rect->bottom - rect->top + 1;
  }
#else
  if (ga_conf_readbool("measure-latency", 0) == 1) {
    android::atrace_init();
  }
#endif

#ifdef WIN32
  controller_ = std::make_unique<SdlController>(game_width, game_height, 480, 320);
#else
  controller_ = std::make_unique<AndroidController>(game_width, game_height, 480, 320);
#endif

  // Handle webrtc signaling related settings
  std::string server_peer_id = ga_conf_readstr("server-peer-id");
  std::string client_peer_id = ga_conf_readstr("client-peer-id");

  if (server_peer_id.empty()) {
    server_peer_id = "ga";
    ga_logger(Severity::INFO, "*** no server peer id specified, default to %s.\n", server_peer_id.c_str());
  }
  if (client_peer_id.empty()) {
    client_peer_id = "client";
    ga_logger(Severity::INFO, "*** no client peer specified, default to %s\n", client_peer_id.c_str());
  }

  // When publishing local encoded stream, this must be enabled. Otherwise
  // disabled.
  bytes_sent_on_last_stat_call_ = 0;
  bytes_sent_on_last_credit_call_ = 0;
  current_available_bandwidth_ = 8 * 1000 * 1000;  //Initial setting for 1080p. Will be adjusted.
  GlobalConfiguration::SetEncodedVideoFrameEnabled(true);
  GlobalConfiguration::SetAEC3Enabled(false);
  GlobalConfiguration::SetAECEnabled(false);
  GlobalConfiguration::SetAGCEnabled(false);

  int ice_port_min = ga_conf_readint("ice-port-min");
  int ice_port_max = ga_conf_readint("ice-port-max");
  if ((ice_port_min > 0) && (ice_port_max > 0)) {
    ga_logger(Severity::INFO, "ice_port_min = %d ice_port_max = %d\n", ice_port_min, ice_port_max);
    GlobalConfiguration::SetIcePortAllocationRange(ice_port_min, ice_port_max);
  } else {
    ga_logger(Severity::INFO, "*** no ICE port range specified\n");
  }

  // Now by default, video hardware acceleration is enabled. On platform prior
  // to Haswell, need to call SetVideoHardwareAccelerationEnabled(false) to
  // disable hardware acceleration.
  GlobalConfiguration::SetVideoHardwareAccelerationEnabled(true);
  GlobalConfiguration::SetLowLatencyStreamingEnabled(true);
  GlobalConfiguration::SetExternalBweEnabled(true);
  GlobalConfiguration::SetExternalBweRateLimits(6 * 1024, 512, 24 * 1024);

  // Always enable customized audio input here. `CreateStream` will
  // enable/disable audio track according to conf.
  std::shared_ptr<P2PSocketSignalingChannel> signaling =
     std::make_shared<P2PSocketSignalingChannel>(P2PSocketSignalingChannel());

#ifdef WIN32
  struct RTSPConf* conf = rtspconf_global();
  std::unique_ptr<GAAudioFrameGenerator> generator =
     std::unique_ptr<GAAudioFrameGenerator>(GAAudioFrameGenerator::Create(
       conf->audio_channels, conf->audio_samplerate));
  audioGenerator = generator.get();
  GlobalConfiguration::SetCustomizedAudioInputEnabled(true,
                                                      std::move(generator));
#endif
  P2PClientConfiguration config;

  std::string codec = ga_conf_readstr("video-codec");

  VideoCodecParameters video_param;
  if (ga_conf_readbool("enable-hevc-codec", 0) != 0 ||
      std::string("h265") == codec ||
      std::string("hevc") == codec) {
    video_param.name = VideoCodec::kH265;
    ga_logger(Severity::INFO, "selected H265 codec\n");
  } else if (std::string("av1") == codec) {
    video_param.name = VideoCodec::kAv1;
    ga_logger(Severity::INFO, "selected AV1 codec\n");
  } else {
    video_param.name = VideoCodec::kH264;
    ga_logger(Severity::INFO, "selected H264 codec\n");
  }
  VideoEncodingParameters video_encoding_param(video_param, 0, false);
  config.video_encodings.push_back(video_encoding_param);

  std::string coturn_ip = ga_conf_readstr("coturn-ip");
  if (!coturn_ip.empty())
  {
    IceServer stun_server, turn_server;

    ga_logger(Severity::INFO, "coturn_ip = %s\n", coturn_ip.c_str());
    std::string coturn_username = ga_conf_readstr("coturn-username");
    std::string coturn_password = ga_conf_readstr("coturn-password");
    std::string coturn_port = ga_conf_readstr("coturn-port");

    std::string stun_url = "stun:" + coturn_ip + ":" + coturn_port;
    stun_server.urls.push_back(stun_url);
    stun_server.username = coturn_username;
    stun_server.password = coturn_password;
    config.ice_servers.push_back(stun_server);

    std::string turn_url_tcp = "turn:" + coturn_ip + ":" + coturn_port + "?transport=tcp";
    std::string turn_url_udp = "turn:" + coturn_ip + ":" + coturn_port + "?transport=udp";
    turn_server.urls.push_back(turn_url_tcp);
    turn_server.urls.push_back(turn_url_udp);
    turn_server.username = coturn_username;
    turn_server.password = coturn_password;
    config.ice_servers.push_back(turn_server);
  }
  else {
    ga_logger(Severity::INFO, "*** no coturn server specified.\n");
  }

  p2pclient_.reset(new P2PClient(config, signaling));
  p2pclient_->AddObserver(*this);
  std::future<int> connect_done = connect_status_.get_future();
  std::weak_ptr<ga::webrtc::ICSP2PClient> weak_this = shared_from_this();
  p2pclient_->AddAllowedRemoteId(client_peer_id);
  uint32_t client_clones = (uint32_t) ga_conf_readint("client-clones");
  for (uint32_t i = 1; i <= client_clones; i++) {
      p2pclient_->AddAllowedRemoteId(client_peer_id + "-clone" + std::to_string(i));
  }
  ga_logger(Severity::INFO, "Allow multi clone clients up to %d\n", client_clones);

  p2pclient_->Connect(get_p2p_server(), server_peer_id,
                      [weak_this](const std::string& id) {
                        auto that = weak_this.lock();
                        if (!that)
                          return;
                        that->ConnectCallback(false, "");
                      },
                      [weak_this](std::unique_ptr<Exception> err) {
                        auto that = weak_this.lock();
                        if (!that)
                          return;
                        that->ConnectCallback(true, err->Message());
                      });
  RegisterCallbacks();
  CreateStream();

  dump_file_ = nullptr;

  if (enable_dump_) {
    char dumpFileName[128];
    snprintf(dumpFileName, 128, "gaVideoInput-%p.h264", this);
    dump_file_ = fopen(dumpFileName, "wb");
  }

  enable_render_drc = (ga_conf_readint("enable-render-drc") > 0)? true: false;

  return 0;
}

void ICSP2PClient::Deinit()
{
#ifdef E2ELATENCY_TELEMETRY_ENABLED
    //E2ELatency
    if (gLatencyServerInstance) {
        ga_logger(Severity::INFO, "ics-p2p-client: Deinit: Deleting server instance.\n");
        ReleaseServer(gLatencyServerInstance);
    }
#endif
  if (stream_provider_)
    stream_provider_->DeRegisterEncoderObserver(*this);
  if (publication_)
    publication_->RemoveObserver(*this);
  if (p2pclient_) {
    p2pclient_->RemoveObserver(*this);
    p2pclient_->Stop(remote_user_id_, nullptr, nullptr);
    p2pclient_->Disconnect(nullptr, nullptr);
  }
  if (local_stream_)
    local_stream_->Close();
  if (local_audio_stream)
    local_audio_stream->Close();

#ifndef WIN32
  if (ga_conf_readbool("measure-latency", 0) == 1) {
    android::atrace_deinit();
  }
#endif
}

int ICSP2PClient::Start() {
  if (encoder_register_client(this) < 0)
    return -1;
  return 0;
}

void ICSP2PClient::ConnectCallback(bool is_fail, const std::string &error) {
  if(!is_fail && (ga_conf_readbool("k8s", 0) == 1)) {
    std::string filePath = ga_conf_readstr("aic-workdir") + "/" + ".p2p_status";
    std::ofstream statusFile;
    statusFile.open(filePath);
    statusFile << "started" <<"\n";
    statusFile.close();
  }
  connect_status_.set_value(is_fail);
}

void ICSP2PClient::OnMessageReceived(const std::string &remote_user_id,
                                  const std::string message) {
  send_blocked_ = false;
  if (message == "start") {
    p2pclient_->Publish(remote_user_id, local_stream_,
      [&](std::shared_ptr<owt::p2p::Publication> pub) {
        streaming_ = true;
        ga_encoder_->RequestKeyFrame();
        RequestCursorShape();
        publication_ = pub;
        publication_->AddObserver(*this);
      },
      nullptr);
    bool clone_client = ga_conf_readint("client-clones") >= 1 && remote_user_id.find("-clone") != std::string::npos;
    if (!clone_client) {
      remote_user_id_ = remote_user_id;
    }

#ifdef WIN32
    audioGenerator->ClientConnectionStatus(true);
    if (pHookClientStatus == NULL) {
      pHookClientStatus = [](uint32_t count) { ga_logger(Severity::INFO, "ics-p2p-client: Number of clients connected: %u.\n", count); };
    }
    pHookClientStatus(++connected_client_count);
#endif

    if (local_audio_stream.get())
      p2pclient_->Publish(remote_user_id, local_audio_stream, nullptr, nullptr);

    if (ga_conf_readbool("enable-multi-user", 0) != 0) {
      int userId = ga_conf_readint("user");
      std::string str = "{\"key\":\"user-id\",\"val\":\"" + std::to_string(userId) + "\"}";
      p2pclient_->Send(remote_user_id, str.c_str(), nullptr, nullptr);
    }
  } else {
    // Set client event for round trip delay calculation feature
    json j1 = json::parse(message);
    if (j1["type"].is_string() &&
      (j1["type"].get<std::string>() == "control") &&
      (j1["data"].is_object()) &&
      (j1["data"]["event"].is_string())) {
      std::string event_type = j1["data"]["event"];
      if (event_type == "framestats") {
        //ga_logger(Severity::INFO, "Debugs: event type %s\n", event_type);
        if (j1["data"]["parameters"].is_object()) {
          json event_param = j1["data"]["parameters"];
          if (event_param.size() == 5 &&
              event_param["framets"].is_number() &&
              event_param["framesize"].is_number() &&
              event_param["framedelay"].is_number() &&
              event_param["framestartdelay"].is_number() &&
              event_param["packetloss"].is_number()) {
            long f_ts = event_param["framets"];
            long f_size = event_param["framesize"];
            int f_delay = event_param["framedelay"];
            long f_start_delay = event_param["framestartdelay"];
            long p_loss = event_param["packetloss"];

            ga_logger(Severity::DBG, "ics-p2p-client: OnMessageRecvd: f_ts=%d, f_size=%d, f_delay=%d, f_start_delay=%d, p_loss=%d\n",
                f_ts, f_size, f_delay, f_start_delay, p_loss);

            if (ga_encoder_)
                ga_encoder_->SetFrameStats(f_ts, f_size, f_delay, f_start_delay, p_loss);

          }
        }
        return;
#ifndef WIN32
      } else if (event_type == "camerainfo") {
        ga_logger(Severity::INFO, "Received camera capability info from client\n");
        // Update camera_info to complete negotiation
        // protocol between client and Android vHAL.
        mCameraClientHandler->updateCameraInfo(message);
        return;
      } else if (enable_render_drc && event_type == "sizechange") {
        if (j1["data"]["parameters"].is_object()) {
          json event_param = j1["data"]["parameters"];
          if (event_param["rendererSize"].is_object()) {
            if (event_param["rendererSize"].size()== 2 &&
                event_param["rendererSize"]["width"].is_number() &&
                event_param["rendererSize"]["height"].is_number()) {
              int newWidth = event_param["rendererSize"]["width"];
              int newHeight = event_param["rendererSize"]["height"];
              if (ga_encoder_)
                ga_encoder_->ChangeRenderResolution(newWidth, newHeight);
            }
          }
        }
        return;
      } else if (event_type == "videoalpha") {
        if (j1["data"]["parameters"].is_object()) {
          json event_param = j1["data"]["parameters"];
          if (event_param["action"].is_number()) {
            uint32_t action = event_param["action"];
            if (ga_encoder_) {
              ga_encoder_->SetVideoAlpha(action);
              const char* ack = "{\"key\":\"video-alpha-success\"}";
              p2pclient_->Send(remote_user_id_, ack, nullptr, nullptr);
            }
          }
        }
        return;
      } else if (event_type == "sensorcheck") {
        mSensorHandler->configureClientSensors();
        mSensorHandler->setClientRequestFlag(true);
        return;
      } else if (event_type == "sensordata") {
        mSensorHandler->processClientMsg(message);
        return;
      } else if (event_type == "gps") {
        json event_param = j1["data"]["parameters"];
        std::string data = event_param["data"];
        ssize_t sts = 0;
        std::string err;
        std::tie(sts, err) = mVirtualGpsReceiver->Write((uint8_t *)data.c_str(), data.length());
        if (sts < 0)
          ga_logger(Severity::ERR, "Failed to write GPS data: %s\n", err.c_str());
        return;
      } else if (event_type == "cmdchannel") {
        mCommandChannelHandler->processClientMsg(message);
        return;
#ifdef E2ELATENCY_TELEMETRY_ENABLED
      } else if (event_type == "touch") {
        json event_param = j1["data"]["parameters"];
        if (event_param["E2ELatency"].is_number()) {
          gClientTimestamp = event_param["E2ELatency"];
          HandleLatencyMessage(message);
        }
#endif
#endif
      }
    }
    controller_->PushClientEvent(message);

#ifdef WIN32
    json j = json::parse(message);
    // Data validation.
    if (j["type"].is_string() &&
        (j["type"].get<std::string>() == "control") &&
        (j["data"].is_object()) &&
        (j["data"]["event"].is_string())) {
      std::string event_type = j["data"]["event"];
      if ((event_type == "mousemove") &&
          (j["data"]["parameters"].is_object())) {
        // Mouse motion.
        json event_param = j["data"]["parameters"];
        if (event_param.size() > 4 &&
            event_param["eventTimeSec"].is_number() &&
            event_param["eventTimeUsec"].is_number()) {
          struct timeval tv;
          tv.tv_sec = event_param["eventTimeSec"];
          tv.tv_usec = event_param["eventTimeUsec"];
          if (ga_encoder_)
            ga_encoder_->SetClientEvent(tv);
        }
      }
#ifdef E2ELATENCY_TELEMETRY_ENABLED
      else {
        // E2Elatency via keyboard
        if (event_type == "keydown" &&
          (j["data"]["parameters"].is_object())) {
          json event_param = j["data"]["parameters"];
          if (event_param["E2ELatency"].is_number()) {
            gClientTimestamp = event_param["E2ELatency"];
            HandleLatencyMessage(message);
          }

          if (ga_encoder_) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            ga_encoder_->SetClientEvent(tv);
          }
        }
      }
#endif
    }
#endif
  }
}

void ICSP2PClient::SendCursor(std::shared_ptr<CURSOR_DATA> cursor_data) {
  if (!streaming_ || send_blocked_)
    return;
#ifdef WIN32
  auto cursor_msg =
      ga::webrtc::CursorUtils::GetJsonForCursorInfo(*cursor_data.get());
  if(p2pclient_)
      p2pclient_->Send(remote_user_id_, cursor_msg, [this]() {
          send_failures_ = 0;
          send_blocked_ = false;
      },
      nullptr);
#endif
}

void ICSP2PClient::SendQoS(std::shared_ptr<QosInfo> qos_info) {
  if (!streaming_ || send_blocked_)
    return;
  auto qos_msg = ga::webrtc::QoSUtils::GetJsonForQoSInfo(*qos_info.get());
  p2pclient_->Send(remote_user_id_, qos_msg, [this]() {
        send_failures_ = 0;
        send_blocked_ = false;
      },
    [this](std::unique_ptr<owt::base::Exception>) {
      send_failures_++;
      if (send_failures_ >= OWT_MAX_SEND_FAILURES) {
        send_blocked_ = true;
      }
    });
}

void ICSP2PClient::CreateStream() {
  bool audio_enabled = ga_conf_readbool("enable-audio", 1);
  bool av_bundle = ga_conf_readbool("av-bundle", 1);
  ga_encoder_ = std::make_unique<GAVideoEncoder>();
  stream_provider_ = owt::base::EncodedStreamProvider::Create();
  stream_provider_->RegisterEncoderObserver(*this);
  std::shared_ptr<owt::base::LocalCustomizedStreamParameters> lcsp;
  if (!av_bundle) {
    lcsp.reset(
      (new LocalCustomizedStreamParameters(false, true)));
  } else {
    lcsp.reset((new LocalCustomizedStreamParameters(true, true)));
  }
  lcsp->Resolution(640, 480);
  int error_code = 0;
  local_stream_ = LocalStream::Create(lcsp, stream_provider_);
  if (audio_enabled && !av_bundle) {
    owt::base::LocalCameraStreamParameters lcspc(true, false);
    local_audio_stream = LocalStream::Create(lcspc, error_code);
  }
}

void ga::webrtc::ICSP2PClient::RequestCursorShape() {
  ga_module_t *video_encoder = encoder_get_vencoder();
  video_encoder->ioctl(GA_IOCTL_REQUEST_NEW_CURSOR, 0, nullptr);
}

void ICSP2PClient::InsertFrame(ga_packet_t* packet) {
  // Each time InsertFrame is invoked, we update the bandwidth to encoder wrapper.
  if (packet == nullptr || !capturer_started_) {
      return;
  }

  if (clock_ == nullptr)
    clock_.reset(new owt::base::Clock());
  if (g_bEnableOwtStats && p2pclient_ && streaming_) {
    p2pclient_->GetConnectionStats(remote_user_id_,
      [&](std::shared_ptr<owt::base::RTCStatsReport> report) {
        for (const auto& stat_rec : *report) {
          if (stat_rec.type_ == owt::base::RTCStatsType::kOutboundRTP) {
            auto& stat = stat_rec.cast_to<owt::base::RTCOutboundRTPStreamStats>();
            if (stat.kind == "video") {
              bytes_sent_on_last_stat_call_ = stat.bytes_sent;
            }
          } else if (stat_rec.type_ == owt::base::RTCStatsType::kCandidatePair) {
            auto& stat = stat_rec.cast_to<owt::base::RTCIceCandidatePairStats>();
            if (stat.nominated) {
              current_available_bandwidth_ = stat.available_outgoing_bitrate;
            }
          } else {
            // Ignore other stats.
          }
        }
      },
      [](std::unique_ptr<owt::base::Exception> err) {});
  }
  if (!capturer_started_ || !stream_provider_.get())
    return;

  owt::base::EncodedImageMetaData meta_data;
  int side_data_len = sizeof(FrameMetaData);
  FrameMetaData* side_data = (FrameMetaData*)ga_packet_get_side_data(
      packet, ga_packet_side_data_type::GA_PACKET_DATA_NEW_EXTRADATA,
      &side_data_len);

  if (!side_data)
    return;

#ifndef _WIN32
  if (packet->flags & GA_PKT_FLAG_KEY)
      meta_data.is_keyframe = true;
#endif

#ifndef DISABLE_TS_FT
  meta_data.picture_id = static_cast<uint16_t>(packet->pts);
#else
  meta_data.picture_id = 0;
#endif

#ifdef E2ELATENCY_TELEMETRY_ENABLED
  static uint64_t prev_time = 0;
#endif

  uint64_t current_time = clock_->TimeInMilliseconds();
  meta_data.last_fragment = (side_data->last_slice);
  meta_data.capture_timestamp = current_time;

  meta_data.encoding_start = meta_data.capture_timestamp +
      side_data->encode_start_ms -
      side_data->capture_time_ms;
  meta_data.encoding_end = meta_data.capture_timestamp +
      side_data->encode_end_ms -
      side_data->capture_time_ms;
#ifdef E2ELATENCY_TELEMETRY_ENABLED
  //E2ELatency
  meta_data.picture_id = updateFrameNumber();
  if (gLatencyServerInstance) {
      // form the message
      void* pMsg = gpMsg; // gpMsg should come from OnMessageReceived
      unsigned int frameToSend = getFrameNumber();

      if (pMsg != nullptr) {
          gLatencyServerInstance->SetProcessingFrameId(pMsg, frameToSend);
          gLatencyServerInstance->SetSendTime(pMsg);
          gLatencyServerInstance->SetEncodeTime(pMsg, (uint32)(meta_data.encoding_end - meta_data.encoding_start));

          int64_t timediff = current_time - prev_time;
          ga_logger(Severity::DBG, "ics-p2p-client: InsertFrame: timediff from prev InsertFrame call: %lld ms\n",
              timediff);

          if (frameToSend == currentFrame + 2) {
              std::string msgString = gLatencyServerInstance->PrintMessage(pMsg);
              // copy message to meta data
              if (!msgString.empty()) {
                  // allocate memory for latency message
                  meta_data.encoded_image_sidedata_new(msgString.size());
                  uint8_t* p_latency_message = meta_data.encoded_image_sidedata_get();
                  size_t latency_message_size = meta_data.encoded_image_sidedata_size();
                  // copy the message
                  if (msgString.size() > 0 && p_latency_message) {
                      memcpy(p_latency_message, msgString.data(), msgString.size());
                  }
                  ga_logger(Severity::INFO, "ics-p2p-client: InsertFrame: Frame %d: msg_size %d: Latency message sent from server: %s\n",
                      frameToSend, latency_message_size, msgString.c_str());
              }
              gLatencyServerInstance->FreeMessage(gpMsg);
              gpMsg = nullptr;
          }
      }
  }

  // previous frame time
  prev_time = current_time;
  // end of E2ELatency
#endif

  std::vector<uint8_t> buffer;
  if (packet->data != nullptr && packet->size > 0) {
    buffer.insert(buffer.begin(), packet->data, packet->data + packet->size);
    if (dump_file_) {
      fwrite(packet->data, 1, packet->size, dump_file_);
    }
#ifndef WIN32
    if (android::is_atrace_enabled()) {
      static int nS3Count = 0;
      nS3Count++;
      std::string str = "atou S3 ID: " + std::to_string(nS3Count) + " size: " + std::to_string(packet->size) + " " + remote_user_id_;
      android::atrace_begin(str);
      android::atrace_end();
    }
#endif
    stream_provider_->SendOneFrame(buffer, meta_data);

#ifdef E2ELATENCY_TELEMETRY_ENABLED
    // free memory for latency message
    meta_data.encoded_image_sidedata_free();
#endif
  }

  ga_encoder_->SetMaxBps(current_available_bandwidth_);
}

// Once this is called, we will reset the credit bytes.
int64_t ICSP2PClient::GetCreditBytes() {
  int64_t credit_bytes = bytes_sent_on_last_stat_call_ - bytes_sent_on_last_credit_call_;
  bytes_sent_on_last_credit_call_ = bytes_sent_on_last_stat_call_;
  return credit_bytes;
}

int64_t ICSP2PClient::GetMaxBitrate() {
  return current_available_bandwidth_;
}

void ICSP2PClient::OnStreamAdded(
    std::shared_ptr<owt::base::RemoteStream> stream) {
#ifndef WIN32
  ga_logger(Severity::INFO, "OnStreamAdded\n");
  mRemoteStreamHandler->setStream(stream);
  mRemoteStreamHandler->subscribeForAudio();
#endif
}

void ICSP2PClient::OnStarted() {
  capturer_started_ = true;
  if (ga_encoder_ != nullptr)
    ga_encoder_->RequestKeyFrame();
}

void ICSP2PClient::OnStopped() {
  capturer_started_ = false;
  streaming_ = false;
#ifndef WIN32
  mSensorHandler->setClientRequestFlag(false);
#endif
}

void ICSP2PClient::OnKeyFrameRequest() {
  if (ga_encoder_ != nullptr) {
    ga_encoder_->RequestKeyFrame();
  }
}

void ICSP2PClient::OnEnded() {
    ga_logger(Severity::INFO, "ended.");
}

void ICSP2PClient::OnStreamStopped(const std::string& remote_user_id) {
#ifdef WIN32
    if (pHookClientStatus == NULL) {
      pHookClientStatus = [](uint32_t count) { ga_logger(Severity::INFO, "ics-p2p-client: Number of clients connected: %u.\n", count); };
    }
    pHookClientStatus(--connected_client_count);
#endif

    uint32_t client_clones = (uint32_t) ga_conf_readint("client-clones");
    if (client_clones >= 1 && remote_user_id.find("-clone") != std::string::npos) {
        ga_logger(Severity::INFO, "Do nothing for clone client stop\n");
        return;
    }

    ga_logger(Severity::INFO, "on_stopped\n");
    if (ga_encoder_ != nullptr) {
      ga_encoder_->Pause();
    }
#ifdef WIN32
    audioGenerator->ClientConnectionStatus(false);
#endif
}

void ICSP2PClient::OnRateUpdate(uint64_t bitrate_bps, uint32_t frame_rate) {
    // Do nothing here.
}
