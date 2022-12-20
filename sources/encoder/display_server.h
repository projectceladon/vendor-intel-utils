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

#ifndef _DISPLAY_SERVER_H_
#define _DISPLAY_SERVER_H_

#include <string.h>
#include <signal.h>

#include "display_renderer.h"

class DisplayServer {
public:
    static DisplayServer *Create(const char *socket);
	DisplayServer();
	virtual ~DisplayServer();

	virtual bool init(char *id, encoder_info_t *info) = 0;
	virtual bool run();
	virtual void deinit() = 0;
    static void signal_handler(int signum);
	virtual int publishStatusToResourceMonitor_sync(uint32_t id, void * status);

protected:
	int                             m_id;
	DisplayRenderer*                m_renderer;
    static int event_flag;
	uint64_t m_statsStartTimeInMs = 0ULL;
};

#endif // _DISPLAY_SERVER_H_
