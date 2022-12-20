// Copyright (C) 2022 Intel Corporation
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

#ifndef __INTEL_CURSOR_H__
#define __INTEL_CURSOR_H__

#include <iostream>
#ifdef WIN32
#include <winsock2.h>
#endif
#include "ga-common.h"
#include "dpipe.h"
#include "rtspconf.h"
#ifdef WIN32
#include "qcscursorcapture.h"
#endif
#define    CURSOR_POOLSIZE    4
#include "ga-msg.h"

#define MAX_CURSOR_WIDTH  64
#define MAX_CURSOR_HEIGHT 64
#define MAX_CURSOR_SIZE (MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT *4)
typedef unsigned long       UL32;
typedef long                L32;


typedef struct {
    L32  left;
    L32  top;
    L32  right;
    L32  bottom;
} Rect;

typedef struct {
    L32  x;
    L32  y;
} Point;

typedef struct _cursorInfo
{
#ifdef WIN32
    BYTE  type; //1: MonoChrome;2 Color 3 Masked Color
    BYTE  isVisible;
    BYTE  waitforvideo;
    BYTE  reserved;
#else
    unsigned int  isVisible;
#endif
    long  pos_x;
    long  pos_y;
    long  hotSpot_x;
    long  hotSpot_y;
    Rect srcRect;
    Rect dstRect;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int lenOfCursor;
} CURSOR_INFO;

typedef struct _cursorpos
{
    long  pos_x;
    long  pos_y;
} CURSOR_POS;


typedef struct _CursorData {
    CURSOR_INFO    cursorInfo;
    unsigned int   cursorDataUpdate;
#ifdef WIN32
    unsigned int   cursorSeqID;  /* track the shape change ID */
#endif
    unsigned char  cursorData[MAX_CURSOR_SIZE];
}CURSOR_DATA;
#ifdef WIN32
EXPORT int start_cursor_client(struct RTSPConf *conf);
EXPORT int set_cursor_pos(CURSOR_POS curpos);
EXPORT int get_cursor_pos(CURSOR_POS *pcurpos);
EXPORT unsigned char *get_cursor_data();
EXPORT int set_cursor_data(unsigned char *pBuffer, unsigned int len);
EXPORT void stop_cursor_service();
EXPORT void restart_cursor_service();
EXPORT int queue_cursor(qcsCursorInfoData ciStruct, unsigned char *pBuffer, int nLen, int waitforvideo);
EXPORT int start_cursor_service(struct RTSPConf *conf);
#endif

#endif

