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

#ifndef STREAM_UTILS_H
#define STREAM_UTILS_H

#include "IrrStreamer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @param stream_info
 * @return 0 on success
 */
int irr_stream_start(IrrStreamer::IrrStreamInfo *stream_info);

void irr_stream_stop();

/*
 * @Desc force key frame
 */
int irr_stream_force_keyframe(int force_key_frame);

/*
 * @Desc set QP
 */
int irr_stream_set_qp(int qp);

/*
 * @Desc set bitrate
 */
int irr_stream_set_bitrate(int bitrate);

/*
 * @Desc set max bitrate
 */
int irr_stream_set_max_bitrate(int max_bitrate);

/*
 * @Desc set framerate
 */
int irr_stream_set_framerate(float framerate);

/*
* @Desc get encode framerate
*/
int irr_stream_get_framerate(void);

/*
 * @Desc set max frame size
 */
int irr_stream_set_max_frame_size(int size);

/*
 * @Desc set rolling intra refresh
 */
int irr_stream_set_rolling_intra_refresh(int type, int cycle_size, int qp_delta);

/*
 * @Desc set region of interest
 */
#ifdef FFMPEG_v42
int irr_stream_set_region_of_interest(int roi_num, AVRoI roi_para[]);
#endif

/*
 * @Desc set min qp and max qp
 */
int irr_stream_set_min_max_qp(int min_qp, int max_qp);

/*
 *  * @Desc change resolution
 *   */
int irr_stream_change_resolution(int width, int height);

/*
*  * @Desc change codec
*   */
int irr_stream_change_codec(AVCodecID codec_type);

/*
 * @Desc latency start/stop/param setting.
 */
int irr_stream_latency(int latency);

/*
* @Desc get stream width.
*/
int irr_stream_get_width();

/*
* @Desc get stream height.
*/
int irr_stream_get_height();

/*
* @Desc get encoder type id.
*/
int irr_stream_get_encoder_type();

enum IRR_RUNTIME_WRITE_MODE {
    IRR_RT_MODE_INPUT,
    IRR_RT_MODE_OUTPUT,
    IRR_RT_MODE_BOTH,
};

void irr_stream_runtime_writer_start(const enum IRR_RUNTIME_WRITE_MODE mode);
void irr_stream_runtime_writer_stop(const enum IRR_RUNTIME_WRITE_MODE mode);
void irr_stream_runtime_writer_start_with_frame_num(const int frame_num);

int irr_get_VASurfaceFlag();

int irr_get_QSVSurfaceFlag();

void irr_stream_incClient();

void irr_stream_decClient();

int irr_stream_getClientNum();

void irr_stream_setEncodeFlag(bool bAllowEncode);

bool irr_stream_getEncodeFlag();

void irr_stream_setTransmitFlag(bool bAllowTransmit);

bool irr_stream_getTransmitFlag();

void irr_stream_first_start_encdoding(bool bFirstStartEncoding);

/**
 * @Desc set sei type and user id
 */
int irr_stream_set_sei(int sei_type, int sei_user_id);

/**
 * @Desc set gop size
 */
int irr_stream_set_gop_size(int size);

bool irr_stream_getAuthFlag();

void irr_stream_set_screen_capture_flag(bool bAllowCapture);

void irr_sream_set_screen_capture_interval(int captureInterval);

void irr_stream_set_screen_capture_quality(int quality_factor);

void irr_stream_set_iostream_writer_params(const char *input_file, const int width, const int height,
                                           const char *output_file, const int output_frame_number);

void irr_stream_set_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                         int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop);


void irr_stream_set_skipframe(bool bSkipFrame);

/*
* @Desc get skip frame flag.
* @return minus mean call the function fail, 1 mean that flag turn on, 0 mean turn off.
*/
int irr_stream_get_skipframe();

void irr_stream_set_alpha_channel_mode(bool isAlpha);

void irr_stream_set_buffer_size(int width, int height);

int irr_stream_get_encode_new_width();

int irr_stream_get_encode_new_height();

/*
*  * @Desc change the profile and level of the codec
*  * @param [in] iProfile, iLevel
*  * @return 0 on success, minus mean fail or not change at all.
*   */
int irr_stream_change_profile_level(const int iProfile, const int iLevel);
/*
 * @Desc set delay and size from client feedback
 */
int irr_stream_set_client_feedback(uint32_t delay, uint32_t size);



void irr_stream_set_encode_renderfps_flag(bool bRenderFpsEnc);

/*
* @Desc get encode by reder fps flag, minus mean call the function fail, 1 mean that flag turn on, 0 mean turn off.
*/
int irr_stream_get_encode_renderfps_flag(void);


void irr_stream_set_latency_optflag(bool bLatencyOpt);

/*
* @Desc get the encoder latency optimization flag.
* @return minus mean call the function fail, 1 mean that flag turn on, 0 mean turn off.
*/
int irr_stream_get_latency_optflag(void);

#ifdef __cplusplus
}
#endif

#endif /* STREAM_UTILS_H */
