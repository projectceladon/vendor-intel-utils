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

#ifndef __INTEL_QOS_H__
#define __INTEL_QOS_H__

#include <iostream>
#include "ga-common.h"
#include "dpipe.h"
#include "rtspconf.h"
#include "ga-msg.h"
#define    QOS_POOLSIZE      4

/**
* Struct: IRD_CURSOR_INFO
* @brief This structure is used by Titan SDK client to get cursor info for current frame.
*/
typedef struct _QosInfo
{
    unsigned int   frameno;
    unsigned int   framesize;
    unsigned int   bitrate;
    unsigned int   captureFps;
    struct timeval eventime;
    double         capturetime;
    double         encodetime;
    unsigned int   estimated_bw;
} QosInfo;

typedef struct _QosClintInfo
{
    unsigned int   framesize;
    struct   timeval decodeSubmit;
    double         netlatency;
} QosClientInfo;
typedef struct _QosInfoFull
{
    unsigned int   frameno;
    unsigned int   framesize;
    unsigned int   bitrate;
    double         capturetime;
    double         encodetime;
    struct   timeval         decodeSubmit;
    double         netlatency;
    struct   timeval         eventtime;
    unsigned int captureFps;
}QosInfoFull;


EXPORT int start_qos_client(struct RTSPConf *conf);
EXPORT void restart_qos_service();
EXPORT unsigned char *get_qos_data();
EXPORT int set_qos_data(unsigned char *pBuffer, unsigned int len);
EXPORT void stop_qos_service();
EXPORT int queue_qos(QosInfo qosInfo);
EXPORT int start_qos_service(struct RTSPConf *conf);
EXPORT void queue_qos_client_info(QosClientInfo qosClientInfo);

#endif
