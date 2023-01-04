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

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <va/va.h>
#include <va/va_drmcommon.h>
#include <drm_fourcc.h>
#include <memory>
#include "display-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QP_MAX_VALUE 51
#define QP_DELTA_MAX_VALUE QP_MAX_VALUE
#define QP_DELTA_MIN_VALUE (-QP_MAX_VALUE)

#define MAX_PLANE_NUM 4

/**
 * Type of surface, for exmaple a surface which have a prime id,
 * or user allocate buffers(not DRM_PRIME or KERNEL_PRIME), user can use
 * them for create a VASurface.
 */
enum surface_type {
    BUFFER = 0,      ///<  external buffer
    FD = 1,          ///<  the prime id
    INTERNAL = 2,    ///<  */
    EXTERNAL = 3,    ///<  */
};

typedef enum surface_type surface_type_t;

enum encode_type {
    DATA_BUFFER = 0,      ///<  data buffer
    VASURFACE_ID = 1,     ///<  va surface id
    QSVSURFACE_ID = 2,    ///<  QSV surface id
};

typedef enum encode_type encode_type_t;

enum output_mux_type {
    IRRV_MUX = 0,      ///<  irrv output mux
    LOCAL_MUX = 1,     ///<  local output mux
    DEFAULT = 2,       ///<  default
};

typedef enum output_mux_type output_mux_type_t;

typedef struct _rate_control_info_t {
    const char *bitrate;       ///< Encoder bitrate, default 1M
    const char *qfactor;       ///< Encoder global quality.
    const char *qp;            ///< Encoder constant QP for CQP mode.
    const char *maxrate;       ///< Encoder max bitrate.
    const char *ratectrl;      ///< Encoder rate control mode.
    const char *bufsize;       ///< Encoding rate control buffer size (in bits)

    int qmaxI;                 ///< Encoding Maximum video quantizer scale for I frame.
    int qminI;                 ///< Encoding Minimum video quantizer scale for I frame.
    int qmaxP;                 ///< Encoding Maximum video quantizer scale for P frame.
    int qminP;                 ///< Encoding Maximum video quantizer scale for P frame.
} rate_control_info_t;

typedef struct _irr_ref_info {
    const char *int_ref_type;  ///< Encoder intra refresh type.
    int int_ref_cycle_size;    ///< Number of frames in the intra refresh cycle.
    int int_ref_qp_delta;      ///< QP difference for the refresh MBs.
} irr_ref_info_t;

typedef struct _irr_roi_info {
    bool     roi_enabled; ///< Enable region of interest
    int16_t  x;           ///< x position of ROI region.
    int16_t  y;           ///< y position of ROI region.
    uint16_t width;       ///< width of ROI region.
    uint16_t height;      ///< height of ROI region.
    int8_t   roi_value;   ///< roi_value specifies ROI delta QP or ROI priority.
} irr_roi_info_t;

typedef  struct _irr_surface_info {
    int type;                   ///< surface type : buffer, fd, internal, external, ...
    int format;                 ///< fmt , currently the format get from the vhal graph is RGBA and RGB565
    int width;
    int height;

    int stride[MAX_PLANE_NUM];
    int offset[MAX_PLANE_NUM];
    int fd[MAX_PLANE_NUM];                     ///< This is prime id, for example, it maybe got from vhal via sockets.
    int data_size;

    unsigned char*  pdata;                ///< This is buff for store the data.
    unsigned int    reserved[6];
}irr_surface_info_t;

typedef struct _irr_surface_t {
    irr_surface_info_t info;

    int         ref_count;
    VASurfaceID vaSurfaceID;
    int         encode_type;

    int         flip_image;

    void*       mfxSurf;

    std::unique_ptr<vhal::client::display_control_t> display_ctrl;

    unsigned int reserved[5];
} irr_surface_t;

typedef struct _irr_rate_ctrl_options_info {
    bool need_qp;
    bool need_qfactor;
    bool need_bitrate;
    bool need_maxbitrate;
} irr_rate_ctrl_options_info_t;

typedef struct _irr_encoder_info {
    int nPixfmt;               ///< fmt
    int gop_size;              ///< Group Of Picture size, default 120
    const char *codec;         ///< Encoder codec, e.x. h264_qsv; may be null
    const char *format;        ///< Mux format, e.x. flv; null as auto
    const char *url;           ///< Output url.
    int low_power;             ///< Enable low-power mode, default not.
    const char *res;           ///< Encoding resolution.
    const char *framerate;     ///< Encoding framerate
    const char *exp_vid_param; ///< Extra encoding/muxer parameters passed to libtrans/FFmpeg
    bool streaming;            ///< streaming true/false
    encode_type_t encodeType;
    int encoderInstanceID;     ///< The encoder instance id, start from 0.
    rate_control_info_t rate_contrl_param;
    int quality;               ///< Encoding quality level
    int max_frame_size;        ///< Encoding max frame size.
    irr_ref_info_t ref_info;
    irr_roi_info_t roi_info;
    int slices;                ///< Encoder number of slices, used in parallelized encoding
    int sei;                   ///< Encoding SEI information
    const char *finput;        ///< Local input file in file dump mode
    int vframe;                ///< Frame number of the input file
    const char *foutput;       ///< Local input file in file dump mode
    const char *loglevel;      ///< Log level to enable icr encoder logs by level
    int latency_opt;           ///< Encoding latency optimization, set 1 to enable, 0 to disable
    bool auth;                 ///< Enable socket authentication
    int renderfps_enc;         ///< Encoding by rendering fps, set 1 to enable, 0 to disable, default is 0.
    int minfps_enc;            ///< Min encode fps when renderfps_enc mode is on.
    const char *profile;       ///< Encoding profile
    const char *level;         ///< Encoding profile level
    int filter_nbthreads;      ///< filter threads number
    bool low_delay_brc;        ///< enable TCBRC that trictly obey average frame size set by target bitarte
    bool skip_frame;           ///< enable Skip Frame
    const char *hwc_sock;      ///< user defined socket name for hwc communication
    const char *plugin;        ///< indicate which plugin is used or not, default vaapi-plugin is used
    bool tcaeEnabled;          ///< indicate whether tcae is enabled or not
    const char * tcaeLogPath;  ///< indicate path to generate tcae dumps. If empty not enabled.
    int user_id;               ///< indicate the user id in mulit-user scenario
} encoder_info_t;

#ifdef __cplusplus
}
#endif

#endif /* DATA_TYPES_H */
