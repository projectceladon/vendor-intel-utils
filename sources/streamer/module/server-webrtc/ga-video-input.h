// Copyright (C) 2017-2022 Intel Corporation
//
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#ifndef GAVIDEOENCODER_H
#define GAVIDEOENCODER_H

#include <stdint.h>
#ifdef WIN32
#include <WinSock2.h>
#endif

namespace ga {
namespace webrtc {

/// This class interfaces with encoder in ga.
class GAVideoEncoder {
 public:
  GAVideoEncoder() = default;
  virtual ~GAVideoEncoder() = default;
  void RequestKeyFrame();
  void Pause();
  void SetMaxBps(int64_t bps);
#ifdef WIN32
  void SetClientEvent(struct timeval tv);
#endif
  void SetFrameStats(long f_ts, long f_size, long f_delay, long f_start_delay, long p_loss);
  void ChangeRenderResolution(int width, int height);
  void SetVideoAlpha(uint32_t action);
};

}  // namespace webrtc
}  // namespace ga

#endif

