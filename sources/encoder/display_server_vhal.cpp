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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <assert.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#include "display_server_vhal.h"
#include "display_video_renderer.h"
#include "utils/TimeLog.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>

#include <errno.h>
#include <string.h>

#include <linux/input.h>

extern "C" {
int irr_stream_set_client_feedback(uint32_t delay, uint32_t size);
void irr_stream_setEncodeFlag(bool bAllowEncode);
void irr_stream_incClient();
void irr_stream_setTransmitFlag(bool bAllowTransmit);
int irr_stream_force_keyframe(int force_key_frame);
}

using namespace std;
using namespace vhal::client;

DisplayServerVHAL::DisplayServerVHAL():
    DisplayServer()

{
}

DisplayServerVHAL::~DisplayServerVHAL()
{
    m_pipeMsgRunning = false;
    if (m_pipeMsgRunning)
        m_pipeMsgThread.join();
    if (m_privatePipe >= 0)
        close(m_privatePipe);
    if (m_vhalReceiver)
        delete m_vhalReceiver;
}

bool DisplayServerVHAL::init(char *id, encoder_info_t *info)
{
    int session_id = atoi(id);
    int user_id = info->user_id;

    // gpu node id
    const char *dev_dri = getenv("VAAPI_DEVICE");
    if (!dev_dri)
        dev_dri = "/dev/dri/renderD128";
    int gpuid = 0;
    sscanf(dev_dri, "/dev/dri/renderD%d", &gpuid);
    
    string path;
    if (!info->hwc_sock)
    {
        path = "/ipc/hwc-sock";
    }
    else
    {
        path = info->hwc_sock;
        if (path.find("hwc-sock") == string::npos)
        {
            path = "/ipc/hwc-sock";
        }
    }

    path.append(id);

    // get width and height
    int width, height;
    sscanf(info->res, "%dx%d", &width, &height);

    ConfigInfo cfg;
    size_t pos = path.find("/hwc-sock");
    cfg.unix_conn_info.socket_dir = path.substr(0, pos);

    if (getenv("K8S_ENV") == NULL || strcmp(getenv("K8S_ENV"), "true") != 0) {
        // docker env
        cfg.unix_conn_info.android_instance_id = session_id;
    } else {
        // k8s env
        cfg.unix_conn_info.android_instance_id = -1; //dont need id for k8s env
    }
    cfg.video_res_width = width;
    cfg.video_res_height = height;
    cfg.video_device = dev_dri;
    cfg.user_id = user_id;
    try {
        m_vhalReceiver = new VirtualHwcReceiver(cfg, [this](CommandType cmd, const frame_info_t* frame)
            {
                CommandHandler(cmd, frame);
            });
    }
    catch (const std::logic_error& e)
    {
        cerr << "Fatal Error: Failed to create vhalReceiver!" << endl;
        return false;
    }

    m_curWidth = width;
    m_curHeight = height;

    // init renderer
    try {
        m_renderer = new DisplayVideoRenderer(session_id);
    }
    catch (const std::bad_alloc& exp)
    {
        cerr << "Fatal Error: Failed to create display video render" << endl;
        return false;
    }

    if (m_renderer) {
        string surface_name = "display_server";
        surface_name.append(id);
        bool ret = m_renderer->init(const_cast<char *>(surface_name.c_str()), info);
        if (!ret) {
            cerr << "Fatal Error: failed to init display video renderer!" << endl;
            return false;
        }
    }


    if (m_renderer)
    {
        sock_log("%s:%d : drawBlankRes!\n", __func__, __LINE__);
        m_renderer->drawBlankRes(0, 0);
    }

    m_vhalReceiver->start();

    // create private fifo
    stringstream pipeNameStream;
    if (getenv("K8S_ENV") == NULL || strcmp(getenv("K8S_ENV"), "true") != 0) {
        pipeNameStream << path.substr(0, pos) << "/hwc-pipe-" << session_id << "-" << user_id;
    }
    else {
        pipeNameStream << path.substr(0, pos) << "/hwc-pipe-" << user_id;
    }
    string pipeName;
    pipeNameStream >> pipeName;

    cout << "Info, path of the internal pipe:" << pipeName << endl;

    unlink(pipeName.c_str());
    if (mkfifo(pipeName.c_str(), 0666) < 0)
    {
        cerr << "Error: make fifo failed: " << strerror(errno) << endl;
        return false;
    }

    m_privatePipe = open(pipeName.c_str(), O_RDWR|O_NONBLOCK);
    if (m_privatePipe < 0)
    {
        cerr << "Error: open pipe " << pipeName << " failed" << endl;
        return false;
    }

    m_pipeMsgRunning = true;
    m_pipeMsgThread = std::thread(&DisplayServerVHAL::PipeMsgHandler, this);

    return true;
    
}

void DisplayServerVHAL::deinit()
{

    if (m_vhalReceiver)
    {
        delete m_vhalReceiver;
        m_vhalReceiver = nullptr;
    }
    m_pipeMsgRunning = false;
    m_pipeMsgThread.join();

    // Release all pending list
    //list<pair<vhal::client::cros_gralloc_handle_t, disp_res_t*>>::iterator iter;
    for(auto iter = m_dispReses.begin(); iter!= m_dispReses.end();) {
        disp_res_t *dispRes = iter->second;
        m_renderer->destroyDispRes(dispRes);
        m_dispReses.erase(iter++);
    }
    m_renderer->deinit();
    delete m_renderer;

}


void DisplayServerVHAL::CreateBuffer(cros_gralloc_handle_t handle)
{
    // Create DispRes
    disp_res_t* dispRes = m_renderer->createDispRes((::buffer_handle_t)handle, handle->format, handle->width,
                    handle->height, handle->height);

    auto ite = m_dispReses.find(handle);
    if (ite != m_dispReses.end())
    {
        // destory the old one
        disp_res_t *old = ite->second;
        m_renderer->destroyDispRes(old);
        m_dispReses.erase(ite);
    }

    m_dispReses.insert(std::make_pair(handle, dispRes));

    return;

}


void DisplayServerVHAL::RemoveBuffer(cros_gralloc_handle_t handle)
{
    auto ite = m_dispReses.find(handle);
    if (ite == m_dispReses.end())
    {
        return;
    }

    disp_res_t *dispRes = m_dispReses.at(handle);

    if (dispRes)
    {
        m_renderer->destroyDispRes(dispRes);
        m_dispReses.erase(handle);
    }

}

void DisplayServerVHAL::DisplayRequest(cros_gralloc_handle_t handle, vhal::client::display_control_t* ctrl)
{
    bool disable_dynamic_resolution = false;
    if (const char* envStr = std::getenv("DISABLE_DYNAMIC_RESOLUTION"))
    {
        if (strncmp(envStr, "1", strlen("1")) == 0)
            disable_dynamic_resolution = true;
    }

    if ((handle->width != m_curWidth || handle->height != m_curHeight) && !disable_dynamic_resolution)
    {
        m_renderer->ChangeResolution(handle->width, handle->height);
        m_curWidth = handle->width;
        m_curHeight = handle->height;
        irr_stream_incClient();
        irr_stream_setEncodeFlag(true);
        irr_stream_setTransmitFlag(true);
        irr_stream_force_keyframe(1);
    }

    auto ite = m_dispReses.find(handle);

    if (ite == m_dispReses.end())
    {
        cerr << "Fatal Error: remote handle request display before creation!" << endl;
        return;
    }

    disp_res_t *dispRes = ite->second;

    m_renderer->beginFrame();

    std::unique_ptr<vhal::client::display_control_t> uctrl;
    if (ctrl) {
        uctrl = std::unique_ptr<vhal::client::display_control_t>(new vhal::client::display_control_t());
        *uctrl = *ctrl;
    }
    m_renderer->drawDispRes(dispRes, m_sessionId, 1, std::move(uctrl));

    m_renderer->endFrame();

    m_renderer->retireFrame();
}

void DisplayServerVHAL::CommandHandler(CommandType cmd, const vhal::client::frame_info_t* frame)
{
    if (!frame || !frame->handle) {
        cerr << "Invalid frame_into_t parameter from libvhal" << endl;
        return;
    }
    switch(cmd)
    {
        case FRAME_CREATE:
            CreateBuffer(frame->handle);
            break;
        case FRAME_REMOVE:
            RemoveBuffer(frame->handle);
            break;
        case FRAME_DISPLAY:
            DisplayRequest(frame->handle, frame->ctrl);
            break;
        default:
            cerr << "Unknown cmd type received from VhalReceiver" << endl;
            break;
    }
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

int DisplayServerVHAL::PipeMsgHandler()
{
    // create epoll
    int epollfd = epoll_create1(0);
    if (epollfd < 0)
    {
        cerr << "Error to create an epoll fd" << endl;
        return -1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = m_privatePipe;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, m_privatePipe, &ev) < 0)
    {
        cerr << "epoll_ctl add fd " << m_privatePipe  << ":" << strerror(errno) << endl;
        return -1;
    }

    // running
    int kMaxEvents = 10;
    while (m_pipeMsgRunning)
    {
        epoll_event events[kMaxEvents]{};
        int nfds = epoll_wait(epollfd, events, kMaxEvents, 1000);
        if (nfds < 0) {
            nfds = 0;
            if (errno != EINTR) {
                cerr << "Error to epoll wait, " << nfds << endl;
            }
        }

        PipeMessage msg;

        for (int n = 0; n < nfds; ++n)
        {
            ssize_t size = read(events[n].data.fd, &msg, sizeof(msg));
            if ((size != sizeof(msg)) || (msg.magic != 0xdeadbeef))
            {
                continue;
            }
            switch (msg.type)
            {
                case MESSAGE_TYPE_TCAE_FEEDBACK:
                    irr_stream_set_client_feedback(msg.data[0], msg.data[1]); // (delay, size)
                    break;
                case MESSAGE_TYPE_RESOLUTION_CHANGE:
                    m_vhalReceiver->setMode(msg.data[0], msg.data[1]); // (new width, new height)
                    break;
                case MESSAGE_TYPE_SET_VIDEO_ALPHA:
                    m_vhalReceiver->setVideoAlpha(msg.data[0]); // (action)
                    break;
                default:
                    cerr << "Error, type " << msg.type << " is not handled" << endl;
                    break;
            }
        }
    }
    if (epollfd >= 0)
    {
        close(epollfd);
    }
    return 0;
}
