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

#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <stdlib.h>
#include <ga-module.h>
#include <CSendRecvMessage.h>

using namespace std::chrono_literals;

namespace {
  static const char* g_icr_default_host = "127.0.0.1";
  static int g_icr_default_port = 23432;
  static const char* g_default_workdir = "/opt/workdir";

  static const char* g_icr_ip = g_icr_default_host;
  static int g_icr_port = g_icr_default_port;
  static const char* g_workdir = g_default_workdir;

  static double g_framerate = 30;

  static int g_bitstream_count = 0;
  static int g_null_pkts = 0;
  static int g_zero_pkts = 0;
}

std::string ga_conf_readstr(const char *key)
{
  ga_logger(Severity::DBG, "ga_conf_readstr: %s\n", key);
  if (std::string("video-bs-file") == key) {
    return "";
  } else if (std::string("aic-workdir") == key) {
    return g_workdir;
  } else if (std::string("icr-ip") == key) {
    return g_icr_ip;
  }

  fprintf(stderr, "fatal: ga_conf_readstr: key not hooked: %s\n", key);
  exit(-1);
}

int ga_conf_readint(const char *key)
{
  ga_logger(Severity::DBG, "ga_conf_readint: %s\n", key);
  if (std::string("android-session") == key ||
      std::string("user") == key) {
    return 0;
  } else if (std::string("icr-port") == key) {
    return g_icr_port;
  }

  fprintf(stderr, "fatal: ga_conf_readint: key not hooked: %s\n", key);
  exit(-1);
}

int ga_conf_readbool(const char *key)
{
  ga_logger(Severity::DBG, "ga_conf_readbool: %s\n", key);
  if (std::string("k8s") == key) {
    return 0;
  }
  fprintf(stderr, "fatal: ga_conf_readbol: key not hooked: %s\n", key);
  exit(-1);
}

int encoder_send_packet(const char *prefix, int channelId, ga_packet_t *pkt, int64_t encoderPts, struct timeval *ptv)
{
  ++g_bitstream_count;
  if (!pkt || !pkt->data) ++g_null_pkts;
  if (!pkt || !pkt->size) ++g_zero_pkts;
  return 0;
}

class IrrvTest: public ::testing::Test
{
protected:
  void SetUp() override {
    irrv_ = std::make_unique<CSendRecvMessage>(false);
    bitstream_count = g_bitstream_count;
  }
  void TearDown() override {
    irrv_ = nullptr;
    PrintCounts();
  }

  void SnapshotCounts() {
    bitstream_count = g_bitstream_count - bitstream_count;
    null_pkts = g_null_pkts;
    zero_pkts = g_zero_pkts;
  }

  void PrintCounts() {
    std::cout << "bitstream count: " << bitstream_count << std::endl;
    std::cout << "null pkts: " << null_pkts << std::endl;
    std::cout << "zero pkts: " << zero_pkts << std::endl;
  }

  std::unique_ptr<CSendRecvMessage> irrv_;
  int bitstream_count;
  int null_pkts;
  int zero_pkts;
};

// Tests start/stop sequence of CSendRecvMessage. During this
// sequence internal thread should be started and it should perform
// negotiation with ICR. Our expectation is that such negotiation
// will take place.
TEST_F(IrrvTest, StartStop)
{
  irrv_->start();
  // TODO: latency of establishing connection with ICR, i.e. timebetween(ready, start)
  // seems to be ~1s on my non-loaded system. This seems too much and worths investigation.
  EXPECT_TRUE(irrv_->ready(std::chrono::system_clock::now() + 5s));
  irrv_->stop();
}

TEST_F(IrrvTest, EncodeStartStop)
{
  irrv_->start();
  EXPECT_TRUE(irrv_->ready(std::chrono::system_clock::now() + 5s));
  EXPECT_GT(irrv_->irrv_set_encodestart(), 0);
  EXPECT_GT(irrv_->irrv_set_encodestop(), 0);
  irrv_->stop();
}

TEST_F(IrrvTest, EncodeStartStopLoop)
{
  irrv_->start();
  EXPECT_TRUE(irrv_->ready(std::chrono::system_clock::now() + 5s));
  for(int i=0; i < 10; ++i) {
    EXPECT_GT(irrv_->irrv_set_encodestart(), 0);
    EXPECT_GT(irrv_->irrv_set_encodestop(), 0);
  }
  irrv_->stop();
}

TEST_F(IrrvTest, EncodeRun1s)
{
  irrv_->start();
  EXPECT_TRUE(irrv_->ready(std::chrono::system_clock::now() + 5s));
  EXPECT_GT(irrv_->irrv_set_encodestart(), 0);
  EXPECT_GT(irrv_->irrv_set_keyframe(), 0);
  usleep(1000*1000);
  SnapshotCounts();
  EXPECT_LE(bitstream_count, (int)(g_framerate*1.1));
  EXPECT_GE(bitstream_count, (int)(g_framerate*0.9));
  EXPECT_EQ(null_pkts, 0);
  EXPECT_EQ(zero_pkts, 0);
  EXPECT_GT(irrv_->irrv_set_encodestop(), 0);
  irrv_->stop();
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  int idx;
  for (idx = 1; idx < argc; ++idx) {
    if (std::string("--icr-ip") == argv[idx]) {
      if (++idx >= argc) break;
      g_icr_ip = argv[idx];
    } else if (std::string("--icr-port") == argv[idx]) {
      if (++idx >= argc) break;
      g_icr_port = atoi(argv[idx]);
    } else if (std::string("--workdir") == argv[idx]) {
      if (++idx >= argc) break;
      g_workdir = argv[idx];
    } else if (std::string("--framerate") == argv[idx]) {
      if (++idx >= argc) break;
      g_framerate = atof(argv[idx]);
    } else {
      break;
    }
  }
  return RUN_ALL_TESTS();
}

