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

#include <string>
#include <assert.h>

#include "display_server.h"
#include "display_server_vhal.h"
#include "display_video_renderer.h"
#include "utils/TimeLog.h"

DisplayServer::DisplayServer()
{
    sock_log("Creating DisplayServer\n");
    m_renderer = nullptr;
    m_id=0;
}

DisplayServer::~DisplayServer()
{
}

#if BUILD_FOR_HOST
#define ICRM_FIFO_PATH   "/tmp/icrm-fifo"
#else
#define ICRM_FIFO_PATH   "/ipc/icrm-fifo"
#endif

int DisplayServer::publishStatusToResourceMonitor_sync(uint32_t id, void * status) {
    int fd, write_bytes = 0;
    unsigned char write_buffer[128];
    char fifo_path[50];
    char *msg;
    const char *dev_dri;
    int data_len, gpuid, len_to_write = 0;
    const int max_msg_len = sizeof(write_buffer) - 4;
    float sync = *(float *)status;
    /* get the gpu id we are on */
    dev_dri = getenv("VAAPI_DEVICE");
    if (!dev_dri)
        dev_dri = "/dev/dri/renderD128";
    sscanf(dev_dri, "/dev/dri/renderD%d", &gpuid);
    if (gpuid > 0 && gpuid <= 256)
    {
        gpuid -= 128;
    }
    else
    {
        return 0;
    }
    snprintf(fifo_path, sizeof(fifo_path), ICRM_FIFO_PATH "-gpu%02d", gpuid);
    fifo_path[sizeof(fifo_path)-1] = '\0';
    if((fd = open(fifo_path, O_WRONLY | O_NONBLOCK)) <= 0) {
        /*
         * This can be a result that no ICR monitor instance is running
         * or it is merely not interested in the local instance info
         * (so that it's not waiting data coming out of the fifo)
         */
        return fd;
    }
    msg = (char *)write_buffer + 4;
    /*
     * Note: We need to make sure data len will not exeed 254,
     *      Otherwise the package len may conflict with
     *      the sync(start of msg) flag
     *
     * 0xff | id | len | msg
     * e.g.
     * 0xff | 1  |  11 | "sync=1.00"
     */
    //start of msg
    write_buffer[0] = 0xff;
    /*
     * id to be transmitted is truncated to 16 bit with MSB followed by LSB
     */
    write_buffer[1] = (unsigned char)((id >> 8) & 0xfful);
    write_buffer[2] = (unsigned char)(id & 0xfful);
    //snprintf will leave at least one byte for string ending character
    snprintf(msg, max_msg_len, "sync=%.2f", sync);
    data_len = strlen(msg) + 1;
    if(data_len + 1 >= 0xff) {
        sock_log("IRR resource monitor frontend: msg lenth too long!!!\n");
        fflush(stdout);
        close(fd);
        return 0;
    }
    write_buffer[3] = strlen(msg) + 1; //including '\0'
    len_to_write = strlen(msg) + 5;
#if 0
    for(int i = 0; i < len_to_write; i++){
        printf("%02x ",(unsigned int)write_buffer[i]);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
#endif
    write_bytes = write(fd, write_buffer, len_to_write);

    close(fd);

    return write_bytes;

}

static inline uint64_t getUs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ll + ts.tv_nsec / 1000;
}

bool DisplayServer::run()
{
    m_statsStartTimeInMs = 0;
    while (true) {
        uint64_t currTimeInMs = getUs() / 1000;
        if (currTimeInMs - m_statsStartTimeInMs >= 1500){
            float sync = 1.00;
            publishStatusToResourceMonitor_sync(m_id, &sync);
            m_statsStartTimeInMs = currTimeInMs;
        }
        if (event_flag != 0){
            SOCK_LOG(("%s:%d : get exit event_flag=%d\n", __func__, __LINE__, event_flag));
            break;
        }
        usleep(16000);
    }
    return true;
}

int DisplayServer::event_flag = 0;

void DisplayServer::signal_handler(int signum) {
    switch (signum){
        case SIGINT://Ctrl+C trigger
            SOCK_LOG(("%s:%d : received SIGINT!\n", __func__, __LINE__));
        case SIGTERM://Ctrl+\ trigger
            SOCK_LOG(("%s:%d : received SIGTERM!\n", __func__, __LINE__));
        case SIGQUIT://kill command will trigger
            SOCK_LOG(("%s:%d : received SIGQUIT!\n", __func__, __LINE__));
        //case SIGKILL://kill -9 command will trigger, but it can not be captured.
            //SOCK_LOG(("%s:%d : received SIGKILL!\n", __func__, __LINE__));
            SOCK_LOG(("%s:%d : set event_flag to 1!\n", __func__, __LINE__));
            event_flag = 1;
            break;
    default:
        SOCK_LOG(("%s:%d : received a signal that needn't handle!\n", __func__, __LINE__));
        break;
    }
}

DisplayServer *DisplayServer::Create(const char *socket)
{
    return new DisplayServerVHAL();
}

