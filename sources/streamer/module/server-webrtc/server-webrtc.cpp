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

#include <cassert>
#include <future>
#include <iostream>

#include "encoder-common.h"
#include "ga-common.h"
#include "ga-conf.h"
#include "ga-module.h"
#include "ics-p2p-client.h"

#include "owt/base/logging.h"

#ifdef WIN32
#include "ctrl-sdl/ctrl-sdl.h"
#include "p2p-socket-signaling-channel.h"
#endif

static std::shared_ptr<ga::webrtc::ICSP2PClient> p2pclient_;

static owt::base::LoggingSeverity get_owt_loglevel(const char* level) {
    if (std::string("none") == level)
        return owt::base::LoggingSeverity::kNone;
    else if (std::string("error") == level)
        return owt::base::LoggingSeverity::kError;
    else if (std::string("warning") == level)
        return owt::base::LoggingSeverity::kWarning;
    else if (std::string("info") == level)
        return owt::base::LoggingSeverity::kInfo;
    else if (std::string("verbose") == level)
        return owt::base::LoggingSeverity::kVerbose;
    else if (std::string("sensitive") == level)
        return owt::base::LoggingSeverity::kSensitive;
    return owt::base::LoggingSeverity::kWarning;
}

#ifdef WIN32
static int webrtc_server_init(void* arg, void (*p)(struct timeval)) {
#else
static int webrtc_server_init(void* arg) {
#endif
  ga_logger(Severity::INFO, "webrtc_server_init\n");

  owt::base::LoggingSeverity owtLogLevel = get_owt_loglevel(ga_conf_readstr("owt-loglevel").c_str());
#ifdef WIN32
  owt::base::Logging::Severity(owt::base::LoggingSeverity::kNone);
  owt::base::Logging::LogToConsole(owt::base::LoggingSeverity::kNone);

  size_t logSize = 2000000; // writes to a single file until 'max_total_log_size/2' bytes

  std::string logDir = "C:\\Temp\\";
  ga_logger(Severity::INFO, "webrtc_server_init owtLogLevel %d, directory %s, file size %d\n",
      static_cast<int>(owtLogLevel), logDir.c_str(), logSize);
  owt::base::Logging::LogToFileRotate(owtLogLevel, logDir, logSize);
#else
  owt::base::Logging::Severity(owtLogLevel);
#endif
  p2pclient_ = std::make_shared<ga::webrtc::ICSP2PClient>();
  return p2pclient_->Init(arg);
}

static int webrtc_server_start(void* arg) {
  ga_logger(Severity::INFO, "webrtc_server_start\n");
  if (p2pclient_)
    return p2pclient_->Start();
  else
    return -1;
}

static int webrtc_server_stop(void* arg) {
  return 0;
}

static int webrtc_server_ioctl(int command, int argsize, void* arg) {
  int ret = 0;
  ga_ioctl_credit_t *credit = (ga_ioctl_credit_t*)arg;
  int64_t credit_bytes = 0;
  switch (command) {
  case GA_IOCTL_GET_CREDIT_BYTES:
    if (argsize != sizeof(ga_ioctl_credit_t))
      return GA_IOCTL_ERR_INVALID_ARGUMENT;
    if(p2pclient_) {
      credit_bytes = p2pclient_->GetCreditBytes();
      bcopy(&credit_bytes, &(credit->credit_bytes), sizeof(credit->credit_bytes));
    }
    break;
  default:
    break;
  }
  return ret;
}

static int webrtc_server_deinit(void* arg) {
  p2pclient_->Deinit();
  return 0;
}

static int webrtc_server_send_packet(const char* prefix,
                                     int channelId,
                                     ga_packet_t* pkt,
                                     int64_t encoderPts,
                                     struct timeval* ptv) {
  if(p2pclient_)
      p2pclient_->InsertFrame(pkt);
  return 0;
}

#ifdef WIN32
static int webrtc_server_send_cursor(std::shared_ptr<CURSOR_DATA> cursorInfo, struct timeval* ptv) {
  if(p2pclient_)
    p2pclient_->SendCursor(cursorInfo);
  return 0;
}
#endif

static int webrtc_server_send_qos(std::shared_ptr<QosInfo> qosInfo) {
  if(p2pclient_)
    p2pclient_->SendQoS(qosInfo);
  return 0;
}

ga_module_t* module_load() {
  static ga_module_t m;
  //
  bzero(&m, sizeof(m));
  m.type = GA_MODULE_TYPE_SERVER;
  m.name = "webrtc-server";
  m.init = webrtc_server_init;
  m.start = webrtc_server_start;
  m.stop = webrtc_server_stop;
  m.deinit = webrtc_server_deinit;
  m.send_packet = webrtc_server_send_packet;
#ifdef WIN32
  m.send_cursor = webrtc_server_send_cursor;
#endif
  m.send_qos = webrtc_server_send_qos;
  m.ioctl = webrtc_server_ioctl;
  //
  encoder_register_sinkserver(&m);
  //
  return &m;
}
