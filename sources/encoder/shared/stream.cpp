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

#include <memory>
#include "stream.h"

using IrrStreamInfo = IrrStreamer::IrrStreamInfo;

int irr_stream_start(IrrStreamInfo *stream_info) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;
    if (stream_info->bVASurface) {
        pStreamer->setVASurfaceFlag(true);
    } else if (stream_info->bQSVSurface) {
        pStreamer->setQSVSurfaceFlag(true);
    } else {
        //do nothing
    }
    return pStreamer->start(stream_info);
}

void irr_stream_stop() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (pStreamer)
        pStreamer->stop();
}

int irr_stream_force_keyframe(int force_key_frame) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->force_key_frame(force_key_frame);
}

int irr_stream_set_qp(int qp) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->set_qp(qp);
}

int irr_stream_set_bitrate(int bitrate) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->set_bitrate(bitrate);
}

int irr_stream_set_max_bitrate(int max_bitrate) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->set_max_bitrate(max_bitrate);
}

int irr_stream_set_framerate(float framerate) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->set_framerate(framerate);
}

int irr_stream_get_framerate(void) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->get_framerate();
}

int irr_stream_set_max_frame_size (int size) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -ENAVAIL;

    return pStreamer->set_max_frame_size(size);
}

int irr_stream_set_rolling_intra_refresh(int type, int cycle_size, int qp_delta) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -ENAVAIL;

    return pStreamer->set_rolling_intra_refresh(type, cycle_size, qp_delta);
}
#ifdef FFMPEG_v42
int irr_stream_set_region_of_interest(int roi_num, AVRoI roi_para[]) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -ENAVAIL;

    return pStreamer->set_region_of_interest(roi_num, roi_para);
}
#endif
int irr_stream_set_min_max_qp(int min_qp, int max_qp) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -ENAVAIL;

    return pStreamer->set_min_max_qp(min_qp, max_qp);
}

int irr_stream_change_resolution(int width, int height) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->change_resolution(width, height);
}


int irr_stream_change_codec(AVCodecID codec_type) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->change_codec(codec_type);
}

int irr_stream_latency(int latency) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->setLatency(latency);
}

int irr_stream_get_width() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getWidth();
}

int irr_stream_get_height() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getHeight();
}

int irr_stream_get_encoder_type() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getEncoderType();
}

int irr_get_VASurfaceFlag() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getVASurfaceFlag();
}

int irr_get_QSVSurfaceFlag() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getQSVSurfaceFlag();
}

void irr_stream_incClient() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (pStreamer)
        pStreamer->incClientNum();
}

void irr_stream_decClient() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (pStreamer)
        pStreamer->decClientNum();
}

int irr_stream_getClientNum() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getClientNum();
}

void irr_stream_setEncodeFlag(bool bAllowEncode) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (pStreamer)
        pStreamer->setEncodeFlag(bAllowEncode);
}

bool irr_stream_getEncodeFlag() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return false;

    return pStreamer->getEncodeFlag();
}

void irr_stream_setTransmitFlag(bool bAllowTransmit) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (pStreamer)
        pStreamer->setTransmitFlag(bAllowTransmit);
}

bool irr_stream_getTransmitFlag() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return false;

    return pStreamer->getTransmitFlag();
}


void irr_stream_first_start_encdoding(bool bFirstStartEncoding) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return ;
    pStreamer->setFisrtStartEncoding(bFirstStartEncoding);
}

int irr_stream_set_sei(int sei_type, int sei_user_id) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->set_sei(sei_type, sei_user_id);
}

int irr_stream_set_gop_size(int size) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->set_gop_size(size);
}

bool irr_stream_getAuthFlag() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return false;

    return pStreamer->getAuthFlag();
}

void irr_stream_set_screen_capture_flag(bool bAllowCapture) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->set_screen_capture_flag(bAllowCapture);
}

void irr_sream_set_screen_capture_interval(int captureInterval) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->set_screen_capture_interval(captureInterval);
}

void irr_stream_set_screen_capture_quality(int quality_factor) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->set_screen_capture_quality(quality_factor);
}

void irr_stream_set_iostream_writer_params(const char *input_file, const int width, const int height,
                                           const char *output_file, const int output_frame_number) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->set_iostream_writer_params(input_file, width, height, output_file, output_frame_number);
}

void irr_stream_set_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                         int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->set_crop(client_rect_right, client_rect_bottom, fb_rect_right, fb_rect_bottom, 
                        crop_top, crop_bottom, crop_left, crop_right, valid_crop);
}

void irr_stream_runtime_writer_start_with_frame_num(const int frame_num) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    IORuntimeWriter::Ptr writer = pStreamer->getRunTimeWriter();
    if (!writer)
        return;

    writer->stopWriting(RUNTIME_WRITE_MODE::INOUT);
    writer->startWriting(RUNTIME_WRITE_MODE::INOUT, frame_num);
}

void irr_stream_runtime_writer_start(const enum IRR_RUNTIME_WRITE_MODE mode) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    IORuntimeWriter::Ptr writer = pStreamer->getRunTimeWriter();
    if (!writer)
        return;

    if (mode == IRR_RT_MODE_INPUT) {
        writer->stopWriting(RUNTIME_WRITE_MODE::INPUT);
        writer->startWriting(RUNTIME_WRITE_MODE::INPUT);
    } else if (mode == IRR_RT_MODE_OUTPUT) {
        writer->stopWriting(RUNTIME_WRITE_MODE::OUTPUT);
        writer->startWriting(RUNTIME_WRITE_MODE::OUTPUT);
    } else {
        writer->stopWriting(RUNTIME_WRITE_MODE::INOUT);
        writer->startWriting(RUNTIME_WRITE_MODE::INOUT);
    }
}

void irr_stream_runtime_writer_stop(const enum IRR_RUNTIME_WRITE_MODE mode) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    IORuntimeWriter::Ptr writer = pStreamer->getRunTimeWriter();
    if (!writer)
        return;

    if (mode == IRR_RT_MODE_INPUT) {
        writer->stopWriting(RUNTIME_WRITE_MODE::INPUT);
    } else if (mode == IRR_RT_MODE_OUTPUT) {
        writer->stopWriting(RUNTIME_WRITE_MODE::OUTPUT);
    } else {
        writer->stopWriting(RUNTIME_WRITE_MODE::INOUT);
    }
}


void irr_stream_set_skipframe(bool bSkipFrame) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->setSkipFrameFlag(bSkipFrame);
}

int irr_stream_get_skipframe() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getSkipFrameFlag();
}


void irr_stream_set_alpha_channel_mode(bool isAlpha) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->set_alpha_channel_mode(isAlpha);
}

void irr_stream_set_buffer_size(int width, int height) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->set_buffer_size(width, height);
}

int irr_stream_get_encode_new_width() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getEncodeNewWidth();
}

int irr_stream_get_encode_new_height() {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getEncodeNewHeight();
}

int irr_stream_change_profile_level(const int iProfile, const int iLevel) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->change_profile_level(iProfile, iLevel);
}

int irr_stream_set_client_feedback(uint32_t delay, uint32_t size) {
#ifdef ENABLE_TCAE
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -ENAVAIL;

    return pStreamer->set_client_feedback(delay, size);
#else
    return 0;
#endif
}



void irr_stream_set_encode_renderfps_flag(bool bRenderFpsEnc) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->setRenderFpsEncFlag(bRenderFpsEnc);
}

int irr_stream_get_encode_renderfps_flag(void) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getRenderFpsEncFlag();
}

void irr_stream_set_latency_optflag(bool bLatencyOpt) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return;

    pStreamer->setLatencyOptFlag(bLatencyOpt);
}

int irr_stream_get_latency_optflag(void) {
    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
        return -EINVAL;

    return pStreamer->getLatencyOptFlag();
}

