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

#ifndef SOCK_UTIL_H
#define SOCK_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

typedef int SOCK_BOOL;
#define SOCK_TRUE	1
#define SOCK_FALSE	0

#define SOCK_MAX_PATH_LEN		128

#define UNUSED(expr) do { (void)(expr); } while (0)

#ifndef LOG_TAG
#define LOG_TAG "sock_util"
#endif

#if BUILD_FOR_HOST
#include <stdio.h>
#include <memory.h>
#define SOCK_UTIL_DEFAULT_PORT  6666
#define SOCK_UTIL_DEFAULT_PATH  "../workdir/ipc/sock-util-default"
#define SOCK_DISPLAY_DAEMON_PATH "../workdir/ipc/display-sock"
#define sock_log		printf
#else
#include "encoder_comm.h"
#define SOCK_UTIL_DEFAULT_PORT  6666
#define SOCK_UTIL_DEFAULT_PATH  "/ipc/sock-util-default"
#define SOCK_DISPLAY_DAEMON_PATH "/ipc/display-sock"

#define sock_log		ALOGI
#endif

typedef enum _sock_conn_status{
	disconnect	=-1,
	normal		=0,
	readable	=1
}sock_conn_status_t;


#define DEBUG_SOCK_SERVER	0
#define DEBUG_SOCK_CLIENT	0

typedef enum _sock_conn_type{
	SOCK_CONN_TYPE_ABS_SOCK  = 0,
	SOCK_CONN_TYPE_UNIX_SOCK = 1,
	SOCK_CONN_TYPE_INET_SOCK = 2,
}SOCK_conn_type_t;


#define SOCK_USLEEP_TIME  	100      //sleep 100us
#define SOCK_TIMEOUT_TIME 	10000000 //timeout 10s


#ifdef __cplusplus
extern "C" {
#endif

int64_t sock_get_currtime();
bool    sock_timeout(int64_t start, int64_t duration_us);
int     sock_usleep(unsigned usec);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* SOCK_UTIL_H */
