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

#ifndef _DISPLAY_RENDERER_H_
#define _DISPLAY_RENDERER_H_

#include "sock_util.h"
#include "display_common.h"

#if DEBUG_DISPLAY_DAEMON
#define SOCK_LOG_INIT()   sock_log("%s:%d", __func__, __LINE__);
#define SOCK_LOG(a)       sock_log a;
#else
#define SOCK_LOG_INIT()
#define SOCK_LOG(a)
#endif

class DisplayRenderer {
public :
	virtual ~DisplayRenderer()
	{
		
	}
	virtual bool init(char *name, encoder_info_t *info)=0;
	virtual void deinit()=0;

	virtual disp_res_t* createDispRes(buffer_handle_t handle, int format, int width, int height, int stride)=0;
	virtual void destroyDispRes(disp_res_t* res)=0;

    virtual void drawDispRes(disp_res_t* res, int client_id, int client_count, std::unique_ptr<vhal::client::display_control_t> ctrl)=0;
	virtual void drawBlankRes(int client_id, int client_count)=0;

	virtual void setVideoMode(int mode_info)=0;

    virtual void beginFrame()=0;
    virtual void endFrame()=0;
    virtual void retireFrame()=0;
    virtual void flushDelayDelRes()=0;
    virtual int getCropFlag() = 0;
    virtual void ChangeResolution(int width, int height) = 0;
protected :

    int m_id;
    int m_width;
    int m_height;
    int m_iCrop;
};

#endif // _DISPLAY_RENDERER_H_

