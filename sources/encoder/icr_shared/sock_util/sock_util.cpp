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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include "sock_util.h"

#define SOCK_USE_USLEEP 1

int64_t sock_get_currtime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

bool sock_timeout(int64_t start, int64_t duration_us)
{
    int64_t curr_time = sock_get_currtime();
    if (curr_time - start > duration_us) {
        return true;
    } else {
        return false;
    }
}

int sock_usleep(unsigned usec)
{
#if SOCK_USE_USLEEP
    return usleep(usec);
#else
    struct timespec ts;

    ts.tv_sec  = usec / 1000000; // seconds
    ts.tv_nsec = usec % 1000000 * 1000; // and nanoseconds
    while (nanosleep(&ts, &ts) < 0 && errno == EINTR);

    return 0;
#endif
}



