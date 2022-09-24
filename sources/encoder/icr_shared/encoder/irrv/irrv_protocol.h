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

#ifndef IRRV_PROTOCOL_H
#define IRRV_PROTOCOL_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <stdint.h>
#include <map>
#include <string>

/*
    Basic communication flow :
    1) server start and listen on specific port

    2) client connect to server

    3) server send VHEAD event. And there is one field indicated authentication is on or off.

    4) if authentication is off, jump to step 6).

    5) if authentication is on, client send IRRV_EVENT_VAUTH with irrv_vauth_t to server.
       server will do nothing but wait AUTH event for authrization.
       if authrization is failed, server will closed the connection.
       if authrization is passed, server send back IRRV_EVENT_VAUTH_ACK and jump to setp 6).

    6) loop of server send IRRV_EVENT_VFRAME / IRRV_EVENT_VSLICE / IRRV_EVENT_AFRAME based on stream type
       client no need to send ACK to server now.

    7) client send IRRV_EVENT_VCTRL to server to start/pause/stop encoding, or change encoding parameters

                                   ________                   ________
                                  |        |      VHEAD      |        |
                                  |        |  -------------> |        |
                                  | Server | <-------------  | Client |
                                  |        |    VHEAD_ACK    |        |
                                  |________|                 |________|
                                                    |
                                          The filed auth in vhead
                                     indicates if it need authentication
                                                    |
                                                    |
                              --With authentication----Without authentication---
                             |                                                  |
                             |                                                  |
        ________             |                 ________                         |
       |        |           VAUTH             |        |                        |
       |        | <-------------------------  |        |                        |
       | Server |  -------------------------> | Client |                        |
       |        |          VAUTH_ACK          |        |                        |
       |________| with authentication result  |________|                        |
                              |                                                 |
                              |                                                 |
             --------------------------------- Authentication passed-------     |
            |                                                              |    |
            |                                                              |    |
 Authentication failed                                                     |    |
 Client disconnet and exit                                                 |    |
                                                                           |    |
                                                      ________                             ________
                                                     |        |    VFRAME/AFRAME/VSLICE   |        |
                                                     |        |  -----------------------> |        |
                                                     | Server | <-----------------------  | Client |
                                                     |        |             VCTRL         |        |
                                                     |________|                           |________|

 */

#define IRRV_MAGIC              ((int)0xc9c9d2c6)   /* 'IRRV' */
#define IRRV_DEFAUL_PORT        6660

#define IRRV_EVENT_VHEAD        0x1000
#define IRRV_EVENT_VHEAD_ACK    0x1001
#define IRRV_EVENT_VFRAME       0x1002
#define IRRV_EVENT_VFRAME_ACK   0x1003
#define IRRV_EVENT_VCTRL        0x1004
#define IRRV_EVENT_VCTRL_ACK    0x1005
#define IRRV_EVENT_VAUTH        0x1006
#define IRRV_EVENT_VAUTH_ACK    0x1007
#define IRRV_EVENT_AFRAME       0x1008
#define IRRV_EVENT_AFRAME_ACK   0x1009
#define IRRV_EVENT_VSLICE       0x100a
#define IRRV_EVENT_VSLICE_ACK   0x100b
#define IRRV_EVENT_MESSAGE      0x100c
#define IRRV_EVENT_MESSAGE_ACK  0x100d

#define IRRV_UUID_LEN           16
#define DEFAULT_AUTH_ID         "irrv_id"
#define DEFAULT_AUTH_KEY        "irrv_key"

typedef struct _irrv_event_t {
    unsigned int    magic;
    unsigned int    type;
    unsigned int    size;
    unsigned int    value;
} irrv_event_t;

typedef struct _irrv_vhead_t {
    unsigned int    flags;
    unsigned int    width;
    unsigned int    height;
    unsigned int    format;
    unsigned int    auth;
    unsigned int    reserved[3];
} irrv_vhead_t;

typedef struct _irrv_vhead_event_t {
    irrv_event_t    event;
    irrv_vhead_t    info;
} irrv_vhead_event_t;

typedef struct _irrv_vframe_t {
    unsigned int    flags;
    unsigned int    data_size;
    unsigned int    video_size;
    unsigned int    alpha_size;
    unsigned int    width;
    unsigned int    height;
    unsigned int    reserved[2];
} irrv_vframe_t;

typedef struct _irrv_vframe_event_t {
    irrv_event_t    event;
    irrv_vframe_t   info;
} irrv_vframe_event_t;

typedef enum _VCtrlType {
    IRRV_CTRL_NONE                  = 0,
    IRRV_CTRL_KEYFRAME_SETTING      = 1,
    IRRV_CTRL_BITRATE_SETTING       = 2,
    IRRV_CTRL_QP_SETTING            = 3,
    IRRV_CTRL_GOP_SETTING           = 4,
    IRRV_CTRL_START                 = 5,
    IRRV_CTRL_PAUSE                 = 6,
    IRRV_CTRL_STOP                  = 7,
    IRRV_CTRL_DUMP_START            = 8,
    IRRV_CTRL_DUMP_STOP             = 9,
    IRRV_CTRL_DUMP_FRAMES           = 10,
    IRRV_CTRL_RESOLUTION            = 11,
    IRRV_CTRL_FRAMERATE_SETTING     = 12,
    IRRV_CTRL_MAXFRAMESIZE_SETTING  = 13,
    IRRV_CTRL_RIR_SETTING           = 14,
    IRRV_CTRL_MIN_MAX_QP_SETTING    = 15,
    IRRV_CTRL_INPUT_DUMP_START      = 16,
    IRRV_CTRL_INPUT_DUMP_STOP       = 17,
    IRRV_CTRL_OUTPUT_DUMP_START     = 18,
    IRRV_CTRL_OUTPUT_DUMP_STOP      = 19,
    IRRV_CTRL_SEI_SETTING           = 20,
    IRRV_CTRL_SCREEN_CAPTURE_START  = 21,
    IRRV_CTRL_SCREEN_CAPTURE_STOP   = 22,
    IRRV_CTRL_ROI_SETTING           = 23,
    IRRV_CTRL_CHANGE_CODEC_TYPE     = 24,
    IRRV_CTRL_MAX_BITRATE_SETTING   = 25,
    IRRV_CTRL_SKIP_FRAME_SETTING    = 26,
    IRRV_CTRL_PROFILE_LEVEL         = 27,
    IRRV_CTRL_CLIENT_FEEDBACK       = 28,
    IRRV_CTRL_END
} VCtrlType;

typedef struct _irrv_vctrl_t {
    VCtrlType        ctrl_type;
    unsigned int     value;
    unsigned int     reserved[6];
} irrv_vctrl_t;

typedef struct _irrv_vctrl_event_t {
    irrv_event_t    event;
    irrv_vctrl_t    info;
} irrv_vctrl_event_t;

enum {
    IRRV_STREAM_FORMAT_UNKNOWN      = -1,
    IRRV_STREAM_FORMAT_RGBA_RAW     = 0,
    IRRV_STREAM_FORMAT_H264_RAW     = 1,
    IRRV_STREAM_FORMAT_H264_RTMP    = 2,
    IRRV_STREAM_FORMAT_H265_RAW     = 3,
    IRRV_STREAM_FORMAT_MJPEG        = 4,
    IRRV_STREAM_FORMAT_AV1_RAW      = 5,

    IRRV_STREAM_FORMAT_COUNT
};

enum {
    IRRV_STREAM_VIDEO_ONLY = 0,
    IRRV_STREAM_VIDEO_ALPHA = 1,
    IRRV_STREAM_TYPE_COUNT
};

typedef enum {
    AUTH_FAILED,
    AUTH_PASSED,
} AuthResult;

typedef unsigned char irrv_uuid_t[IRRV_UUID_LEN];

typedef struct _irrv_vauth_t {
    irrv_uuid_t    id;
    irrv_uuid_t    key;
    AuthResult     result;
    unsigned int   reserved[6];
} irrv_vauth_t;

typedef struct _irrv_vauth_event_t {
    irrv_event_t    event;
    irrv_vauth_t    info;
} irrv_vauth_event_t;

typedef enum {
    NONE  = 0,
    MJPEG = 7,    // equal to AV_CODEC_ID_MJPEG
    H264  = 27,   // equal to AV_CODEC_ID_H264
    H265  = 173,  // equal to AV_CODEC_ID_H265
    AV1   = 226,  // equal to AV_CODEC_ID_AV1
} EncodeType;

typedef enum _MessageType {
    IRRV_MESSAGE_NONE                = 0,
    IRRV_MESSAGE_VIDEO_FORMAT_CHANGE = 1,
    IRRV_MESSAGE_END
} MessageType;
typedef struct _irrv_message_t {
    MessageType      msg_type;
    unsigned int     value;
    unsigned int     reserved[6];
} irrv_message_t;

typedef struct _irrv_message_event_t {
    irrv_event_t    event;
    irrv_message_t  msg;
} irrv_message_event_t;

#ifdef __cplusplus
extern "C" {
#endif

int irrv_wirteback(void *opaque, uint8_t *data, size_t size);
int irrv_wirteback2(void *opaque, uint8_t *data, size_t size, int type);
void irrv_close(void *opaque);
int irrv_checknewconn(void *opaque);
bool irrv_check_authentication(irrv_uuid_t id, irrv_uuid_t key);
int irrv_send_message(void *opaque, int msg, unsigned int value);

#ifdef __cplusplus
}
#endif

#endif // IRRV_PROTOCOL_H
