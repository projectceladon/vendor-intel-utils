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

#include <chrono>
#include <csignal>
#include <mutex>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ga-module.h>
#include <CSendRecvMessage.h>

//using namespace std::chrono;
using namespace std::chrono_literals;

namespace {
  static const char* g_default_loglevel = "info";
  static const char* g_icr_default_host = "127.0.0.1";
  static int g_icr_default_port = 23432;
  static const char* g_default_workdir = "/opt/workdir";

  const char* g_loglevel = g_default_loglevel;
  static const char* g_icr_ip = g_icr_default_host;
  static int g_icr_port = g_icr_default_port;
  static const char* g_workdir = g_default_workdir;

  std::mutex g_mutex;
  std::condition_variable g_cv;
  static bool g_stop = false;
  static FILE* g_bitstream = NULL;
}

static enum Severity get_loglevel(const char* level) {
    if (std::string("error") == level)
        return Severity::ERR;
    else if (std::string("warning") == level)
        return Severity::WARNING;
    else if (std::string("info") == level)
        return Severity::INFO;
    else if (std::string("debug") == level)
        return Severity::DBG;
    return Severity::ERR;
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

int encoder_send_packet(const char* prefix, int channelId, ga_packet_t *pkt, int64_t encoderPts, struct timeval *ptv)
{
  ga_logger(Severity::DBG, "encoder_send_packet: pkt=%p, size=%d\n", pkt, (pkt)? pkt->size: 0);
  if (g_bitstream) {
    fwrite(pkt->data, 1, pkt->size, g_bitstream);
  }
  return 0;
}

static void signal_handler(int signal) {
  if (signal == SIGTERM || signal == SIGINT) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_stop  = true;
    printf("\nUser requested to terminate server.\n");
    g_cv.notify_one();
  }
}

void usage(const char* app)
{
    printf("usage: %s [options] <output>\n", app);
    printf("\n");
    printf("Captured output will be in a raw encoded format, i.e. .h264, .h265, .av1\n");
    printf("\n");
    printf("Global options:\n");
    printf("  -h, --help              Print this help\n");
    printf("  --loglevel <level>      Loglevel to use (default: %s)\n", g_default_loglevel);
    printf("              error         Only errors will be printed\n");
    printf("              warning       Errors and warnings will be printed\n");
    printf("              info          Errors, warnings and info messages will be printed\n");
    printf("              debug         Everything will be printed, including lowlevel debug messages\n");
    printf("ICR server options:\n");
    printf("  --icr-ip <ip>           ICR server ip address (default: %s)\n", g_icr_default_host);
    printf("  --icr-port <port>       ICR server port (default: %d)\n", g_icr_default_port);
    printf("\n");
    printf("AIC (Android In Container) server options:\n");
    printf("  --workdir <path>        Path to AIC workdir (default: %s)\n", g_default_workdir);
}

int main(int argc, char* argv[])
{
  int idx;
  for (idx = 1; idx < argc; ++idx) {
    if (std::string("-h") == argv[idx] ||
        std::string("--help") == argv[idx]) {
      usage(argv[0]);
      exit(0);
    } else if (std::string("--loglevel") == argv[idx]) {
      if (++idx >= argc) break;
      g_loglevel = argv[idx];
    } else if (std::string("--icr-ip") == argv[idx]) {
      if (++idx >= argc) break;
      g_icr_ip = argv[idx];
    } else if (std::string("--icr-port") == argv[idx]) {
      if (++idx >= argc) break;
      g_icr_port = atoi(argv[idx]);
    } else if (std::string("--workdir") == argv[idx]) {
      if (++idx >= argc) break;
      g_workdir = argv[idx];
    } else {
      break;
    }
  }

  if(idx >= argc) {
    fprintf(stderr, "fatal: invalid option or no output file specified\n");
    usage(argv[0]);
    return -1;
  }

  g_bitstream = fopen(argv[idx], "wb");
  if (!g_bitstream) {
    fprintf(stderr, "fatal: failed to open file for writing: %s\n", argv[idx]);
    exit(-1);
  }

  printf("Starting IRRV server client, press CTRL^C to stop...");

  ga_set_loglevel(get_loglevel(g_loglevel));

  std::unique_ptr<CSendRecvMessage> irrv;

  irrv = std::make_unique<CSendRecvMessage>(false);

  std::signal(SIGTERM, signal_handler);
  std::signal(SIGINT, signal_handler);

  irrv->start();

  if (!irrv->ready(std::chrono::system_clock::now() + 1s)) {
    fprintf(stderr, "fatal: connection to ICR not ready after 1s\n");
    fclose(g_bitstream);
    return -1;
  }

  int ret = irrv->irrv_set_encodestart();
  if (ret < 0) {
    fprintf(stderr, "fatal: failed to start encoding\n");
    fclose(g_bitstream);
    return -1;
  }

  ret = irrv->irrv_set_keyframe();
  if (ret < 0) {
    fprintf(stderr, "fatal: irrv_set_keyframe failed\n");
    fclose(g_bitstream);
    return -1;
  }

  {
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cv.wait(lock, []{ return g_stop; });
  }

  ret = irrv->irrv_set_encodestop();
  if (ret < 0) {
    fprintf(stderr, "warning: irrv_set_encodestop failed\n");
  }

  irrv->stop();

  fclose(g_bitstream);

  return 0;
}

