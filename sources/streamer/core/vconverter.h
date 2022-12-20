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

/**
 * @file
 * video frame converter: header files
 */

#ifndef __VCONVERTER_H__
#define __VCONVERTER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <libswscale/swscale.h>
//#include <libswresample/swresample.h>
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libavutil/base64.h>
#ifdef __cplusplus
}
#endif

/**
 * Structure used to look up an existing converter
 */
struct vconvcfg {
    int src_width;        /**< source video frame width */
    int src_height;        /**< source video frame height */
    AVPixelFormat src_fmt;    /**< source video frame pixel format */
    int dst_width;        /**< destination video frame width */
    int dst_height;        /**< destination video frame height */
    AVPixelFormat dst_fmt;    /**< destination video frame pixel format */
};

EXPORT struct SwsContext * lookup_frame_converter(int srcw, int srch, AVPixelFormat srcfmt, int dstw, int dsth, AVPixelFormat dstfmt);
EXPORT struct SwsContext * create_frame_converter(
        int srcw, int srch, AVPixelFormat srcfmt,
        int dstw, int dsth, AVPixelFormat dstfmt);

#endif
