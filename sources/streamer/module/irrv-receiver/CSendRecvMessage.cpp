// Copyright (C) 2017-2022 Intel Corporation
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

#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>

#include <unistd.h>
#include <sys/stat.h>

#include "ga-conf.h"
#include "ga-common.h"
#include "ga-module.h"

#include "irrv_protocol.h"
#include "CSendRecvMessage.h"

#include "encoder-common.h"

using namespace std;

#define MAX_NUM 2000

#define LOG_PREFIX "irrv-receiver: "

CSendRecvMessage::CSendRecvMessage(bool startEncoderImmediately) {
    m_bEnableAlphaTransmission = false;
    m_width                    = 0;
    m_height                   = 0;
    m_format                   = 0;
    m_bStartEncoderImmediately  = startEncoderImmediately;

    std::string fileName = ga_conf_readstr("video-bs-file");
    if (!fileName.empty()) {
        m_encout = fopen(fileName.c_str(), "wb");
        if (!m_encout) {
            ga_logger(Severity::ERR, LOG_PREFIX "failed to open bitstream dump file: %s\n", fileName.c_str());
        }
    }
    ga_logger(Severity::INFO, LOG_PREFIX "video-bs-file: %s\n", fileName.c_str());
}

CSendRecvMessage::~CSendRecvMessage() {
    stop();

    if (m_client >= 0) {
        close(m_client);
    }
    if (m_encout) {
        fclose(m_encout);
    }
}

void CSendRecvMessage::start() {
    if (m_thread.joinable())
        return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = false;
    }
    m_thread = std::thread([this]{
        while(1) {
            run();
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_stop) break;
            }
        }
    });
}

void CSendRecvMessage::stop() {
    if (m_thread.joinable()) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_thread.join();
    }
}

void CSendRecvMessage::run() {
    recv_es_stream();
}

bool CSendRecvMessage::irrv_sock_client_init() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client < 0) {
        m_client = sock_client_init();

        if (m_bStartEncoderImmediately && m_client >= 0 && disconnect != sock_client_check_connect(m_client, 0))
        {
            ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_START\n");
            irrv_vctrl_event_t ctrl_ev;
            memset(&ctrl_ev, 0, sizeof(ctrl_ev));
            ctrl_ev.event.magic    = IRRV_MAGIC;
            ctrl_ev.event.type     = IRRV_EVENT_VCTRL;
            ctrl_ev.event.size     = sizeof(ctrl_ev);
            ctrl_ev.event.value    = 0;
            ctrl_ev.info.ctrl_type = IRRV_CTRL_START;
            ctrl_ev.info.value     = 1;

            sock_client_send(m_client, &ctrl_ev, sizeof(ctrl_ev));

            ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_KEYFRAME_SETTING\n");
            memset(&ctrl_ev, 0, sizeof(ctrl_ev));
            ctrl_ev.event.magic    = IRRV_MAGIC;
            ctrl_ev.event.type     = IRRV_EVENT_VCTRL;
            ctrl_ev.event.size     = sizeof(ctrl_ev);
            ctrl_ev.event.value    = 0;
            ctrl_ev.info.ctrl_type = IRRV_CTRL_KEYFRAME_SETTING;
            ctrl_ev.info.value     = 1;

            sock_client_send(m_client, &ctrl_ev, sizeof(ctrl_ev));
        }
    }

    return (m_client < 0)? false: true;
}

void CSendRecvMessage::irrv_sock_client_disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_client >= 0) {
        close(m_client);
        m_client = -1;
    }
}

void CSendRecvMessage::irrv_send_authentication()
{
    // That's internal helper function which is called from the
    // context where we already checked connection - no need to recheck

    ga_logger(Severity::INFO, LOG_PREFIX "IRRV_EVENT_VAUTH\n");
    // currently use hard coded uuid key
    // customer may use their own key mechanism
    const unsigned char auth_id[IRRV_UUID_LEN]  = DEFAULT_AUTH_ID;
    const unsigned char auth_key[IRRV_UUID_LEN] = DEFAULT_AUTH_KEY;
    irrv_vauth_event_t  auth_ev;

    memset(&auth_ev, 0, sizeof(auth_ev));
    auth_ev.event.type = IRRV_EVENT_VAUTH;
    auth_ev.event.size = sizeof(auth_ev);
    memcpy(auth_ev.info.id,  auth_id,  IRRV_UUID_LEN);
    memcpy(auth_ev.info.key, auth_key, IRRV_UUID_LEN);

    sock_client_send(m_client, &auth_ev, sizeof(auth_ev));
}

const char* irrv_ctrl_type(int type)
{
#define IRRV_TO_STR(S) #S
#define IRRV_CASE(T) case T: return IRRV_TO_STR(T)
    switch (type) {
    IRRV_CASE(IRRV_CTRL_NONE);
    IRRV_CASE(IRRV_CTRL_KEYFRAME_SETTING);
    IRRV_CASE(IRRV_CTRL_BITRATE_SETTING);
    IRRV_CASE(IRRV_CTRL_QP_SETTING);
    IRRV_CASE(IRRV_CTRL_GOP_SETTING);
    IRRV_CASE(IRRV_CTRL_START);
    IRRV_CASE(IRRV_CTRL_PAUSE);
    IRRV_CASE(IRRV_CTRL_STOP);
    IRRV_CASE(IRRV_CTRL_DUMP_START);
    IRRV_CASE(IRRV_CTRL_DUMP_STOP);
    IRRV_CASE(IRRV_CTRL_DUMP_FRAMES);
    IRRV_CASE(IRRV_CTRL_RESOLUTION);
    IRRV_CASE(IRRV_CTRL_FRAMERATE_SETTING);
    IRRV_CASE(IRRV_CTRL_MAXFRAMESIZE_SETTING);
    IRRV_CASE(IRRV_CTRL_RIR_SETTING);
    IRRV_CASE(IRRV_CTRL_MIN_MAX_QP_SETTING);
    IRRV_CASE(IRRV_CTRL_INPUT_DUMP_START);
    IRRV_CASE(IRRV_CTRL_INPUT_DUMP_STOP);
    IRRV_CASE(IRRV_CTRL_OUTPUT_DUMP_START);
    IRRV_CASE(IRRV_CTRL_OUTPUT_DUMP_STOP);
    IRRV_CASE(IRRV_CTRL_SEI_SETTING);
    IRRV_CASE(IRRV_CTRL_SCREEN_CAPTURE_START);
    IRRV_CASE(IRRV_CTRL_SCREEN_CAPTURE_STOP);
    IRRV_CASE(IRRV_CTRL_ROI_SETTING);
    IRRV_CASE(IRRV_CTRL_CHANGE_CODEC_TYPE);
    IRRV_CASE(IRRV_CTRL_MAX_BITRATE_SETTING);
    IRRV_CASE(IRRV_CTRL_SKIP_FRAME_SETTING);
    IRRV_CASE(IRRV_CTRL_CLIENT_FEEDBACK);
    default:
        return "unknown";
    }
}

int CSendRecvMessage::irrv_op(irrv_vctrl_t& ctrl) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_client < 0) {
      ga_logger(Severity::WARNING, LOG_PREFIX "failed to send %s: not connected to server\n",
        irrv_ctrl_type(ctrl.ctrl_type));
      return -1;
    }

    irrv_vctrl_event_t ctrl_ev{};
    ctrl_ev.event.magic = IRRV_MAGIC;
    ctrl_ev.event.type  = IRRV_EVENT_VCTRL;
    ctrl_ev.event.size  = sizeof(ctrl_ev);
    ctrl_ev.event.value = 0;
    ctrl_ev.info        = ctrl;

    int ret = send(m_client, &ctrl_ev, sizeof(ctrl_ev), MSG_NOSIGNAL);
    ga_logger((ret > 0)? Severity::DBG: Severity::WARNING, LOG_PREFIX "failed to send %s: errno=%d\n",
      irrv_ctrl_type(ctrl.ctrl_type), errno);
    return ret;
}

int CSendRecvMessage::irrv_op(std::vector<irrv_vctrl_event_t>& ctrls) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_client < 0) {
      ga_logger(Severity::WARNING,
        LOG_PREFIX "failed to send ctrl array (first ctrl: %s): not connected to server\n",
        irrv_ctrl_type(ctrls[0].info.ctrl_type));
      return -1;
    }

    int ret = send(m_client, ctrls.data(), ctrls.size() * sizeof(irrv_vctrl_event_t), MSG_NOSIGNAL);
    ga_logger((ret > 0)? Severity::DBG: Severity::WARNING,
      LOG_PREFIX "failed to send ctrl array (first ctrl: %s): errno=%d\n",
      irrv_ctrl_type(ctrls[0].info.ctrl_type), errno);
    return ret;
}

int CSendRecvMessage::irrv_set_rolling_intra_refresh(unsigned int type, unsigned int cycle_size, unsigned int qp_delta) {
    irrv_vctrl_t ctrl{};
    ctrl.ctrl_type   = IRRV_CTRL_RIR_SETTING;
    ctrl.reserved[0] = type;
    ctrl.reserved[1] = cycle_size;
    ctrl.reserved[2] = qp_delta;
    ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_RIR_SETTING: type=%d, cycle_size=%d, qp_delta=%d\n",
      type, cycle_size, qp_delta);
    return irrv_op(ctrl);
}

int CSendRecvMessage::irrv_set_region_of_interest(int roi_num, AVRoI2 roi_para[]) {
    if(roi_num <= 0 || roi_num > MAX_ROI_NUM) {
        ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_ROI_SETTING: incorrect number of roi ([1,%d]): %d\n",
          MAX_ROI_NUM, roi_num);
        return -1;
    }

    ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_ROI_SETTING: roi_num=%d\n", roi_num);
    std::vector<irrv_vctrl_event_t> ctrls(roi_num);
    for (int i = 0; i < roi_num; ++i) {
        ctrls[i] = {};
        ctrls[i].event.magic = IRRV_MAGIC;
        ctrls[i].event.type  = IRRV_EVENT_VCTRL;
        ctrls[i].event.size  = sizeof(irrv_vctrl_event_t);
        ctrls[i].info.ctrl_type   = IRRV_CTRL_ROI_SETTING;
        ctrls[i].info.value       = roi_num;
        ctrls[i].info.reserved[0] = roi_para[i].x;
        ctrls[i].info.reserved[1] = roi_para[i].y;
        ctrls[i].info.reserved[2] = roi_para[i].width;
        ctrls[i].info.reserved[3] = roi_para[i].height;
        ctrls[i].info.reserved[4] = roi_para[i].roi_value;
        ga_logger(Severity::INFO, LOG_PREFIX "roi_index=%d, x=%d, y=%d, width=%d, height=%d, roi_value=%d\n",
          i, roi_para[i].x, roi_para[i].y,
          roi_para[i].width, roi_para[i].height, roi_para[i].roi_value);
    }
    return irrv_op(ctrls);
}

int CSendRecvMessage::irrv_set_min_max_qp(unsigned int min_qp, unsigned int max_qp) {
    irrv_vctrl_t ctrl{};
    ctrl.ctrl_type   = IRRV_CTRL_MIN_MAX_QP_SETTING;
    ctrl.reserved[0] = min_qp;
    ctrl.reserved[1] = max_qp;
    ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_MIN_MAX_QP_SETTING: min_qp=%d, max_qp=%d\n", min_qp, max_qp);
    return irrv_op(ctrl);
}

int CSendRecvMessage::irrv_change_resolution(unsigned int new_width, unsigned int new_height) {
    irrv_vctrl_t ctrl{};
    ctrl.ctrl_type   = IRRV_CTRL_RESOLUTION;
    ctrl.reserved[0] = new_width;
    ctrl.reserved[1] = new_height;
    ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_RESOLUTION: %dx%d\n", new_width, new_height);
    return irrv_op(ctrl);
}

/* TODO: IRRV_EVENT_VHEAD_ACK does not seem to be currently handled on ICR side.
void CSendRecvMessage::irrv_set_alpha_transmission_flag(unsigned int flags) {
    if (!irrv_sock_client_init()) {
        ga_logger(Severity::INFO, LOG_PREFIX "failed connect to server\n");
        return;
    }

    if (disconnect != sock_client_check_connect(m_client, 0)) {
        ga_logger(Severity::INFO, LOG_PREFIX "IRRV_EVENT_VHEAD_ACK: send alpha transmission: flags=%u\n", flags);
        irrv_vhead_event_t head_ev_ack;
        memset(&head_ev_ack, 0, sizeof(head_ev_ack));
        head_ev_ack.event.magic = IRRV_MAGIC;
        head_ev_ack.event.type  = IRRV_EVENT_VHEAD_ACK;
        head_ev_ack.event.size  = sizeof(head_ev_ack);
        head_ev_ack.event.value = 1;
        head_ev_ack.info.flags  = flags;
        head_ev_ack.info.width  = m_width;
        head_ev_ack.info.height = m_height;
        head_ev_ack.info.format = m_format;

        sock_client_send(m_client, &head_ev_ack, sizeof(head_ev_ack));
        if (flags == 1) {
            m_bEnableAlphaTransmission = true;
        } else {
            m_bEnableAlphaTransmission = false;
        }
    }
}
*/

int CSendRecvMessage::irrv_set_sei(unsigned int sei_type, unsigned int sei_id) {
    irrv_vctrl_t ctrl{};
    ctrl.ctrl_type   = IRRV_CTRL_SEI_SETTING;
    ctrl.reserved[0] = sei_type;
    ctrl.reserved[1] = sei_id;
    ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_SEI_SETTING: sei_type=%d, sei_id=%d\n", sei_type, sei_id);
    return irrv_op(ctrl);
}


int CSendRecvMessage::irrv_set_screen_capture_start(unsigned int capture_interval, unsigned int quality_factor) {
    irrv_vctrl_t ctrl{};
    ctrl.ctrl_type   = IRRV_CTRL_SCREEN_CAPTURE_START;
    ctrl.value       = capture_interval;
    ctrl.reserved[0] = quality_factor;
    ga_logger(Severity::INFO, LOG_PREFIX "IRRV_CTRL_SCREEN_CAPTURE_START: capture_interval=%d, quality_factor=%d\n",
      capture_interval, quality_factor);
    return irrv_op(ctrl);
}

int CSendRecvMessage::irrv_set_client_feedback(unsigned int delay, unsigned int size)
{
    irrv_vctrl_t ctrl{};
    ctrl.ctrl_type   = IRRV_CTRL_CLIENT_FEEDBACK;
    ctrl.value       = delay;
    ctrl.reserved[0] = size;
    ga_logger(Severity::DBG, LOG_PREFIX "IRRV_CTRL_CLIENT_FEEDBACK: delay=%d, size=%d\n", delay, size);
    return irrv_op(ctrl);
}

enum PipeMessageType
{
    MESSAGE_TYPE_NONE = 0,
    MESSAGE_TYPE_TCAE_FEEDBACK = 1,
    MESSAGE_TYPE_RESOLUTION_CHANGE = 2,
    MESSAGE_TYPE_SET_VIDEO_ALPHA = 3,
};

struct PipeMessage
{
    uint32_t magic;
    uint32_t type;
    uint32_t data[6];
};

void CSendRecvMessage::pipe_set_client_feedback(uint32_t delay, uint32_t size)
{
    if (!private_pipe_connect()) {
        ga_logger(Severity::INFO, LOG_PREFIX "failed connect to server\n");
        return;
    }

    PipeMessage p = {};
    p.magic = 0xdeadbeef;
    p.type = MESSAGE_TYPE_TCAE_FEEDBACK;
    p.data[0] = delay;
    p.data[1] = size;

    int ret = write(m_privatePipe, &p, sizeof(p));
    if (ret != sizeof(p))
    {
        close(m_privatePipe);
        m_privatePipe = -1;
    }
}

void CSendRecvMessage::pipe_set_resolution_change(uint32_t width, uint32_t height)
{
    if (!private_pipe_connect()) {
        ga_logger(Severity::INFO, LOG_PREFIX "failed connect to server\n");
        return;
    }

    ga_logger(Severity::INFO, LOG_PREFIX "setting resolution to %dx%d\n", width, height);

    PipeMessage p = {};
    p.magic = 0xdeadbeef;
    p.type = MESSAGE_TYPE_RESOLUTION_CHANGE;
    p.data[0] = width;
    p.data[1] = height;

    int ret = write(m_privatePipe, &p, sizeof(p));
    if (ret != sizeof(p))
    {
        close(m_privatePipe);
        m_privatePipe = -1;
    }
}

void CSendRecvMessage::pipe_set_video_alpha(uint32_t action)
{
    if (!private_pipe_connect()) {
        ga_logger(Severity::INFO, LOG_PREFIX "failed connect to server\n");
        return;
    }

    ga_logger(Severity::INFO, LOG_PREFIX "setting video alpha mode to %d\n", action);

    PipeMessage p = {};
    p.magic = 0xdeadbeef;
    p.type = MESSAGE_TYPE_SET_VIDEO_ALPHA;
    p.data[0] = action;

    int ret = write(m_privatePipe, &p, sizeof(p));
    if (ret != sizeof(p))
    {
        close(m_privatePipe);
        m_privatePipe = -1;
    }
}

bool CSendRecvMessage::private_pipe_connect()
{
    if (m_privatePipe >= 0)
        return true;

    int session_id = ga_conf_readint("android-session");
    int user_id = ga_conf_readint("user");

    std::string socket_dir = ga_conf_readstr("aic-workdir");
    stringstream pipeNameStream;

    if (ga_conf_readbool("k8s", 0) == 0) {
        socket_dir += "/ipc";
        pipeNameStream << socket_dir << "/hwc-pipe-" << session_id << "-" << user_id;
    }
    else {
        pipeNameStream << socket_dir << "/hwc-pipe-" << user_id;
    }

    string pipeName;
    pipeNameStream >> pipeName;

    ga_logger(Severity::INFO, LOG_PREFIX "Internal pipe path is: %s\n", pipeName.c_str());

    m_privatePipe = open(pipeName.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (m_privatePipe < 0)
    {
        ga_logger(Severity::ERR, LOG_PREFIX "failed to open pipe %s\n", pipeName.c_str());
        return false;
    }

    return true;
}

void CSendRecvMessage::private_pipe_disconnect()
{
    if (m_privatePipe>= 0) {
        close(m_privatePipe);
        m_privatePipe = -1;
    }
}

void CSendRecvMessage::recv_es_stream() {
    if (!irrv_sock_client_init()) {
        ga_logger(Severity::INFO, LOG_PREFIX "failed connect to server, will retry in 1 second\n");
        usleep(1000000);
        return;
    }

    if (m_bUnexpectedDisconnect) {
        irrv_set_encodestart();
        irrv_set_keyframe();
        m_bUnexpectedDisconnect = false;
    }

    struct timeval encBeginTv, encEndTv;
    struct timeval pkttv;
    uint64_t encode_start_ms, capture_time_ms, encode_end_ms;

    gettimeofday(&encBeginTv, NULL);
    encode_start_ms = encBeginTv.tv_sec * 1000 + encBeginTv.tv_usec / 1000 - 5;
    capture_time_ms = encode_start_ms;
    bool last_slice = true;

    if (m_client >= 0) {
        struct pollfd fd{};
        fd.fd = m_client;
        fd.events = POLLIN;

        int ret = poll(&fd, 1, 1000); // each 1 second
        if (ret < 0) {
            ga_logger(Severity::ERR, LOG_PREFIX "can't receive data: %s\n", strerror(errno));
            ga_logger(Severity::ERR, LOG_PREFIX "disconnected\n");
            close(m_client);
            m_client = -1;
            return;
        }
        if (ret == 0) {
            //timeout
            ga_logger(Severity::DBG, LOG_PREFIX "timeout, no data posted\n");
            return;
        }

        irrv_event_t ev{};
        ret = sock_client_recv(m_client, &ev, sizeof(ev));
        if (ret < 0)
            return;

        switch (ev.type) {
            case IRRV_EVENT_VHEAD:
            {
                ga_logger(Severity::INFO, LOG_PREFIX "IRRV_EVENT_VHEAD\n");
                irrv_vhead_t head{};

                static_assert((sizeof(irrv_vhead_event_t) - sizeof(irrv_event_t)) == sizeof(irrv_vhead_t));
                ret = sock_client_recv(m_client, &head, sizeof(irrv_vhead_t));
                if (ret < 0)
                    return;

                ga_logger(Severity::INFO, LOG_PREFIX "width=%d, height=%d, format=%d\n", head.width, head.height, head.format);

                m_width  = head.width;
                m_height = head.height;
                m_format = head.format;

                if (head.auth) {
                    irrv_send_authentication();
                }

                irrv_vhead_event_t head_ev_ack;
                memset(&head_ev_ack, 0, sizeof(head_ev_ack));
                head_ev_ack.event.magic = IRRV_MAGIC;
                head_ev_ack.event.type  = IRRV_EVENT_VHEAD_ACK;
                head_ev_ack.event.size  = sizeof(head_ev_ack);
                head_ev_ack.event.value = 1;
                head_ev_ack.info.flags  = IRRV_STREAM_VIDEO_ONLY;
                head_ev_ack.info.width  = head.width;
                head_ev_ack.info.height = head.height;
                head_ev_ack.info.format = head.format;
                sock_client_send(m_client, &head_ev_ack, sizeof(head_ev_ack));

                m_ready = true;
                m_cv.notify_all();
                break;
            }
            case IRRV_EVENT_VAUTH_ACK:
            {
                ga_logger(Severity::INFO, LOG_PREFIX "IRRV_EVENT_VAUTH_ACK\n");
                irrv_vauth_t auth{};

                static_assert((sizeof(irrv_vauth_event_t) - sizeof(irrv_event_t)) == sizeof(irrv_vauth_t));
                ret = sock_client_recv(m_client, &auth, sizeof(irrv_vauth_t));
                if (ret < 0)
                    return;

                if(AUTH_PASSED == auth.result) {
                    ga_logger(Severity::INFO, LOG_PREFIX "authentication passed!\n");
                } else if (AUTH_FAILED == auth.result) {
                    ga_logger(Severity::INFO, LOG_PREFIX "authentication failed!\n");
                }
                break;
            }
            case IRRV_EVENT_VFRAME:
            {
                ga_logger(Severity::DBG, LOG_PREFIX "IRRV_EVENT_VFRAME\n");
                irrv_vframe_t frame{};

                static_assert((sizeof(irrv_vframe_event_t) - sizeof(irrv_event_t)) == sizeof(irrv_vframe_t));
                ret = sock_client_recv(m_client, &frame, sizeof(irrv_vframe_t));
                if (ret < 0)
                    return;

                ga_logger(Severity::DBG, LOG_PREFIX "data_size=%d, video_size=%d, alpha_size=%d, flags=%d\n",
                    frame.data_size, frame.video_size, frame.alpha_size, frame.flags);

                if (!frame.data_size || frame.data_size < frame.video_size) {
                    ga_logger(Severity::ERR, "broken frame\n");
                    return;
                }

                m_frameData.resize(frame.data_size);
                uint8_t* p       = &m_frameData[0];
                int left_size = frame.data_size;  // data_size include video_size and alpha_size if the transmission data including alpha data.
                while (left_size > 0) {
                    ret = sock_client_recv(m_client, p, left_size);
                    if (ret > 0) {
                        p += ret;
                        left_size -= ret;
                    }
                }

                if (m_encout) {
                    if (m_bEnableAlphaTransmission && frame.video_size && frame.alpha_size > 0) {
                        fwrite(&m_frameData[0], 1, frame.video_size, m_encout);
                    }
                    else {
                        fwrite(&m_frameData[0], 1, frame.data_size, m_encout);
                    }
                }

                gettimeofday(&encEndTv, NULL);
                encode_end_ms = encEndTv.tv_sec * 1000 + encEndTv.tv_usec / 1000;

                // send frame by webrtc
                ga_packet_t pkt;
                ga_init_packet(&pkt);
                pkt.data = &m_frameData[0];
                pkt.size = frame.data_size;
                pkt.flags = (int)frame.flags;
                pkt.pts = 0;

                FrameMetaData* sp = (FrameMetaData*)ga_packet_new_side_data(&pkt, ga_packet_side_data_type::GA_PACKET_DATA_NEW_EXTRADATA, sizeof(FrameMetaData));
                if (sp) {
                    sp->encode_end_ms   = encode_end_ms;
                    sp->encode_start_ms = encode_start_ms;
                    sp->last_slice      = last_slice;
                    sp->capture_time_ms = capture_time_ms;

                    if (encoder_send_packet("video-encoder", 0, &pkt, pkt.pts, &pkttv) < 0) {
                        ga_logger(Severity::ERR, LOG_PREFIX "encoder_send_packet() error!\n");
                        return;
                    }

                    ga_packet_free_side_data(&pkt);
                }
                break;
            }
            default:
                ga_logger(Severity::WARNING, LOG_PREFIX "recv_es_stream: unknown event, ev.type = %x\n", ev.type);
                irrv_sock_client_disconnect();
                private_pipe_disconnect();
                m_bUnexpectedDisconnect = true;
                m_ready = false;
                break;
        }
    }
}

