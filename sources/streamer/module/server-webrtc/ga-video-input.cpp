// Copyright (C) 2017-2022 Intel Corporation
//
// SPDX-License-Identifier: BSD-3-Clause

#include "ga-video-input.h"
#include "encoder-common.h"

namespace ga {
namespace webrtc {

void GAVideoEncoder::RequestKeyFrame() {
  ga_module_t* video_encoder = encoder_get_vencoder();
  if (video_encoder != nullptr)
    video_encoder->ioctl(GA_IOCTL_REQUEST_KEYFRAME, 0, nullptr);
}

void GAVideoEncoder::Pause() {
  ga_module_t* video_encoder = encoder_get_vencoder();
  if (video_encoder != nullptr)
    video_encoder->ioctl(GA_IOCTL_PAUSE, 0, nullptr);
}

void GAVideoEncoder::SetMaxBps(int64_t bps) {
  ga_module_t* video_encoder = encoder_get_vencoder();
  ga_ioctl_maxbps_t ioctl;
  ioctl.max_bps = bps;
  video_encoder->ioctl(GA_IOCTL_SET_MAX_BPS, sizeof(ga_ioctl_maxbps_t), &ioctl);
}

#ifdef WIN32
void GAVideoEncoder::SetClientEvent(struct timeval tv) {
  ga_module_t* video_encoder = encoder_get_vencoder();
  ga_ioctl_clevent_t pclievent;
  pclievent.timeevent.tv_sec = tv.tv_sec;
  pclievent.timeevent.tv_usec = tv.tv_usec;
  video_encoder->ioctl(GA_IOCTL_UPDATE_CLIENT_EVENT, sizeof(ga_ioctl_clevent_t), &pclievent);
}
#endif

void GAVideoEncoder::SetFrameStats(long f_ts, long f_size, long f_delay, long f_start_delay, long p_loss) {
  ga_module_t* video_encoder = encoder_get_vencoder();
  ga_ioctl_framestats_t framestats;
  framestats.frame_ts = f_ts;
  framestats.frame_size = f_size;
  framestats.frame_delay = f_delay;
  framestats.frame_start_delay = f_start_delay;
  framestats.packet_loss = p_loss;
  video_encoder->ioctl(GA_IOCTL_UPDATE_FRAME_STATS, sizeof(ga_ioctl_framestats_t), &framestats);
}

void GAVideoEncoder::ChangeRenderResolution(int width, int height)
{
  ga_module_t* video_encoder = encoder_get_vencoder();
  ga_ioctl_resolution_t res;
  res.width = width;
  res.height = height;
  video_encoder->ioctl(GA_IOCTL_CHANGE_RENDER_RESOLUTION, sizeof(ga_ioctl_resolution_t), &res);
}

void GAVideoEncoder::SetVideoAlpha(uint32_t action)
{
  ga_module_t* video_encoder = encoder_get_vencoder();
  video_encoder->ioctl(GA_IOCTL_SET_VIDEO_ALPHA, sizeof(uint32_t), &action);
}
}
}
