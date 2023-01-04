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

#ifndef ENCODER_H
#define ENCODER_H

#include <va/va.h>
#include <va/va_drmcommon.h>

#include "CTransCoder.h"
#include "IrrStreamer.h"
#include "sock_util.h"
#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENCODER_RESOLUTION_WIDTH_DEFAULT   RESOLUTION_WIDTH_DEFAULT
#define ENCODER_RESOLUTION_WIDTH_MIN       RESOLUTION_WIDTH_MIN
#define ENCODER_RESOLUTION_WIDTH_MAX       RESOLUTION_WIDTH_MAX
#define ENCODER_RESOLUTION_HEIGHT_DEFAULT  RESOLUTION_HEIGHT_DEFAULT
#define ENCODER_RESOLUTION_HEIGHT_MIN      RESOLUTION_HEIGHT_MIN
#define ENCODER_RESOLUTION_HEIGHT_MAX      RESOLUTION_HEIGHT_MAX

extern VADisplay va_dpy;

/**
 * @param encoder_info_t encoder information
 * @desc                check the parameters in encoder_info_t.
 */
int irr_check_options(encoder_info_t *encoder_info);

/**
 * @param encoder_info_t encoder information
 * @desc                check the profile parameters in encoder_info_t.
 */
int irr_check_encode_profile(encoder_info_t *encoder_info);

/**
 * @param encoder_info_t encoder information
 * @desc                check the level parameters in encoder_info_t.
 */
int irr_check_encode_level(encoder_info_t *encoder_info);

/*
 * @param encoder_info_t encoder information
 * @desc                check the rate ctrl parameters in encoder_info_t.
 */
int irr_check_rate_ctrl_options (encoder_info_t *encoder_info);

/*
 * @param encoder_info_t encoder information
 * @desc                check the rolling intra refresh parameters in encoder_info_t.
 */
int irr_check_rir_options(encoder_info_t *encoder_info);

/*
 * @param encoder_info_t encoder information
 * @desc                check the region of interest parameters in encoder_info_t.
 */
int irr_check_roi_options(encoder_info_t *encoder_info);

/**
 * @param encoder_info_t   encoder information
 * @desc                 Initilize encoder and start the encode process, the parameters are passed into by encode service process
 * following is the parameter value example
 * streaming = 1, res = 0x870e00 "720x1280", b = 0x8723f0 "2M", url = 0x870fa0 "irrv:264",
 *   fr = 0x8710f0 "30", codec = 0x0, lowpower = 1
 */
int irr_encoder_start(int id, encoder_info_t *encoder_info);

/**
 *
 * @desc                 Close encoder and it will clear all the related instances and close the irr sockets and other resources.
 */
void irr_encoder_stop();

/**
*
* @param irr_surface_info_t*   surface information
* @desc                        create the irr_surface according to the surface info
*/
irr_surface_t* irr_encoder_create_surface(irr_surface_info_t* surface_info);
irr_surface_t* irr_encoder_create_blank_surface(irr_surface_info_t* surface_info);

/**
*
* @param irr_surface_t*        surface
* @desc                        add the reference count of VASurface
*/
void irr_encoder_ref_surface(irr_surface_t* surface);

/**
*
* @param irr_surface_t*       surface
* @desc                       decrease the reference count of VASurface, if zero reference, destroy the vasurface.
*/
void irr_encoder_unref_surface(irr_surface_t* surface);

/**
*
* @param no
* @desc                        destroy the VADisplay.
*/
void irr_encoder_destroy_display();

/**
*
* @param irr_surface_t*        surface
* @desc                        push the surface to encoding list.
*/
int irr_encoder_write(irr_surface_t* surface);

/**
*
* @param client_rect_right     client(device end) rect right coordinate
* @param client_rect_bottom    client(device end) rect bottom coordinate
* @param fb_rect_right         fb rect right coordinate
* @param fb_rect_bottom        fb rect bottom coordinate
* @param crop_top              crop offset from top border
* @param crop_bottom           crop offset from bottom border
* @param crop_left             crop offset from left border
* @param crop_right            crop offset from right border
* @param valid_crop            crop valid or not
* @desc                        set encoder crop info.
*/
void irr_encoder_write_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                            int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop);


/**
*
* @param AVCodecID             codec_type
* @desc                        change the encoder's codec type
*/
int irr_encoder_change_codec(AVCodecID codec_type);

/**
*
* @param width                 width
* @param height                height
* @desc                        change the encoder's resolution
*/
int irr_encoder_change_resolution(int width,int height);

/**
*
* @param isAlpha               alpha channel mode
* @desc                        set the alpha channel mode
*/
void irr_encoder_set_alpha_channel_mode(bool isAlpha);

/**
*
* @param width                 width
* @param height                height
* @desc                        change the encodeing buffer size
*/
void irr_encoder_set_buffer_size(int width,int height);

/**
*
* @desc                        get the vasurface flag.
*/
int irr_encoder_get_VASurfaceFlag();

/**
*

* @desc                        get the qsvsurface flag.
*/
int irr_encoder_get_QSVSurfaceFlag();

/* @desc                        get the frame rate.
*/
int irr_encoder_get_framerate(void);


/*
* @Desc set encode by render fps flag.
* @param bRenderFpsEnc          true mean that flag turn on, false mean turn off.
*/
void irr_encoder_set_encode_renderfps_flag(bool bRenderFpsEnc);

/*
* @Desc get encode by render fps flag.
* @return minus mean call the function fail, 1 mean that flag turn on, 0 mean turn off.
*/
int irr_encoder_get_encode_renderfps_flag(void);

/*
* @Desc set the skip frame flag.
* @param bSkipFrame, true mean that flag turn on, false mean turn off.
*/
void irr_encoder_set_skipframe(bool bSkipFrame);

/*
* @Desc get skip frame flag.
* @return minus mean call the function fail, 1 mean that flag turn on, 0 mean turn off.
*/
int irr_encoder_get_skipframe(void);

/*
* @Desc set the encoder latency optimization flag.
* @param bLatencyOpt is true mean that flag turn on, false mean turn off.
*/
void irr_encoder_set_latency_optflag(bool bLatencyOpt);

/*
* @Desc get the encoder latency optimization flag.
* @return minus mean call the function fail, 1 mean that flag turn on, 0 mean turn off.
*/
int irr_encoder_get_latency_optflag(void);


#ifdef __cplusplus
}
#endif

#endif /* ENCODER_H */
