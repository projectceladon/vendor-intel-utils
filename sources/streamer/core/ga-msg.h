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

#pragma once
typedef enum _reqID {
    HANDSHAKE = 0x8001,
    ENUMARATE = 0x8002,
#ifdef WIN32
    PLAY = 0x8003,
#else
    PLAY_ = 0x8003,
#endif
    CLOSE = 0x8005,
    CURSOR = 0x8006,
    QOSREPORT = 0x8007
}REQ_ID;

typedef enum _respID {
    HANDSHAKE_RESP = 0x9001,
    ENUMARATE_RESP = 0x9002,
    PLAY_RESP = 0x9003,
    CLOSE_RESP = 0x9005,
    CURSOR_INFO_RESP = 0x9006,
    QOS_INFO_RESP = 0x9007
}RESP_ID;

typedef struct msg_req_info {
    int  magic;      // 0x55AA55AA
    REQ_ID  msgHdr;     // msgID
    int  payloadLen; // length of the payload
    unsigned char payload[4096];
}MSG_REQ_INFO_S;

typedef struct msg_resp_info {
    int  magic;      // 0x55AA55AA
    int  msgHdr;     // msgID
    int  payloadLen; // length of the payload
}MSG_REP_INFO_S;

typedef struct _play_req_info {
    unsigned int gameid;
    unsigned int fps;
    unsigned int kbps;
    unsigned int qualityMode;
    unsigned int width;
    unsigned int height;
}PLAY_REQ_INFO;

typedef struct _play_resp_info {
    unsigned int streamPort;
    unsigned int controlPort;
    unsigned int mouseMode;
    unsigned char ipaddr[128];
}PLAY_RESP_INFO;

typedef struct thread_info {
    struct RTSPConf    conf;
}THREAD_INFO_S;
