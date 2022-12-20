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

#ifndef _DISPLAY_SERVER_VHAL_H_
#define _DISPLAY_SERVER_VHAL_H_

#include "display_server.h"
#include "hwc_vhal.h"
#include <thread>

class DisplayServerVHAL : public  DisplayServer {
public:
    DisplayServerVHAL();
    virtual ~DisplayServerVHAL();

    virtual bool init(char *id, encoder_info_t *info) override;
    virtual void deinit() override;

protected:
    void CreateBuffer(vhal::client::cros_gralloc_handle_t handle);
    void RemoveBuffer(vhal::client::cros_gralloc_handle_t handle);

    void DisplayRequest(vhal::client::cros_gralloc_handle_t handle, vhal::client::display_control_t* ctrl);

    void CommandHandler(vhal::client::CommandType cmd, const vhal::client::frame_info_t* frame);

	int PipeMsgHandler();

    std::map<vhal::client::cros_gralloc_handle_t, disp_res_t*> m_dispReses;

    vhal::client::VirtualHwcReceiver *m_vhalReceiver = nullptr;

    int m_sessionId = 0;

    int m_privatePipe = -1;
    bool m_pipeMsgRunning = false;
    std::thread m_pipeMsgThread;

    uint32_t m_curWidth = 0;
    uint32_t m_curHeight = 0;
};

#endif // _DISPLAY_SERVER_VHAL_H_


