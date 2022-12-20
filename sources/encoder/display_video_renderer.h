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

#ifndef _DISPLAY_VIDEO_RENDERER_H_
#define _DISPLAY_VIDEO_RENDERER_H_

#include <vector>
#include <list>
#include <utility>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "sock_util.h"
#include "display_common.h"
#include "display_renderer.h"

class DisplayVideoRenderer : public  DisplayRenderer{
public :
	DisplayVideoRenderer();
	DisplayVideoRenderer(int id): DisplayVideoRenderer() { m_id = id; }
	virtual ~DisplayVideoRenderer();

	virtual bool init(char *name, encoder_info_t *info);
	virtual void deinit();

    virtual disp_res_t* createDispRes(buffer_handle_t handle, int format, int width, int height, int stride);
    virtual void destroyDispRes(disp_res_t* res);

    virtual void drawDispRes(disp_res_t* res, int client_id, int client_count, std::unique_ptr<vhal::client::display_control_t> ctrl);
	virtual void drawBlankRes(int client_id, int client_count);

    virtual void beginFrame();
    virtual void endFrame();
    virtual void retireFrame();
    virtual void flushDelayDelRes();

	virtual void setVideoMode(int mode_info) {}
    virtual int getCropFlag() { return 0; }
    virtual void ChangeResolution(int width, int height);

protected :
	virtual int publishStatusToResourceMonitor(uint32_t id, void * status);

private :
	int m_width;
	int m_height;
	bool m_fpsStats = true;
	int m_statsNumFrames = 0;
	uint64_t m_statsStartTimeInMs = 0;

	uint64_t m_frameIdx;
	std::list<std::pair<uint64_t, disp_res_t*>>	m_deletedReses;

	irr_surface_t*	m_blankSurface;
	irr_surface_t*	m_curSurface;

    encoder_info_t m_currentInfo;
};

#endif // _DISPLAY_VIDEO_RENDERER_H_

