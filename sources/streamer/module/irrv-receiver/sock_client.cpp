// Copyright (C) 2016-2022 Intel Corporation
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

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "ga-conf.h"
#include "ga-common.h"
#include "sock_client.h"

#define LOG_PREFIX "irrv-receiver: sock: "

int sock_client_init() {
    std::string icr_ip = ga_conf_readstr("icr-ip");
    int icr_port = ga_conf_readint("icr-port");
    int session = ga_conf_readint("android-session");

    if (icr_port <= 0) {
        ga_logger(Severity::ERR, LOG_PREFIX "invalid port: %d\n", icr_port);
        return -1;
    }
    if (session < 0) {
        session = 0;
    }
    icr_port += 1000 + session;

    ga_logger(Severity::INFO, LOG_PREFIX "initializing %s:%d...\n", icr_ip.c_str(), icr_port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ga_logger(Severity::ERR, LOG_PREFIX "create client socket failed\n");
        return -1;
    }

    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        ga_logger(Severity::ERR, LOG_PREFIX "set socketopt(REUSEADDR) failed\n");
        close(fd);
        return -1;
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&on, sizeof(int)) < 0) {
        ga_logger(Severity::ERR, LOG_PREFIX "set socketopt(TCP_NODELAY) failed\n");
        close(fd);
        return -1;
    }

    struct sockaddr_in serv_addr_in;
    memset(&serv_addr_in, 0, sizeof(struct sockaddr_in));
    serv_addr_in.sin_family = AF_INET;
    serv_addr_in.sin_port   = htons(icr_port);
    inet_pton(AF_INET, icr_ip.c_str(), &serv_addr_in.sin_addr);
    int ret = connect(fd, (struct sockaddr *)&serv_addr_in, sizeof(struct sockaddr_in));
    if (ret < 0) {
        ga_logger(Severity::ERR, LOG_PREFIX "failed to connect\n");
        close(fd);
        return -1;
    }

    int flag = 1;
    if (ioctl(fd, FIONBIO, &flag) < 0) {
        ga_logger(Severity::ERR, LOG_PREFIX "ioctl(FIONBIO) failed\n!");
        close(fd);
        return -1;
    }

    ga_logger(Severity::INFO, LOG_PREFIX "initializing %s:%d: SUCCESS\n)", icr_ip.c_str(), icr_port);
    return fd;
}

sock_conn_status_t sock_client_check_connect(int fd, int timeout_ms) {
    sock_conn_status_t result = normal;
    int nsel                  = 0;
    fd_set rfds;
    struct timeval timeout;
    int nread = 0;

    if (fd < 0) {
        return result;
    }

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    timeout.tv_sec  = 0;
    timeout.tv_usec = timeout_ms * 1000;
    nsel            = select(fd + 1, &rfds, NULL, NULL, &timeout);
    switch (nsel) {
        case -1:
            ga_logger(Severity::ERR, LOG_PREFIX "select() failed!\n");
            result = normal;
            break;

        case 0:
            result = normal;
            break;

        default:
            if (FD_ISSET(fd, &rfds)) {
                if (-1 != ioctl(fd, FIONREAD, &nread)) {
                    if (nread != 0) {
                        result = readable;
                    } else {
                        result = disconnect;
                    }
                } else {
                    ga_logger(Severity::WARNING, LOG_PREFIX "ioctl(FIONREAD) failed");
                    result = normal;
                }
            } else {
                result = normal;
            }
    }
    return result;
}

int sock_client_send(int fd, const void *data, size_t datalen) {
    ga_logger(Severity::DBG, LOG_PREFIX "send: fd=%d, datalen=%lld\n", fd, datalen);

    unsigned char *p_src = (unsigned char *)data;
    size_t left_len      = datalen;
    int retry_count      = 30;

    while (left_len > 0) {
        int ret = send(fd, p_src, left_len, MSG_NOSIGNAL);
        if (ret > 0) {
            left_len -= ret;
            p_src += ret;
        } else {
            if ((errno == EINTR) || (errno == EAGAIN)) {
                if ((retry_count--) < 0) {
                    ga_logger(Severity::ERR, LOG_PREFIX "send: giving up after 30 retries\n");
                    return ret;
                }
                usleep(1000);
                continue;
            } else {
                ga_logger(Severity::ERR, LOG_PREFIX "send: failed: %s\n", strerror(errno));
                return ret;
            }
        }
    }
    return (int)(datalen - left_len);
}

int sock_client_recv(int fd, void *data, size_t datalen) {
    int ret = recv(fd, data, datalen, MSG_DONTWAIT);
    if (ret < 0 && !(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) {
        ga_logger(Severity::ERR, LOG_PREFIX "recv: failed: %s\n", strerror(errno));
    }
    return ret;
}
