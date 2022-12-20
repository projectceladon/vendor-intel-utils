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

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <drm_fourcc.h>

#ifndef ICR_ALLOC_H
#define ICR_ALLOC_H

#define DRV_MAX_PLANES 4

typedef struct _icr_alloc icr_alloc_t;

typedef struct _icr_buffer {
	int32_t fds[DRV_MAX_PLANES];
	uint32_t strides[DRV_MAX_PLANES];
	uint32_t offsets[DRV_MAX_PLANES];
	uint32_t sizes[DRV_MAX_PLANES];
	uint32_t width;
	uint32_t height;
	uint32_t format;       /* DRM format */
	uint32_t tiling_mode;
	uint32_t pixel_stride;
	uint32_t aligned_width;
	uint32_t aligned_height;
	uint32_t total_size;
	uint32_t gem_handle;
	uint32_t mapped;
	void*    map_address;
	unsigned int reserved[8];
} icr_buffer_t;

typedef icr_buffer_t* icr_buffer_handle_t;

typedef struct _icr_buffer_desc {
    uint32_t width;
	uint32_t height;
	uint32_t format;   /* DRM format */
	uint64_t modifier;
    unsigned int reserved[8];
} icr_buffer_desc_t;


icr_alloc_t*  icr_bufmgr_init();
icr_buffer_t* icr_bufmgr_alloc(icr_alloc_t* alloc, icr_buffer_desc_t* info);
void          icr_bufmgr_free(icr_alloc_t* alloc, icr_buffer_t* buffer);
void          icr_bufmgr_deinit(icr_alloc_t* alloc);

bool		  icr_bufmgr_map(icr_alloc_t* alloc, icr_buffer_t* buffer); //map address in icr_buffer struct
void		  icr_bufmgr_unmap(icr_alloc_t* alloc, icr_buffer_t* buffer);	

//bool          icr_import();
//void          icr_release();

#endif