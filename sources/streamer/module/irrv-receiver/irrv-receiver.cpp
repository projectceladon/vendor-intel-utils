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

#include <memory>

#include <strings.h>

#include "ga-common.h"
#include "ga-conf.h"
#include "ga-module.h"

#include "CSendRecvMessage.h"

#define LOG_PREFIX "irrv-receiver: "

namespace {
    std::unique_ptr<CSendRecvMessage> pCSendRecvMessage;
}

static int irrv_receiver_init(void *arg)
{
    int icr_start = ga_conf_readint("icr-start-immediately");
    try {
        pCSendRecvMessage = std::make_unique<CSendRecvMessage>(icr_start == 1);
    } catch(...) {
        ga_logger(Severity::ERR, LOG_PREFIX "failed to init\n");
        return 1;
    }
    ga_logger(Severity::INFO, LOG_PREFIX "initialized\n");
    return 0;
}

static int irrv_receiver_deinit(void *arg)
{
    pCSendRecvMessage.reset();
    return 0;
}

static int irrv_receiver_start(void *arg)
{
    try {
        pCSendRecvMessage->start();
    } catch(...) {
        ga_logger(Severity::ERR, LOG_PREFIX "failed to start\n");
        return 1;
    }
    ga_logger(Severity::INFO, LOG_PREFIX "all started\n");
    return 0;
}

static int irrv_receiver_stop(void *arg)
{
    pCSendRecvMessage->irrv_set_encodestop();
    pCSendRecvMessage->stop();
    ga_logger(Severity::INFO, LOG_PREFIX "all stopped\n");
    return 0;
}

static int irrv_receiver_ioctl(int command, int argsize, void *arg) {
    int ret = 0;

    switch(command) {
    case GA_IOCTL_UPDATE_FRAME_STATS:
        ga_logger(Severity::DBG, LOG_PREFIX "GA_IOCTL_UPDATE_FRAME_STATS\n");
        {
            ga_ioctl_framestats_s *framestats = (ga_ioctl_framestats_s*)arg;
            pCSendRecvMessage->pipe_set_client_feedback(framestats->frame_delay, framestats->frame_size);
        }
        break;
    case GA_IOCTL_REQUEST_KEYFRAME:
        ga_logger(Severity::INFO, LOG_PREFIX "GA_IOCTL_REQUEST_KEYFRAME\n");
        pCSendRecvMessage->irrv_set_encodestart();
        pCSendRecvMessage->irrv_set_keyframe();
        break;
    case GA_IOCTL_PAUSE:
        ga_logger(Severity::INFO, LOG_PREFIX "GA_IOCTL_PAUSE\n");
        pCSendRecvMessage->irrv_set_encodestop();
        break;
    case GA_IOCTL_CHANGE_RENDER_RESOLUTION:
        ga_logger(Severity::INFO, LOG_PREFIX "GA_IOCTL_CHANGE_RESOLUTION\n");
        {
            ga_ioctl_resolution_s *res = (ga_ioctl_resolution_s *)arg;
            pCSendRecvMessage->pipe_set_resolution_change(res->width, res->height);
        }
        break;
    case GA_IOCTL_SET_VIDEO_ALPHA:
        ga_logger(Severity::INFO, LOG_PREFIX "GA_IOCTL_SET_VIDEO_ALPHA\n");
        {
            uint32_t action = *(uint32_t*)arg;
            pCSendRecvMessage->pipe_set_video_alpha(action);
        }
    default:
        ga_logger(Severity::DBG, LOG_PREFIX "GA_IOCTL_<unknown>: %d\n", command);
        ret = GA_IOCTL_ERR_NOTSUPPORTED;
        break;
    }

    return ret;
}

ga_module_t *
module_load() {
        static ga_module_t m;
        //
        bzero(&m, sizeof(m));
        m.type = GA_MODULE_TYPE_VENCODER;
        m.name = "intel-irrv-receiver";
        m.mimetype = "video/H264";
        m.init = irrv_receiver_init;
        m.start = irrv_receiver_start;
        m.stop = irrv_receiver_stop;
        m.deinit = irrv_receiver_deinit;
        //
        m.ioctl = irrv_receiver_ioctl;
        return &m;
}
