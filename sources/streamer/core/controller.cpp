// Copyright (C) 2022 Intel Corporation

/*
 * Copyright (c) 2013 Chun-Ying Huang
 *
 * This file is part of GamingAnywhere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifndef WIN32
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <condition_variable>
#include <mutex>
#include <shared_mutex>

#include "ga-common.h"
#include "controller.h"
#ifdef WIN32
#include "encoder-common.h"
#include "QoSMgt.h"
#endif
using namespace std;

static char *myctrlid = NULL;
static bool ctrlenabled = true;
//
static std::mutex wakeup_mutex;
static std::condition_variable wakeup;
static int ctrlsocket = -1;
static struct sockaddr_in ctrlsin;
// message queue
static std::mutex queue_mutex;
static int qhead, qtail, qsize, qunit;
static unsigned char *qbuffer = NULL;

static msgfunc replay = NULL;

#ifdef WIN32
static unsigned long
#else
static in_addr_t
#endif
name_resolve(const char *hostname) {
    struct in_addr addr;
    struct hostent *hostEnt;
    if((addr.s_addr = inet_addr(hostname)) == INADDR_NONE) {
        if((hostEnt = gethostbyname(hostname)) == NULL)
            return INADDR_NONE;
        bcopy(hostEnt->h_addr, (char *) &addr.s_addr, hostEnt->h_length);
    }
    return addr.s_addr;
}

// queue routines
int
ctrl_queue_init(int size, int maxunit) {
    qunit = maxunit + sizeof(struct queuemsg);
    qsize = size - (size % qunit);
    qhead = qtail = 0;
    if((qbuffer = (unsigned char*) malloc(qsize)) == NULL) {
        return -1;
    }
    ga_logger(Severity::INFO, "controller queue: initialized size=%d (%d units)\n",
        qsize, qsize/qunit);
    return 0;
}

void
ctrl_queue_free() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if(qbuffer != NULL)
        free(qbuffer);
    qbuffer = NULL;
    qhead = qtail = qsize = 0;
}

struct queuemsg *
ctrl_queue_read_msg() {
    struct queuemsg *msg;
    //
    std::lock_guard<std::mutex> lock(queue_mutex);
    if(qbuffer == NULL) {
        ga_logger(Severity::INFO, "controller queue: buffer released.\n");
        return NULL;
    }
    if(qtail == qhead) {
        // queue is empty
        msg = NULL;
    } else {
        msg = (struct queuemsg *) (qbuffer + qhead);
    }
    //
    return msg;
}

void
ctrl_queue_release_msg(struct queuemsg *msg) {
    struct queuemsg *currmsg;
    std::lock_guard<std::mutex> lock(queue_mutex);
    if(qbuffer == NULL) {
        ga_logger(Severity::INFO, "controller queue: buffer released.\n");
        return;
    }
    if(qhead == qtail) {
        // queue is empty
        return;
    }
    currmsg = (struct queuemsg *) (qbuffer + qhead);
    if(msg != currmsg) {
        ga_logger(Severity::WARNING, "controller queue: WARNING - release an incorrect msg?\n");
    }
    qhead += qunit;
    if(qhead == qsize) {
        qhead = 0;
    }
    return;
}

int
ctrl_queue_write_msg(void *msg, int msgsize) {
    int nextpos;
    struct queuemsg *qmsg;
    //
    if((msgsize + sizeof(struct queuemsg)) > qunit) {
        ga_logger(Severity::INFO, "controller queue: msg size exceeded (%d > %d).\n",
            msgsize + sizeof(struct queuemsg), qunit);
        return 0;
    }
    std::lock_guard<std::mutex> lock(queue_mutex);
    if(qbuffer == NULL) {
        ga_logger(Severity::INFO, "controller queue: buffer released.\n");
        return 0;
    }
    //
    nextpos = qtail + qunit;
    if(nextpos == qsize) {
        nextpos = 0;
    }
    //
    if(nextpos == qhead) {
        // queue is full
        msgsize = 0;
    } else {
        qmsg = (struct queuemsg*) (qbuffer + qtail);
        qmsg->msgsize = msgsize;
        if(msgsize > 0 && msgsize <= 2)
            bcopy(msg, qmsg->msg, msgsize);
        qtail = nextpos;
    }
    //
    return msgsize;
}

void
ctrl_queue_clear() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    qhead = qtail = 0;
}

////////////////////////////////////////////////////////////////////

int
ctrl_socket_init(struct RTSPConf *conf) {
    //
    if(conf->ctrlproto == IPPROTO_TCP) {
        ctrlsocket = static_cast<int>(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    } else if(conf->ctrlproto == IPPROTO_UDP) {
        ctrlsocket = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    } else {
        ga_logger(Severity::ERR, "Controller socket-init: not supported protocol.\n");
        return -1;
    }
    if(ctrlsocket < 0) {
        ga_logger(Severity::INFO, "Controller socket-init: %s\n", strerror(errno));
    }
    //
    bzero(&ctrlsin, sizeof(struct sockaddr_in));
    ctrlsin.sin_family = AF_INET;
    ctrlsin.sin_port = htons(conf->ctrlport);
    if(conf->servername != NULL) {
        ctrlsin.sin_addr.s_addr = name_resolve(conf->servername);
        if(ctrlsin.sin_addr.s_addr == INADDR_NONE) {
            ga_logger(Severity::ERR, "Name resolution failed: %s\n", conf->servername);
            return -1;
        }
    }
    ga_logger(Severity::INFO, "controller socket: socket address [%s:%u]\n",
        inet_ntoa(ctrlsin.sin_addr), ntohs(ctrlsin.sin_port));
    //
    return ctrlsocket;
}

////////////////////////////////////////////////////////////////////

int
ctrl_client_init(struct RTSPConf *conf, const char *ctrlid) {
    if(ctrl_socket_init(conf) < 0) {
        conf->ctrlenable = 0;
        return -1;
    }
    if(conf->ctrlproto == IPPROTO_TCP) {
        struct ctrlhandshake hh;
        // connect to the server
        if(connect(ctrlsocket, (struct sockaddr*) &ctrlsin, sizeof(ctrlsin)) < 0) {
            ga_logger(Severity::ERR, "controller client-connect: %s\n", strerror(errno));
            goto error;
        }
        // send handshake
        hh.length = static_cast<unsigned char>(1+strlen(ctrlid)+1);    // msg total len, id, null-terminated
        if(hh.length > sizeof(hh))
            hh.length = sizeof(hh);
        strncpy(hh.id, ctrlid, sizeof(hh.id));
        if(send(ctrlsocket, (char*) &hh, hh.length, 0) <= 0) {
            ga_logger(Severity::ERR, "controller client-send(handshake): %s\n", strerror(errno));
            goto error;
        }
    }
    return 0;
error:
    conf->ctrlenable = 0;
    ctrlenabled = false;
    ga_logger(Severity::ERR, "controller client: controller disabled.\n");
    close(ctrlsocket);
    ctrlsocket = -1;
    return -1;
}

void*
ctrl_client_thread(void *rtspconf) {
    struct RTSPConf *conf = (struct RTSPConf*) rtspconf;
#ifdef ANDROID
    static int drop = 0;
#endif

    if(ctrl_client_init(conf, CTRL_CURRENT_VERSION) < 0) {
        ga_logger(Severity::ERR, "controller client-thread: init failed, thread terminated.\n");
        return NULL;
    }

    ga_logger(Severity::INFO, "controller client-thread started: tid=%ld.\n", ga_gettid());

    while(true) {
        struct queuemsg *qm;
        {
            std::unique_lock<std::mutex> lock(wakeup_mutex);
            wakeup.wait(lock);
        }
        //
        while((qm = ctrl_queue_read_msg()) != NULL) {
            int wlen;
            if(qm->msgsize == 0) {
                ga_logger(Severity::INFO, "controller client: null message received, terminate the thread.\n");
                goto quit;
            }
#ifdef ANDROID
            if(drop > 0)
                continue;
#endif
            if(conf->ctrlproto == IPPROTO_TCP) {
                if((wlen = send(ctrlsocket, (char*) qm->msg, qm->msgsize, 0)) < 0) {
                    ga_logger(Severity::ERR, "controller client-send(tcp): %s\n", strerror(errno));
#ifdef ANDROID
                    drop = 1;
#else
                    exit(-1);
#endif
                }
            } else if(conf->ctrlproto == IPPROTO_UDP) {
                if((wlen = sendto(ctrlsocket, (char*) qm->msg, qm->msgsize, 0, (struct sockaddr*) &ctrlsin, sizeof(ctrlsin))) < 0) {
                    ga_logger(Severity::ERR, "controller client-send(udp): %s\n", strerror(errno));
#ifdef ANDROID
                    drop = 1;
#else
                    exit(-1);
#endif
                }
            }
            ctrl_queue_release_msg(qm);
            //ga_logger(Severity::INFO, "controller client-debug: send msg (%d bytes)\n", wlen);
        }
    }

quit:
    close(ctrlsocket);
    ctrlsocket = -1;
    ga_logger(Severity::INFO, "controller client-thread terminated: tid=%ld.\n", ga_gettid());

    return NULL;
}

void
ctrl_client_sendmsg(void *msg, int msglen) {
    if(ctrlenabled == false) {
        ga_logger(Severity::INFO, "controller client-sendmsg: controller was disabled.\n");
        return;
    }
    if(ctrl_queue_write_msg(msg, msglen) != msglen) {
        ga_logger(Severity::INFO, "controller client-sendmsg: queue full, message dropped.\n");
    } else {
        wakeup.notify_one();
    }
    return;
}

////////////////////////////////////////////////////////////////////

int
ctrl_server_init(struct RTSPConf *conf, const char *ctrlid) {
    if(ctrl_socket_init(conf) < 0)
        return -1;
    myctrlid = strdup(ctrlid);
    // reuse port
    do {
        int val = 1;
        if(setsockopt(ctrlsocket, SOL_SOCKET, SO_REUSEADDR, (char*) &val, sizeof(val)) < 0) {
            ga_logger(Severity::ERR, "controller server-bind: %s\n", strerror(errno));
            goto error;
        }
    } while(0);
    // bind for either TCP of UDP
    if(bind(ctrlsocket, (struct sockaddr*) &ctrlsin, sizeof(ctrlsin)) < 0) {
        ga_logger(Severity::ERR, "controller server-bind: %s\n", strerror(errno));
        goto error;
    }
    // TCP listen
    if(conf->ctrlproto == IPPROTO_TCP) {
        if(listen(ctrlsocket, 16) < 0) {
            ga_logger(Severity::ERR, "controller server-listen: %s\n", strerror(errno));
            goto error;
        }
    }
    return 0;
error:
    close(ctrlsocket);
    ctrlsocket = -1;
    return -1;
}

msgfunc
ctrl_server_setreplay(msgfunc callback) {
    msgfunc old = replay;
    replay = callback;
    return old;
}

void*
ctrl_server_thread(void *rtspconf) {
    struct RTSPConf *conf = (struct RTSPConf*) rtspconf;
    struct sockaddr_in csin, xsin;
    int socket;
#ifdef WIN32
    int csinlen, xsinlen;
#else
    socklen_t csinlen, xsinlen;
#endif
    int clientaccepted = 0;
    //
    const int bufSize = 8192;
    unsigned char buf[bufSize];
    int bufhead, buflen;

    if(ctrl_server_init(conf, CTRL_CURRENT_VERSION) < 0) {
        ga_logger(Severity::ERR, "controller server-thread: init failed, terminated.\n");
        exit(-1);
    }

    ga_logger(Severity::INFO, "controller server started: tid=%ld.\n", ga_gettid());

restart:
    bzero(&csin, sizeof(csin));
    csinlen = sizeof(csin);
    csin.sin_family = AF_INET;
    clientaccepted = 0;
    // handle only one client
    if(conf->ctrlproto == IPPROTO_TCP) {
        struct ctrlhandshake *hh = (struct ctrlhandshake*) buf;
        //
        if((socket = static_cast<int>(accept(ctrlsocket, (struct sockaddr*) &csin, &csinlen))) < 0) {
            ga_logger(Severity::INFO, "controller server-accept: %s.\n", strerror(errno));
            goto restart;
        }
        ga_logger(Severity::INFO, "controller server-thread: accepted TCP client from %s.%d\n",
            inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
        // check initial handshake message
        if((buflen = recv(socket, (char*) buf, sizeof(buf), 0)) <= 0) {
            ga_logger(Severity::INFO, "controller server-thread: %s\n", strerror(errno));
            close(socket);
            goto restart;
        }
        if(hh->length > buflen) {
            ga_logger(Severity::INFO, "controller server-thread: bad handshake length (%d > %d)\n",
                hh->length, buflen);
            close(socket);
            goto restart;
        }
        if(memcmp(myctrlid, hh->id, hh->length-1) != 0) {
            ga_logger(Severity::INFO, "controller server-thread: mismatched protocol version (%s != %s), length = %d\n",
                hh->id, myctrlid, hh->length-1);
            close(socket);
            goto restart;
        }
        ga_logger(Severity::INFO, "controller server-thread: receiving events ...\n");
        clientaccepted = 1;
    }

    buflen = 0;

    while(true) {
        int rlen, msglen;
        //
        bufhead = 0;
        //
        if(conf->ctrlproto == IPPROTO_TCP) {
tcp_readmore:
            if((rlen = recv(socket, (char*) buf+buflen, sizeof(buf)-buflen, 0)) <= 0) {
                ga_logger(Severity::INFO, "controller server-read: %s\n", strerror(errno));
                ga_logger(Severity::INFO, "controller server-thread: conenction closed.\n");
                close(socket);
                goto restart;
            }
#if 0
            ga_logger(Severity::INFO, "controller server-debug: recv msg (%d bytes) | head = %02x %02x\n",
                rlen, buf[0], buf[1]);
#endif
            buflen += rlen;
        } else if(conf->ctrlproto == IPPROTO_UDP) {
            bzero(&xsin, sizeof(xsin));
            xsinlen = sizeof(xsin);
            xsin.sin_family = AF_INET;
            buflen = recvfrom(ctrlsocket, (char*) buf, sizeof(buf), 0, (struct sockaddr*) &xsin, &xsinlen);
            if(clientaccepted == 0) {
                bcopy(&xsin, &csin, sizeof(csin));
                clientaccepted = 1;
            } else if(memcmp(&csin, &xsin, sizeof(csin)) != 0) {
                ga_logger(Severity::INFO, "controller server-thread: NOTICE - UDP client reconnected?\n");
                bcopy(&xsin, &csin, sizeof(csin));
#ifdef WIN32
                ga_module_t *m = encoder_get_vencoder();
                ga_ioctl_buffer_t mb;
                int err = 0;
                if ((err = ga_module_ioctl(m, GA_IOCTL_REQUEST_KEYFRAME, sizeof(mb), &mb)) < 0) {
                    ga_logger(Severity::INFO, "Failed to insert a key frame%s, err=%d\n", m->name, err);
                    exit(-1);
                }

                clientaccepted = 0;
                goto restart;
#endif
                //continue;
            }
        }
tcp_again:
        if(buflen < 2) {
            if(conf->ctrlproto == IPPROTO_TCP)
                goto tcp_readmore;
            else
                continue;
        }
        //
        msglen = ntohs(*((unsigned short*) (buf + bufhead)));
        //
        if(msglen == 0) {
            ga_logger(Severity::WARNING, "controller server: WARNING - invalid message with size equal to zero!\n");
            continue;
        }
        // buffer checks for TCP - not sufficient?
        if(conf->ctrlproto == IPPROTO_TCP) {
            if(buflen < msglen && buflen < (bufSize - bufhead)) {
                bcopy(buf+bufhead, buf, buflen);
                goto tcp_readmore;
            }
        } else if(conf->ctrlproto == IPPROTO_UDP) {
            if(buflen != msglen) {
                ga_logger(Severity::INFO, "controller server: UDP msg size matched (expected %d, got %d).\n",
                    msglen, buflen);
                continue;
            }
        }
        // handle message
        if(ctrlsys_handle_message(buf+bufhead, msglen) != 0) {
            // message has been handled, do nothing
        } else if(replay != NULL) {
            replay(buf+bufhead, msglen);
        } else if(ctrl_queue_write_msg(buf+bufhead, msglen) != msglen) {
            ga_logger(Severity::INFO, "controller server: queue full, message dropped.\n");
        } else {
            wakeup.notify_one();
        }
        // handle buffers for TCP
        if(conf->ctrlproto == IPPROTO_TCP && buflen > msglen) {
            bufhead += msglen;
            buflen -= msglen;
            goto tcp_again;
        }
        buflen = 0;
    }

    clientaccepted = 0;

    goto restart;

    if (myctrlid != NULL) {
        free(myctrlid);
    }
    return NULL;
}

int
ctrl_server_readnext(void *msg, int msglen) {
    int ret;
    struct queuemsg *qm;
again:
    if((qm = ctrl_queue_read_msg()) != NULL) {
        if(qm->msgsize > msglen) {
            ret = -1;
        } else {
            bcopy(qm->msg, msg, qm->msgsize);
            ret = qm->msgsize;
        }
        ctrl_queue_release_msg(qm);
        return ret;
    }
    // nothing available, wait for next input
    {
        std::unique_lock<std::mutex> lock(wakeup_mutex);
        wakeup.wait(lock);
    }
    goto again;
    // never return from here
    return 0;
}

static std::shared_mutex reslock;
static int curr_width = -1;
static int curr_height = -1;
static std::shared_mutex oreslock;
static int output_width = -1;
static int output_height = -1;

void
ctrl_server_set_output_resolution(int width, int height) {
    std::unique_lock<std::shared_mutex> lock(oreslock);
    output_width = width;
    output_height = height;
    return;
}

void
ctrl_server_set_resolution(int width, int height) {
    std::unique_lock<std::shared_mutex> lock(reslock);
    curr_width = width;
    curr_height = height;
    return;
}

void
ctrl_server_get_resolution(int *width, int *height) {
    std::shared_lock<std::shared_mutex> lock(reslock);
    *width = curr_width;
    *height = curr_height;
    return;
}

void
ctrl_server_get_scalefactor(double *fx, double *fy) {
    double rx, ry;
    std::shared_lock<std::shared_mutex> lock(reslock);
    std::shared_lock<std::shared_mutex> olock(oreslock);
    rx = 1.0 * curr_width / output_width;
    ry = 1.0 * curr_height / output_height;
    if(rx <= 0.0)    rx = 1.0;
    if(ry <= 0.0)    ry = 1.0;
    *fx = rx;
    *fy = ry;
    return;
}

