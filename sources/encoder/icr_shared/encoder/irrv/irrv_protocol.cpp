// Copyright (C) 2018-2022 Intel Corporation
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

#include "irrv_impl.h"
#include "sock_client.h"
#include "sock_server.h"

#include "../stream.h"
#include "TimeLog.h"
#include "CTransLog.h"

#define MAX_CLIENTS	 8
#define MAX_MESSAGE	 128

CTransLog IrrvLog("IRRV");

extern void write_file_output(unsigned char *data, size_t size);

std::map<sock_server_t*, sock_client_proxy_t*> clients;
std::map<sock_server_t*, bool> auth_pass;

// currently use hard coded uuid key
// customer may use their own key mechanism
irrv_uuid_t auth_id  = DEFAULT_AUTH_ID;
irrv_uuid_t auth_key = DEFAULT_AUTH_KEY;

unsigned long g_wb_frame_idx = 0;

const std::map<VCtrlType, std::string> VCtrlTypeMap = {
    { IRRV_CTRL_NONE                 , "IRRV_CTRL_NONE                 " },
    { IRRV_CTRL_KEYFRAME_SETTING     , "IRRV_CTRL_KEYFRAME_SETTING     " },
    { IRRV_CTRL_BITRATE_SETTING      , "IRRV_CTRL_BITRATE_SETTING      " },
    { IRRV_CTRL_QP_SETTING           , "IRRV_CTRL_QP_SETTING           " },
    { IRRV_CTRL_GOP_SETTING          , "IRRV_CTRL_GOP_SETTING          " },
    { IRRV_CTRL_START                , "IRRV_CTRL_START                " },
    { IRRV_CTRL_PAUSE                , "IRRV_CTRL_PAUSE                " },
    { IRRV_CTRL_STOP                 , "IRRV_CTRL_STOP                 " },
    { IRRV_CTRL_DUMP_START           , "IRRV_CTRL_DUMP_START           " },
    { IRRV_CTRL_DUMP_STOP            , "IRRV_CTRL_DUMP_STOP            " },
    { IRRV_CTRL_DUMP_FRAMES          , "IRRV_CTRL_DUMP_FRAMES          " },
    { IRRV_CTRL_RESOLUTION           , "IRRV_CTRL_RESOLUTION           " },
    { IRRV_CTRL_FRAMERATE_SETTING    , "IRRV_CTRL_FRAMERATE_SETTING    " },
    { IRRV_CTRL_MAXFRAMESIZE_SETTING , "IRRV_CTRL_MAXFRAMESIZE_SETTING " },
    { IRRV_CTRL_RIR_SETTING          , "IRRV_CTRL_RIR_SETTING          " },
    { IRRV_CTRL_MIN_MAX_QP_SETTING   , "IRRV_CTRL_MIN_MAX_QP_SETTING   " },
    { IRRV_CTRL_INPUT_DUMP_START     , "IRRV_CTRL_INPUT_DUMP_START     " },
    { IRRV_CTRL_INPUT_DUMP_STOP      , "IRRV_CTRL_INPUT_DUMP_STOP      " },
    { IRRV_CTRL_OUTPUT_DUMP_START    , "IRRV_CTRL_OUTPUT_DUMP_START    " },
    { IRRV_CTRL_OUTPUT_DUMP_STOP     , "IRRV_CTRL_OUTPUT_DUMP_STOP     " },
    { IRRV_CTRL_SEI_SETTING          , "IRRV_CTRL_SEI_SETTING          " },
    { IRRV_CTRL_SCREEN_CAPTURE_START , "IRRV_CTRL_SCREEN_CAPTURE_START " },
    { IRRV_CTRL_SCREEN_CAPTURE_STOP  , "IRRV_CTRL_SCREEN_CAPTURE_STOP  " },
    { IRRV_CTRL_ROI_SETTING          , "IRRV_CTRL_ROI_SETTING          " },
    { IRRV_CTRL_CHANGE_CODEC_TYPE    , "IRRV_CTRL_CHANGE_CODEC_TYPE    " },
    { IRRV_CTRL_MAX_BITRATE_SETTING  , "IRRV_CTRL_MAX_BITRATE_SETTING  " },
    { IRRV_CTRL_SKIP_FRAME_SETTING   , "IRRV_CTRL_SKIP_FRAME_SETTING   " },
    { IRRV_CTRL_PROFILE_LEVEL        , "IRRV_CTRL_PROFILE_LEVEL        " },
    { IRRV_CTRL_CLIENT_FEEDBACK      , "IRRV_CTRL_CLIENT_FEEDBACK    " },
    { IRRV_CTRL_END                  , "IRRV_CTRL_END                  " },
};

static inline int ConvertCodecTypeToStreamFormat(int codec_id) {
    switch (codec_id)
    {
    case AV1:
        return IRRV_STREAM_FORMAT_AV1_RAW;
    case MJPEG:
        return IRRV_STREAM_FORMAT_MJPEG;
    case H264:
        return IRRV_STREAM_FORMAT_H264_RAW;
    case H265:
        return IRRV_STREAM_FORMAT_H265_RAW;
    default:
        return IRRV_STREAM_FORMAT_UNKNOWN;
    }
}

bool irrv_check_authentication(irrv_uuid_t id, irrv_uuid_t key) {
    bool res = false;

    if(!memcmp(id, auth_id, IRRV_UUID_LEN) && !memcmp(key, auth_key, IRRV_UUID_LEN)) {
            res = true;
    }

    return res;
}

bool irrv_get_auth_state(sock_server_t *server) {
    auto it  = auth_pass.find(server);
    bool ret = false;

    if(it != auth_pass.end()) {
        ret = it->second;
    }
    return ret;
}

bool irrv_have_client(sock_server_t *server) {
    auto it  = clients.find(server);
    bool ret = false;

    if(it != clients.end() && it->second != NULL) {
        ret = true;
    }
    return ret;
}

int irrv_writeback(void *opaque, uint8_t *data, size_t size)
{
    sock_server_t *server = static_cast<sock_server_t*> (opaque);
    bool auth_required    = irr_stream_getAuthFlag();

    ATRACE_INDEX("irrv_writeback", (int)g_wb_frame_idx, 0);
    TimeLog timelog("IRRB_irrv_writeback", 0, g_wb_frame_idx++, 0); 

    //IrrvLog.Info("send frame size %lu\n", size);

    irrv_checknewconn(opaque);

    if (server) {
        //IrrvLog.Info("+++server here\n");
        if(irrv_have_client(server) && (!auth_required || irrv_get_auth_state(server))) {
            //IrrvLog.Info("send frame event\n");
            int  width = irr_stream_get_encode_new_width();
            int  height = irr_stream_get_encode_new_height();

            if (width <= 0 || height <= 0) {
                width = irr_stream_get_width();
                height = irr_stream_get_height();
            }

            irrv_vframe_event_t frame_ev;
            memset(&frame_ev, 0, sizeof(frame_ev));
            frame_ev.event.magic = IRRV_MAGIC;
            frame_ev.event.type  = IRRV_EVENT_VFRAME;
            frame_ev.event.size  = sizeof(frame_ev);
            frame_ev.event.value = 0;
            frame_ev.info.flags  = 0;
            frame_ev.info.data_size = size;
            frame_ev.info.width = width  > 0 ? width : 720;
            frame_ev.info.height = height > 0 ? height : 1280;
            sock_server_send(server, clients[server], &frame_ev, sizeof(frame_ev));

            //IrrvLog.Info("send frame data, size(%lu)", size);
#if 1
            sock_server_send(server, clients[server], data, size);
#else 
            int leftsize = size;
            char *tmp = (char *)data;
            while(leftsize > 0) {
                ret = sock_server_send(server, client, data, size);
                ret = sock_server_send(server, clients[server], tmp, leftsize);
                if (ret < 0 && !(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) {
                    IrrvLog.Warn("socket send() error: %d (%s)!!\n", errno, strerror(errno));
                    break;
                }
                if(ret >= 0) {
                    tmp += ret;
                    leftsize -= ret;
                }
            }
#endif
        }
    }
    return 0;
}

int irrv_writeback2(void *opaque, uint8_t *data, size_t size, int type)
{
    sock_server_t *server = static_cast<sock_server_t*> (opaque);
    bool auth_required    = irr_stream_getAuthFlag();

    irrv_checknewconn(opaque);

    if (server) {
        switch(type) {
            case NONE:
                break;
            case H264:
            case MJPEG:
                if(irrv_have_client(server) && (!auth_required || irrv_get_auth_state(server))) {
                    int  width = irr_stream_get_encode_new_width();
                    int  height = irr_stream_get_encode_new_height();

                    if (width <= 0 || height <= 0) {
                        width = irr_stream_get_width();
                        height = irr_stream_get_height();
                    }

                    irrv_vframe_event_t frame_ev;
                    memset(&frame_ev, 0, sizeof(frame_ev));
                    frame_ev.event.magic = IRRV_MAGIC;
                    frame_ev.event.type  = IRRV_EVENT_VFRAME;
                    frame_ev.event.size  = sizeof(frame_ev);
                    frame_ev.event.value = 0;
                    frame_ev.info.flags  = type;
                    frame_ev.info.data_size = size;
                    frame_ev.info.width = width  > 0 ? width : 720;
                    frame_ev.info.height = height > 0 ? height : 1280;
                    sock_server_send(server, clients[server], &frame_ev, sizeof(frame_ev));
                    sock_server_send(server, clients[server], data, size);
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

int irrv_send_message(void *opaque, int msg, unsigned int value)
{
    sock_server_t *server = static_cast<sock_server_t*> (opaque);

    if (server) {
        if (irrv_have_client(server)) {
            irrv_message_event_t message_ev = { 0 };
            message_ev.event.magic = IRRV_MAGIC;
            message_ev.event.type  = IRRV_EVENT_MESSAGE;
            message_ev.event.size  = sizeof(message_ev);
            message_ev.event.value = 0;
            message_ev.msg.msg_type = (MessageType)msg;
            message_ev.msg.value    = value;
            sock_server_send(server, clients[server], &message_ev, sizeof(message_ev));
            IrrvLog.Info("Send message: %d, value: %lu", msg, value);
        }
    }
    return 0;
}

int irrv_checknewconn(void *opaque)
{
    sock_server_t *server = static_cast<sock_server_t*> (opaque);

    if (server) {
        //IrrvLog.Info("+++server here\n");
        bool auth_required = irr_stream_getAuthFlag();
        if( !irrv_have_client(server) && sock_server_has_newconn(server, 0)) {
            IrrvLog.Info("%s: %d: has new connection, create client proxy \n", __func__, __LINE__);
            clients[server] = sock_server_create_client(server);

            if(!clients[server]) {
                IrrvLog.Warn("create client proxy failed\n");
            }

            irr_stream_incClient();

            int  width = irr_stream_get_encode_new_width();
            int  height = irr_stream_get_encode_new_height();

            if (width <= 0 || height <= 0) {
                width = irr_stream_get_width();
                height = irr_stream_get_height();
            }

            irrv_vhead_event_t head_ev;
            memset(&head_ev, 0, sizeof(head_ev));
            head_ev.event.magic = IRRV_MAGIC;
            head_ev.event.type  = IRRV_EVENT_VHEAD;
            head_ev.event.size  = sizeof(head_ev);
            head_ev.event.value = 0;
            head_ev.info.flags  = 0;
            head_ev.info.width  = width  > 0 ? width:720;
            head_ev.info.height = height > 0 ? height:1280;
            IrrvLog.Info("%s: %d: width=%d, height=%d\n", __func__, __LINE__, width, height);
            head_ev.info.format = ConvertCodecTypeToStreamFormat(irr_stream_get_encoder_type());
            head_ev.info.auth   = auth_required;
            sock_server_send(server, clients[server], &head_ev, sizeof(head_ev));
        }

        if(sock_server_clients_readable(server, 0)) {
            if(irrv_have_client(server)) {
                // check client status, read ack event, or close it if disconnected
                switch(sock_server_check_connect(server, clients[server])) {
                    case readable:
                        irrv_event_t ev;
                        memset(&ev, 0, sizeof(ev));
                        sock_server_recv(server, clients[server], &ev, sizeof(ev));
                        IrrvLog.Info("recv client event magic = 0x%x, type = 0x%x\n", ev.magic, ev.type);

                        if (IRRV_EVENT_VAUTH == ev.type && auth_required) {
                            IrrvLog.Info("receive authentication request\n");
                            irrv_vauth_t vauth;
                            memset(&vauth, 0, sizeof(vauth));
                            sock_server_recv(server, clients[server], &vauth, sizeof(irrv_vauth_t));

                            irrv_vauth_event_t auth_ev;
                            memset(&auth_ev, 0, sizeof(auth_ev));
                            auth_ev.event.magic = IRRV_MAGIC;
                            auth_ev.event.type  = IRRV_EVENT_VAUTH_ACK;
                            auth_ev.event.size  = sizeof(auth_ev);

                            if(irrv_check_authentication(vauth.id, vauth.key)) {
                                auth_pass[server] = true;
                                IrrvLog.Info("sock client authentication passed\n");
                                auth_ev.info.result     = AUTH_PASSED;
                                sock_server_send(server, clients[server], &auth_ev, sizeof(auth_ev));
                            } else {
                                auth_pass[server] = false;
                                IrrvLog.Info("sock client authentication failed\n");
                                auth_ev.info.result     = AUTH_FAILED;
                                sock_server_send(server, clients[server], &auth_ev, sizeof(auth_ev));

                                sock_server_close_client(server, clients[server]);
                                clients[server] = NULL;
                            }
                        }

                        if (IRRV_EVENT_VCTRL == ev.type && (!auth_required || irrv_get_auth_state(server))) {
                            irrv_vctrl_t vctrl;
                            sock_server_recv(server, clients[server], &vctrl, sizeof(irrv_vctrl_t));
                            IrrvLog.Info("dynamic encode setting, type = %s\n", VCtrlTypeMap.at(vctrl.ctrl_type).c_str());
#ifdef FFMPEG_v42
                            int roi_index = 0;
                            int roi_num   = 0;
                            AVRoI roi_para[MAX_ROI_NUM] = {0};
                            irrv_event_t placeholder;
#endif
                            switch (vctrl.ctrl_type) {
                            case IRRV_CTRL_KEYFRAME_SETTING:
                                irr_stream_force_keyframe(vctrl.value);
                                break;
                            case IRRV_CTRL_BITRATE_SETTING:
                                irr_stream_set_bitrate(vctrl.value);
                                break;
                            case IRRV_CTRL_MAX_BITRATE_SETTING:
                                irr_stream_set_max_bitrate(vctrl.value);
                                break;
                            case IRRV_CTRL_SKIP_FRAME_SETTING:
                                if (vctrl.value == 1) {
                                    irr_stream_set_skipframe(true);
                                }
                                else if (vctrl.value == 0){
                                    irr_stream_set_skipframe(false);
                                }
                                else {
                                    IrrvLog.Info("irrv_checknewconn: receive error skip frame setting value, vctrl.value = %d, correct value: 1 -- enable skip frame, 0 -- disable\n", vctrl.value);
                                }
                                break;
                            case IRRV_CTRL_QP_SETTING:
                                irr_stream_set_qp(vctrl.value);
                                break;
                            case IRRV_CTRL_START:
                                irr_stream_setEncodeFlag(true);
                                irr_stream_setTransmitFlag(true);
                                irr_stream_first_start_encdoding(true);
                                break;
                            case IRRV_CTRL_PAUSE:
                                irr_stream_setTransmitFlag(false);
                                break;
                            case IRRV_CTRL_STOP:
                                irr_stream_setEncodeFlag(false);
                                irr_stream_setTransmitFlag(false);
                                irr_stream_first_start_encdoding(false);
                                break;
                            case IRRV_CTRL_DUMP_START:
                                irr_stream_runtime_writer_start(IRR_RT_MODE_BOTH);
                                break;
                            case IRRV_CTRL_DUMP_STOP:
                                irr_stream_runtime_writer_stop(IRR_RT_MODE_BOTH);
                                break;
                            case IRRV_CTRL_DUMP_FRAMES:
                                irr_stream_runtime_writer_start_with_frame_num(vctrl.value);
                                break;
                            case IRRV_CTRL_FRAMERATE_SETTING:
                                irr_stream_set_framerate(vctrl.value);
                                break;
                            case IRRV_CTRL_MAXFRAMESIZE_SETTING:
                                irr_stream_set_max_frame_size(vctrl.value);
                                break;
                            case IRRV_CTRL_RIR_SETTING:
                                irr_stream_set_rolling_intra_refresh(vctrl.reserved[0], vctrl.reserved[1], vctrl.reserved[2]);
                                break;
                            case IRRV_CTRL_MIN_MAX_QP_SETTING:
                                irr_stream_set_min_max_qp(vctrl.reserved[0], vctrl.reserved[1]);
                                break;
                            case IRRV_CTRL_RESOLUTION:
                                irr_stream_change_resolution(vctrl.reserved[0], vctrl.reserved[1]);
                                break;
                            case IRRV_CTRL_CHANGE_CODEC_TYPE:
                                irr_stream_change_codec((AVCodecID)vctrl.value);
                                IrrvLog.Info("dynamic encode setting IRRV_CTRL_CHANGE_CODEC_TYPE, vctrl.value = %d\n", vctrl.value);
                                break;
                            case IRRV_CTRL_INPUT_DUMP_START:
                                irr_stream_runtime_writer_start(IRR_RT_MODE_INPUT);
                                break;
                            case IRRV_CTRL_INPUT_DUMP_STOP:
                                irr_stream_runtime_writer_stop(IRR_RT_MODE_INPUT);
                                break;
                            case IRRV_CTRL_OUTPUT_DUMP_START:
                                irr_stream_runtime_writer_start(IRR_RT_MODE_OUTPUT);
                                break;
                            case IRRV_CTRL_OUTPUT_DUMP_STOP:
                                irr_stream_runtime_writer_stop(IRR_RT_MODE_OUTPUT);
                                break;
                            case IRRV_CTRL_SEI_SETTING:
                                irr_stream_set_sei(vctrl.reserved[0], vctrl.reserved[1]);
                                break;
                            case IRRV_CTRL_GOP_SETTING:
                                irr_stream_set_gop_size(vctrl.value);
                                break;
                            case IRRV_CTRL_SCREEN_CAPTURE_START:
                                irr_sream_set_screen_capture_interval(vctrl.value);
                                irr_stream_set_screen_capture_quality(vctrl.reserved[0]);
                                irr_stream_set_screen_capture_flag(true);
                                break;
                            case IRRV_CTRL_SCREEN_CAPTURE_STOP:
                                irr_stream_set_screen_capture_flag(false);
                                break;
                            case IRRV_CTRL_PROFILE_LEVEL:
                                irr_stream_change_profile_level(vctrl.reserved[0], vctrl.reserved[1]);
                                break;
#ifdef FFMPEG_v42
                            case IRRV_CTRL_ROI_SETTING:
                                roi_num = vctrl.value;

                                do {
                                    roi_para[roi_index].x         = vctrl.reserved[0];
                                    roi_para[roi_index].y         = vctrl.reserved[1];
                                    roi_para[roi_index].width     = vctrl.reserved[2];
                                    roi_para[roi_index].height    = vctrl.reserved[3];
                                    roi_para[roi_index].roi_value = vctrl.reserved[4];
                                    ++roi_index;

                                    if(--roi_num) {
                                        sock_server_recv(server, clients[server], &placeholder, sizeof(placeholder));
                                        sock_server_recv(server, clients[server], &vctrl, sizeof(irrv_vctrl_t));
                                    }
                                } while(roi_num);

                                irr_stream_set_region_of_interest(vctrl.value, roi_para);
                                break;
#endif
                            case IRRV_CTRL_CLIENT_FEEDBACK:
                                irr_stream_set_client_feedback(vctrl.value, vctrl.reserved[0]);
                                break;
                            default:
                                IrrvLog.Warn("ERROR encode setting (type < %d)\n", IRRV_CTRL_END);
                                break;
                            }
                        }
                        break;

                    case disconnect:
                    {
                        IrrvLog.Info("client disconnected, close it\n");
                        sock_server_close_client(server, clients[server]);
                        clients[server]   = NULL;
                        auth_pass[server] = false;
                        irr_stream_decClient();
                        int iClientNum = irr_stream_getClientNum();
                        if (iClientNum == 0) {
                            irr_stream_setEncodeFlag(false);
                            irr_stream_setTransmitFlag(false);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

    }
    return irr_stream_getClientNum();
}

void irrv_close(void *opaque)
{
    IrrvLog.Info("close client\n");
    sock_server_t *server = static_cast<sock_server_t*> (opaque);
    if (server) {
        if(irrv_have_client(server)) {
            sock_server_close_client(server, clients[server]);
            clients[server] = NULL;
        }

        auto it = auth_pass.find(server);
        if (it != auth_pass.end())
            it->second = false;
            // auth_pass.erase(it);

        sock_server_close(server);
    }
}

