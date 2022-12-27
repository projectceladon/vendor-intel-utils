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
#include <vector>
#include <algorithm>
#include "stream.h"
#include "encoder.h"

// transcode
#include "CTransLog.h"
#include "core/CVAAPIDevice.h"
#include "core/CQSVAPIDevice.h"
#include "core/CTransCoder.h"


#include "irrv/irrv_impl.h"
#include "sock_server.h"

#include "ProfileLevel.h"

using IrrStreamInfo = IrrStreamer::IrrStreamInfo;

VADisplay va_dpy = nullptr;

static std::unique_ptr<CTransLog> e_Log(new CTransLog("Encoder"));

//#define DUMP_RGBA
#ifdef DUMP_RGBA
static FILE * fpraw = NULL;
static const char *filename = "vasurface_map_renderbuffer.rgba";
static int rgba_frames = 600;
static int framecount = 0;
#endif

typedef struct _comparator {
    bool operator()(const std::string &s1, const std::string &s2) const
    {
        return strcasecmp(s1.c_str(), s2.c_str()) < 0;
    }
} nocase_comparator;

const static std::map<std::string, irr_rate_ctrl_options_info_t,
                    nocase_comparator> irr_rate_ctrl_type_map = {
    //         qp        qfactor    bitrate    maxrate
    { "CQP",  {true,     false,     false,     false}},
    { "CBR",  {false,    false,     true,      true}},
    { "VBR",  {false,    false,     true,      true}},
    { "QVBR", {false,    true,      true,      true}},
#ifdef FFMPEG_v42
    { "ICQ",  {false,    true,      false,     false}},
    { "AVBR", {false,    false,     true,      false}},
#endif
};

extern std::map<sock_server_t*, bool> auth_pass;
extern std::map<sock_server_t*, sock_client_proxy_t*> clients;

const static std::vector<std::string> valid_url = {
    "irrv:264",
    "irrv:265",
    "irrv:av1",
    "local:264",
    "local:265",
    "local:av1",
};

#define DEFAULT_URL "irrv:264"

const static std::vector<std::string> valid_plugin = {
    "vaapi",
    "qsv",
};

#define DEFAULT_PLUGIN "vaapi"

#ifdef ENABLE_QSV
mfxSession session;

#ifdef FFMPEG_v42
struct vaapiMemId {
    VASurfaceID* m_surface;
    VAImage      m_image;    // variables for VAAPI Allocator internal color conversion
    unsigned int m_fourcc;
    mfxU8*       m_sys_buffer;
    mfxU8*       m_va_buffer; // buffer info to support surface export
    VABufferInfo m_buffer_info;    // pointer to private export data
    void*        m_custom;
};
#endif
#endif

static void set_iostream_writer(encoder_info_t *info);

static int convert_bitrate(const char *b) {
    int bitrate = 0;
    if (b == NULL)
        return bitrate;

    double d_bitrate = atof(b);
    char c = b[strlen(b) - 1];
    if (c == 'M') {
        d_bitrate = d_bitrate * 1000000;
    } else if ( c == 'K') {
        d_bitrate = d_bitrate * 1000;
    }

    bitrate = d_bitrate;

    return bitrate;
}

int irr_check_options(encoder_info_t *encode_info) {
    int ret = 0, bitrate = 0, average_frame_size = 0;

    if (!encode_info) {
        goto fail;
    }

    // check url
    if (encode_info->url == nullptr || find(valid_url.begin(), valid_url.end(), encode_info->url) == valid_url.end()) {
        encode_info->url = DEFAULT_URL;
        e_Log->Info("%s : %d : use default url: %s\n", __func__, __LINE__, encode_info->url);
    }

    ret = irr_check_rate_ctrl_options(encode_info);
    if (ret < 0)
        goto fail;

    ret = irr_check_rir_options(encode_info);
    if (ret < 0)
        goto fail;

    if (encode_info->profile) {
        ret = irr_check_encode_profile(encode_info);
        if (ret < 0) {
            e_Log->Error("The encoder doesn't support the profile(%s), will use default profile\n", encode_info->profile);
            encode_info->profile = "default";
        }
    }

    if (encode_info->level) {
        ret = irr_check_encode_level(encode_info);
        if (ret < 0) {
            e_Log->Error("The encoder doesn't support the profile level(%s), will use default profile level\n", encode_info->level);
            encode_info->level = "default";
        }
    }

    if (encode_info->roi_info.roi_enabled) {
        ret = irr_check_roi_options(encode_info);
        if (ret < 0) {
            e_Log->Error("%s : %d : ROI options check failed, discard the parameters!\n", __func__, __LINE__);
            encode_info->roi_info.roi_enabled = 0;
        }
    }

    // check frame rate
    if (encode_info->framerate == nullptr || atoi(encode_info->framerate) <= 0) {
        encode_info->framerate = "30";
        e_Log->Info("%s : %d : use default encode frame rate: %s\n", __func__, __LINE__, encode_info->framerate);
    }

    // check max frame size
    if(encode_info->max_frame_size < 0) {
        encode_info->max_frame_size = 0;
        e_Log->Info("%s : %d : max frame size:%d should not be less than zero, will ignore the max frame size setting.\n", __func__, __LINE__, encode_info->max_frame_size);
    }

    if (encode_info->rate_contrl_param.bitrate == nullptr) {
        // max frame size can't be used in non-BRC mode.
        encode_info->max_frame_size = 0;
    } else if (encode_info->max_frame_size > 0 && atof(encode_info->framerate) > 0) {
        // caculate max average frame size
        bitrate = std::max(convert_bitrate(encode_info->rate_contrl_param.bitrate), convert_bitrate(encode_info->rate_contrl_param.maxrate));
        average_frame_size = bitrate / atof(encode_info->framerate) / 8;
        if (encode_info->max_frame_size < average_frame_size) {
            e_Log->Info("%s : %d : max frame size:%d should not be less than average frame size, will use average frame size(%d) as max frame size\n", __func__, __LINE__, encode_info->max_frame_size, average_frame_size);
            encode_info->max_frame_size = average_frame_size;
        }
    }

    // check gop_size
    if (encode_info->gop_size <= 0) {
        encode_info->gop_size = 120;
        e_Log->Info("%s : %d :gop_size should be greater than 0, will use default gop size: %d\n", __func__, __LINE__, encode_info->gop_size);
    }

    // check slices
    if (encode_info->slices < 0) {
        encode_info->slices = 0;
        e_Log->Info("%s : %d :slices should be greater than 0, will ignore the slices setting\n", __func__, __LINE__);
    }

    // check quality level
    if (encode_info->quality <= 0 || encode_info->quality > 7) {
        e_Log->Info("%s : %d :encode_info->quality:%d not in range(1,7), use default quality level 4\n", __func__, __LINE__, encode_info->quality);
        encode_info->quality = 4;
    }

    // check minfps_enc
    if (encode_info->renderfps_enc && encode_info->minfps_enc <= 0) {
        encode_info->minfps_enc = 3;
        e_Log->Info("%s : %d :use default minfps_enc %d\n", __func__, __LINE__, encode_info->minfps_enc);
    }

    // check low_delay_brc
    if (encode_info->low_delay_brc && (0 != strcasecmp(encode_info->rate_contrl_param.ratectrl, "VBR")
                                       && 0 != strcasecmp(encode_info->rate_contrl_param.ratectrl, "QVBR"))) {
        encode_info->low_delay_brc = false;
        e_Log->Info("%s : %d :low_delay_brc should be used in VBR/QVBR\n",__func__, __LINE__);
    }

    // check plugin
    if (encode_info->plugin == nullptr
        || find(valid_plugin.begin(), valid_plugin.end(), encode_info->plugin) == valid_plugin.end()) {
        encode_info->plugin = DEFAULT_PLUGIN;
        e_Log->Info("%s : %d : use default plugin: %s\n", __func__, __LINE__, encode_info->plugin);
    }

    return 0;

fail:
    e_Log->Error("Please check your irr options!\n");
    return AVERROR(EINVAL);
}

int irr_check_encode_profile(encoder_info_t *encode_info) {
    if (!encode_info || !encode_info->profile)
        return 0;

    std::string url(encode_info->url);
    int pos = url.find(":");
    std::string encode_type = url.substr(pos + 1);
    const std::map<std::string, int> *encode_profiles = nullptr;

    if (encode_type == "264") {
        encode_profiles = &vaapi_encode_h264_profiles;
    } else if (encode_type == "265") {
        encode_profiles = &vaapi_encode_h265_profiles;
    } else if (encode_type == "av1") {
        encode_profiles = &vaapi_encode_av1_profiles;
    }

    auto profile = encode_profiles->find(encode_info->profile);
    if (profile == encode_profiles->end())
        return AVERROR(EINVAL);

    return 0;
}

int irr_check_encode_level(encoder_info_t *encode_info) {
    if (!encode_info || !encode_info->level)
        return 0;

    std::string url(encode_info->url);
    int pos = url.find(":");
    std::string encode_type = url.substr(pos + 1);
    const std::map<std::string, int> *encode_levels = nullptr;

    if (encode_type == "264") {
        encode_levels = &vaapi_encode_h264_levels;
    } else if (encode_type == "265") {
        encode_levels = &vaapi_encode_h265_levels;
    } else if (encode_type == "av1") {
        encode_levels = &vaapi_encode_av1_levels;
    }

    auto level = encode_levels->find(encode_info->level);
    if (level == encode_levels->end())
        return AVERROR(EINVAL);

    return 0;
}

int irr_check_rate_ctrl_options (encoder_info_t *encoder_info) {
    rate_control_info_t rate_contrl_param = encoder_info->rate_contrl_param;
    irr_rate_ctrl_options_info_t options;

    if (!encoder_info->rate_contrl_param.ratectrl) {
        e_Log->Error("%s : %d : -ratectrl must be set!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    auto it = irr_rate_ctrl_type_map.find(rate_contrl_param.ratectrl);
    if (it != irr_rate_ctrl_type_map.end()) {
        options = it->second;
    } else {
        e_Log->Error("%s : %d : Unsupported rate control mode: %s!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
        return AVERROR(EINVAL);
    }
// qp
    if (options.need_qp) {
        if (!rate_contrl_param.qp) {
            e_Log->Error("%s : %d : -qp must be set in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
         }

        if ((atoi(rate_contrl_param.qp) <= 0) || (atoi(rate_contrl_param.qp) > QP_MAX_VALUE)) {
            e_Log->Error("%s : %d : -qp should be in range (0,51] for %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
        }
    } else {
        rate_contrl_param.qp = nullptr;
    }
// qfactor
    if (options.need_qfactor) {
         if (!rate_contrl_param.qfactor) {
            e_Log->Error("%s : %d : -qfactor must be set in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
         }

        if (atoi(rate_contrl_param.qfactor) <= 0) {
            e_Log->Error("%s : %d : -qfactor must be larger than 0 in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
        }
    } else {
        rate_contrl_param.qfactor = nullptr;
    }
// bitrate
    if (options.need_bitrate) {
        if (!rate_contrl_param.bitrate) {
            e_Log->Error("%s : %d : -b must be set in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
        }
        if (atoi(rate_contrl_param.bitrate) <= 0) {
            e_Log->Error("%s : %d : -b must be larger than 0 in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
        }
        if (!rate_contrl_param.bufsize || (atoi(rate_contrl_param.bufsize) <= 0))
            rate_contrl_param.bufsize = nullptr;

        if (rate_contrl_param.qmaxI <= 0 || rate_contrl_param.qmaxI > QP_MAX_VALUE) {
            e_Log->Info("%s : %d :setting rate_contrl_param.qmaxI value:%d not in range(1,%d)\n", __func__, __LINE__, rate_contrl_param.qmaxI, QP_MAX_VALUE);
        }
        if (rate_contrl_param.qminI <= 0 || rate_contrl_param.qminI > QP_MAX_VALUE) {
            e_Log->Info("%s : %d :setting rate_contrl_param.qminI value:%d not in range(1,%d)\n", __func__, __LINE__, rate_contrl_param.qminI, QP_MAX_VALUE);
        }

        rate_contrl_param.qmaxI = (rate_contrl_param.qmaxI > QP_MAX_VALUE) ? QP_MAX_VALUE : rate_contrl_param.qmaxI;
        rate_contrl_param.qmaxI = (rate_contrl_param.qmaxI > 0) ? rate_contrl_param.qmaxI : 0;
        rate_contrl_param.qminI = (rate_contrl_param.qminI > 0 && rate_contrl_param.qminI <= QP_MAX_VALUE) ? rate_contrl_param.qminI : 0;

        e_Log->Info("%s : %d :current using rate_contrl_param.qmaxI value:%d, rate_contrl_param.qminI value:%d\n", __func__, __LINE__, rate_contrl_param.qmaxI, rate_contrl_param.qminI);

        if (rate_contrl_param.qmaxP <= 0 || rate_contrl_param.qmaxP > QP_MAX_VALUE) {
            e_Log->Info("%s : %d :setting rate_contrl_param.qmaxP value:%d not in range(1,%d)\n", __func__, __LINE__, rate_contrl_param.qmaxP, QP_MAX_VALUE);
        }
        if (rate_contrl_param.qminP <= 0 || rate_contrl_param.qminP > QP_MAX_VALUE) {
            e_Log->Info("%s : %d :setting rate_contrl_param.qminP value:%d not in range(1,%d)\n", __func__, __LINE__, rate_contrl_param.qminP, QP_MAX_VALUE);
        }

        rate_contrl_param.qmaxP = (rate_contrl_param.qmaxP > QP_MAX_VALUE) ? QP_MAX_VALUE : rate_contrl_param.qmaxP;
        rate_contrl_param.qmaxP = (rate_contrl_param.qmaxP > 0) ? rate_contrl_param.qmaxP : 0;
        rate_contrl_param.qminP = (rate_contrl_param.qminP > 0 && rate_contrl_param.qminP <= QP_MAX_VALUE) ? rate_contrl_param.qminP : 0;

        e_Log->Info("%s : %d :current using rate_contrl_param.qmaxP value:%d, rate_contrl_param.qminP value:%d\n", __func__, __LINE__, rate_contrl_param.qmaxP, rate_contrl_param.qminP);

    } else {
        rate_contrl_param.qmaxI = 0;
        rate_contrl_param.qminI = 0;
        rate_contrl_param.qmaxP = 0;
        rate_contrl_param.qminP = 0;
        rate_contrl_param.bufsize = nullptr;
        rate_contrl_param.bitrate = nullptr;
    }
// maxbitrate
    if (options.need_maxbitrate) {
         if (!rate_contrl_param.maxrate) {
            e_Log->Error("%s : %d : -maxrate must be set in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
         }

        if (atoi(rate_contrl_param.maxrate) <= 0) {
            e_Log->Error("%s : %d : -maxrate must be larger than 0 in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
            return AVERROR(EINVAL);
        }
    } else {
        rate_contrl_param.maxrate = nullptr;
    }
// compare maxbitrate and bitrate
    if (rate_contrl_param.maxrate && rate_contrl_param.bitrate) {
        int _bitrate = convert_bitrate(rate_contrl_param.bitrate);
        int _maxrate = convert_bitrate(rate_contrl_param.maxrate);

        if (!strcasecmp(rate_contrl_param.ratectrl, "CBR")) {
            if (_bitrate != _maxrate) {
                e_Log->Error("%s : %d : bitrate must be equal to maxrate in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
                return AVERROR(EINVAL);
            }
        }

        if (!strcasecmp(rate_contrl_param.ratectrl, "VBR") || !strcasecmp(rate_contrl_param.ratectrl, "QVBR")) {
            if (_bitrate >= _maxrate) {
                e_Log->Error("%s : %d : bitrate must be smaller to maxrate in %s mode!\n", __func__, __LINE__, rate_contrl_param.ratectrl);
                return AVERROR(EINVAL);
            }
        }
    }

    encoder_info->rate_contrl_param = rate_contrl_param;

    return 0;
}

int irr_check_rir_options(encoder_info_t *encoder_info) {
    if (!encoder_info->ref_info.int_ref_type) {
        e_Log->Debug("%s : %d : Do not set rolling intra refresh type!\n", __func__, __LINE__);
        return 0;
    }

    if (!strcmp(encoder_info->ref_info.int_ref_type, "vertical") || !strcmp(encoder_info->ref_info.int_ref_type, "horizontal")) {
        if (encoder_info->ref_info.int_ref_cycle_size <= 0) {
            e_Log->Error("%s : %d : the -rircyclesize must be set and should be greater than 0 in %s RIR mode!\n", __func__, __LINE__, encoder_info->ref_info.int_ref_type);
            return AVERROR(EINVAL);
        }
    } else if (!strcmp(encoder_info->ref_info.int_ref_type, "slice")) {
#ifdef FFMPEG_v42
        if (encoder_info->slices <= 0) {
            e_Log->Error("%s : %d : the -slices must be set and should be greater than 0 in slice RIR mode!\n", __func__, __LINE__);
            return AVERROR(EINVAL);
        }
#else
        e_Log->Error("%s : %d : the slice RIR mode is not supported with this FFmpeg version!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
#endif
    } else {
#ifdef FFMPEG_v42
        e_Log->Error("%s : %d : the RIR type should be 1/2/3 or v/h/s or vertical/horizontal/slice!\n", __func__, __LINE__);
#else
        e_Log->Error("%s : %d : RIR type should be 1/2 or v/h or vertical/horizontal!\n", __func__, __LINE__);
#endif
        return AVERROR(EINVAL);
    }

    if ((encoder_info->ref_info.int_ref_qp_delta < QP_DELTA_MIN_VALUE) || (encoder_info->ref_info.int_ref_qp_delta > QP_DELTA_MAX_VALUE)) {
        e_Log->Error("%s : %d : -rirdeltaqp value should be in range[-51, 51]!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return 0;
}

int irr_check_roi_options(encoder_info_t *encoder_info) {
    int width  = 0;
    int height = 0;

    if (encoder_info->roi_info.x     < 0  || encoder_info->roi_info.y < 0 ||
        encoder_info->roi_info.width <= 0 || encoder_info->roi_info.height <= 0) {
        e_Log->Error("%s : %d : the position parameters of ROI can NOT be negative!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    if (encoder_info->roi_info.roi_value < -51 || encoder_info->roi_info.roi_value > 51) {
        e_Log->Error("%s : %d : the ROI value can NOT be out of range [-51, 51]!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    if (encoder_info->res) {
        sscanf(encoder_info->res, "%dx%d", &width, &height);
        if (encoder_info->roi_info.x + encoder_info->roi_info.width  > width ||
            encoder_info->roi_info.y + encoder_info->roi_info.height > height) {
            e_Log->Error("%s : %d : the ROI region can NOT beyond the resolution(%dx%d)!\n", __func__, __LINE__, width, height);
            return AVERROR(EINVAL);
        }
    }

    return 0;
}

static irr_surface_t* create_surface_from_fd(irr_surface_info_t* surface_info)
{
    VASurfaceAttrib va_attribs[2];
    VASurfaceAttribExternalBuffers va_attrib_extbuf;
    VAStatus status = 0;

    VASurfaceID vaSurf = VA_INVALID_SURFACE;

    int iVASurface = irr_get_VASurfaceFlag();
    int iQSVSurface = irr_get_QSVSurfaceFlag();

    irr_surface_t* surface = (irr_surface_t*)calloc(1, sizeof(irr_surface_t));
    if (!surface) {
        e_Log->Error("%s : %d : create_surface_from_fd failed!\n", __func__, __LINE__);
        return NULL;
    }

    surface->info = *surface_info;
    surface->info.pdata = NULL;
    surface->info.data_size = 0;
    for (int i = 0; i < MAX_PLANE_NUM; i++) {
        if (surface_info->fd[i] != -1) {
            int fd = surface_info->fd[i];
            surface->info.fd[i] = dup(fd);
            e_Log->Debug("%s : %d : dup fd %d -> fd %d\n", __func__, __LINE__, fd, surface->info.fd[i]);
        } else {
            surface->info.fd[i] = -1;
        }
    }

    va_attrib_extbuf.width = surface_info->width;
    va_attrib_extbuf.height = surface_info->height;
    va_attrib_extbuf.data_size = surface_info->height * surface_info->stride[0];
    va_attrib_extbuf.num_planes = 1;
    va_attrib_extbuf.pitches[0] = surface_info->stride[0];
    va_attrib_extbuf.offsets[0] = surface_info->offset[0];
    va_attrib_extbuf.buffers = (unsigned long*)&(surface->info.fd[0]);
    va_attrib_extbuf.num_buffers = 1;
    va_attrib_extbuf.flags = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    va_attrib_extbuf.private_data = NULL;

    switch (surface_info->format) {
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_XBGR8888:
        va_attrib_extbuf.pixel_format = VA_FOURCC_RGBA;
        break;
    case DRM_FORMAT_RGB565:
        va_attrib_extbuf.pixel_format = VA_FOURCC_RGB565;
        break;
    case DRM_FORMAT_NV12:
        va_attrib_extbuf.pixel_format = VA_FOURCC_NV12;
        va_attrib_extbuf.flags |= VA_SURFACE_EXTBUF_DESC_ENABLE_TILING;
        va_attrib_extbuf.num_planes = 2;
        va_attrib_extbuf.pitches[1] = surface_info->stride[1];
        va_attrib_extbuf.offsets[1] = surface_info->offset[1];
        va_attrib_extbuf.data_size = surface_info->offset[1] * 3 / 2;
        break;
    default:
        e_Log->Warn("%s : %d : Cannot support this pixel format(%x)!\n", __func__, __LINE__, surface_info->format);
    }

    va_attribs[0].type = VASurfaceAttribMemoryType;
    va_attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    va_attribs[0].value.type = VAGenericValueTypeInteger;
    va_attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;

    va_attribs[1].type = VASurfaceAttribExternalBufferDescriptor;
    va_attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    va_attribs[1].value.type = VAGenericValueTypePointer;
    va_attribs[1].value.value.p = &va_attrib_extbuf;

    if (va_attrib_extbuf.pixel_format == VA_FOURCC_RGBA) {
        status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_RGB32, surface_info->width, surface_info->height,
          &vaSurf, 1, va_attribs, 2);
    } else if (va_attrib_extbuf.pixel_format == VA_FOURCC_RGB565) {
        status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_RGB16, surface_info->width, surface_info->height,
          &vaSurf, 1, va_attribs, 2);
    } else if (va_attrib_extbuf.pixel_format == VA_FOURCC_NV12) {
        status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, surface_info->width, surface_info->height,
          &vaSurf, 1, va_attribs, 2);
    }
    if (status == VA_STATUS_SUCCESS) {
        surface->ref_count = 1;
        surface->encode_type = VASURFACE_ID;
        e_Log->Debug("%s : %d : prime fd=%d, vaCreateSurface succeed, vaSurfaceID = %d, ref_count = %d, encode = %d\n",
          __func__, __LINE__, surface->info.fd[0], vaSurf, surface->ref_count, surface->encode_type);
    }
    else {
        e_Log->Error("%s : %d : vaCreateSurfaces failed, vaStatus = %d!\n", __func__, __LINE__, status);
        for (int i = 0; i < MAX_PLANE_NUM; i++) {
            if (surface->info.fd[i] != -1) {
                close(surface->info.fd[i]);
            }
        }
        free(surface);
        e_Log->Error("%s : %d : create_surface_from_fd failed!\n", __func__, __LINE__);
        return NULL;
    }
    if (iVASurface) {
        surface->vaSurfaceID = vaSurf;
        surface->mfxSurf = nullptr;
    }
    else if (iQSVSurface)
    {
#ifdef ENABLE_QSV
        mfxFrameSurface1* srf = (mfxFrameSurface1*)calloc(1, sizeof(mfxFrameSurface1));
        if (!srf) {
            free(surface);
            e_Log->Error("%s : %d : create_surface_from_fd mfxFrameSurface1 failed!\n", __func__, __LINE__);
            return NULL;
        }
        // NOTE: that's a HACK. As of now ffmpeg does not standardize what
        // it expects to see in mfxFrameSurface1.Data.MemId field for AV_PIX_FMT_QSV.
        // As a result we just look into what ffmpeg-qsv expects internally and
        // do accordingly. Problem is that this is implementation specific and
        // might change from one ffmpeg version to another.
        //
        // Last time that was changed in n5.0 by this commit:
        //
        // commit a08a5299ac68b1151179c8b0ca1e920ee6c96e2b
        // Author: Artem Galin <artem.galin@intel.com>
        // Date:   Fri Aug 20 22:48:03 2021 +0100
        //
        //     libavutil/hwcontext_qsv: supporting d3d11va device type
        //
        //     This enables usage of non-powered/headless GPU, better HDR support.
        //     Pool of resources is allocated as one texture with array of slices.
        //
        //     Signed-off-by: Artem Galin <artem.galin@intel.com>
        //
#ifdef FFMPEG_v42
        vaapiMemId* memid = (vaapiMemId*)calloc(1, sizeof(vaapiMemId));
#else
        mfxHDLPair* memid = (mfxHDLPair*)calloc(1, sizeof(mfxHDLPair));
#endif
        if (!memid) {
            free(srf);
            free(surface);
            e_Log->Error("%s : %d : create_surface_from_fd vaapi_mid failed!\n", __func__, __LINE__);
            return NULL;
        }

        surface->mfxSurf = srf;
        surface->vaSurfaceID = vaSurf; //Still needed it for remove buffer logic
        surface->encode_type = QSVSURFACE_ID;

#ifdef FFMPEG_v42
        memid->m_surface = (VASurfaceID*)(intptr_t)vaSurf;
        memid->m_fourcc = MFX_FOURCC_RGB4;
#else
        memid->first = &surface->vaSurfaceID;
        memid->second = (mfxMemId)MFX_INFINITE;
#endif

        srf->Data.MemId = memid;
        srf->Info.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        srf->Info.CropX = 0;
        srf->Info.CropY = 0;
        srf->Info.CropW = surface_info->width;
        srf->Info.CropH = surface_info->height;
        srf->Info.Width = (surface_info->width + 15) / 16 * 16;
        srf->Info.Height = (surface_info->height + 15) / 16 * 16;
        srf->Info.FourCC = MFX_FOURCC_RGB4;
        srf->Info.ChromaFormat = MFX_CHROMAFORMAT_YUV444;
#else
        free(surface);
        e_Log->Error("%s : %d : QSV path is not enabled in build, can't enable QSV path in runtime\n", __func__, __LINE__);
        return NULL;
#endif
    }

    return surface;
}

static irr_surface_t* create_surface_from_buf(irr_surface_info_t* surface_info)
{
    irr_surface_t* surface = NULL;
    // TODO: make create surface from buf workable

#if 0
    VAStatus vaStatus;
    VAEntrypoint entrypoints[5];
    int num_entrypoints, slice_entrypoint;
    VAConfigAttrib attrib[2];
    VAConfigID config_id;
    // unused variable
    //VASurfaceID surface_id;
    unsigned char *usrbuf;

    usrbuf = (unsigned char*)surface->pdata;

    VASurfaceAttribExternalBuffers  vaSurfaceExternBuf;
    VASurfaceAttrib attrib_list[VASurfaceAttribCount];

    vaStatus = vaQueryConfigEntrypoints(va_dpy, VAProfileH264ConstrainedBaseline, entrypoints, &num_entrypoints);

    for (slice_entrypoint = 0; slice_entrypoint < num_entrypoints; slice_entrypoint++) {
        if (entrypoints[slice_entrypoint] == VAEntrypointEncSlice)
            break;
    }
    if (slice_entrypoint == num_entrypoints) {
        /* not find Slice entry point */
        e_Log->Error("%s : %d : VAEntrypointEncSlice doesn't support!\n", __func__, __LINE__);
        return vaStatus;
    }

    /* find out the format for the render target, and rate control mode */
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[1].type = VAConfigAttribRateControl;
    vaStatus = vaGetConfigAttributes(va_dpy, VAProfileH264ConstrainedBaseline, VAEntrypointEncSlice, &attrib[0], 2);

    if ((attrib[0].value & VA_RT_FORMAT_RGB32) == 0) {
        /* not find desired RGB32 RT format */
        e_Log->Error("%s : %d : VA_RT_FORMAT_RGB32 doesn't support!\n", __func__, __LINE__);
    }
    if ((attrib[1].value & VA_RC_VBR) == 0) {
        /* Can't find matched RC mode */
        e_Log->Error("%s : %d : VBR mode doesn't support!\n", __func__, __LINE__);
        return vaStatus;
    }

    attrib[0].value = VA_RT_FORMAT_RGB32; /* set to desired RT format */
    //attrib[1].value = VA_RC_VBR; /* set to desired RC mode */

    vaStatus = vaCreateConfig(va_dpy, VAProfileH264ConstrainedBaseline, VAEntrypointEncSlice, &attrib[0], 1, &config_id);

    attrib_list[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
    attrib_list[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
    unsigned int num_attribs = 2;
    vaStatus = vaQuerySurfaceAttributes(va_dpy, config_id, attrib_list, &num_attribs);
    if (vaStatus == VA_STATUS_SUCCESS || vaStatus == VA_STATUS_ERROR_MAX_NUM_EXCEEDED) {
        e_Log->Debug("%s : %d : got the attribs!\n", __func__, __LINE__);
    }
    if (attrib_list[0].flags != VA_SURFACE_ATTRIB_NOT_SUPPORTED) {
        e_Log->Debug("%s : %d : supported memory type:\n", __func__, __LINE__);
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_VA)
            e_Log->Debug("%s : %d : VA_SURFACE_ATTRIB_MEM_TYPE_VA\n", __func__, __LINE__);
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_V4L2)
            e_Log->Debug("%s : %d : VA_SURFACE_ATTRIB_MEM_TYPE_V4L2\n", __func__, __LINE__);
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR)
            e_Log->Debug("%s : %d : VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR\n", __func__, __LINE__);
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM)
            e_Log->Debug("%s : %d : VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM\n", __func__, __LINE__);
        if (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME)
            e_Log->Debug("%s : %d : VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME\n", __func__, __LINE__);
    }

    if ((attrib_list[1].flags != VA_SURFACE_ATTRIB_NOT_SUPPORTED) &&
        (attrib_list[0].value.value.i & VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR)) {
        e_Log->Debug("%s : %d : vaCreateSurfaces from external usr pointer\n", __func__, __LINE__);

        int bpp = 4; // only for RGBA, 32bits/4bytes

        //vaSurfaceExternBuf.buffers = (unsigned long*)malloc(sizeof(unsigned int));
        vaSurfaceExternBuf.buffers = (unsigned long*)surface->pdata;
        vaSurfaceExternBuf.num_buffers = 1;
        vaSurfaceExternBuf.width = surface->width;
        vaSurfaceExternBuf.height = surface->height;
        //vaSurfaceExternBuf.pitches[0] = surface->width * bpp;
        vaSurfaceExternBuf.pitches[0] = vaSurfaceExternBuf.pitches[1] = vaSurfaceExternBuf.pitches[2] = surface->width;
        vaSurfaceExternBuf.pixel_format = VA_FOURCC_BGRX; //VA_FOURCC_BGRX may need to change to VA_FOURCC_XBGR

        attrib_list[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib_list[1].value.type = VAGenericValueTypePointer;
        attrib_list[1].value.value.p = (void *)&vaSurfaceExternBuf;

        attrib_list[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib_list[0].value.type = VAGenericValueTypeInteger;
        attrib_list[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_USER_PTR;

        vaStatus = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_RGB32, surface->width, surface->height, &surface->vaSurfaceID, 1, attrib_list, 2);
        vaStatus = vaDestroyConfig(va_dpy, config_id);
    }

    return vaStatus;
#endif

    return surface;
}

static output_mux_type_t get_output_mux_type(const char* param_url) {
    if(strncmp(param_url, "irrv", strlen("irrv")) == 0)
        return IRRV_MUX;

    if (strncmp(param_url, "local", strlen("local")) == 0)
        return LOCAL_MUX;

    return DEFAULT;
}

class StaticSockServer
{
public:
    StaticSockServer() {}
    ~StaticSockServer()
    {
        if (m_sock)
        {
            irrv_close(m_sock);
            m_sock = nullptr;
        }
    }
    bool Init(int port)
    {
        m_sock = sock_server_init(SOCK_CONN_TYPE_INET_SOCK, SOCK_UTIL_DEFAULT_PATH, NULL, port);
        return (m_sock != nullptr);
    }
    inline sock_server_t *Get() { return m_sock; }

private:
    sock_server_t *m_sock = nullptr;
};

int irr_encoder_start(int id, encoder_info_t *encoder_info) {
    e_Log->Info("%s: +\n", __func__);
    int ret = -1;
    char *render_port = NULL;
    static StaticSockServer irrv_server;
    static StaticSockServer irrv_auxiliary_server;

    int max_render_port = INT32_MAX - 1000 - encoder_info->encoderInstanceID;

    if (encoder_info->loglevel)
        CTransLog::SetLogLevel(encoder_info->loglevel);
    else
        CTransLog::SetLogLevel(CTransLog::LL_INFO);

    //streaming resolution not set, using default 576x960
    int hw_lcd_width  = RESOLUTION_WIDTH_DEFAULT;
    int hw_lcd_height = RESOLUTION_HEIGHT_DEFAULT;
    if (encoder_info->res) {
        e_Log->Info("%s : %d : rendering and streaming with resolution %s.\n", __func__, __LINE__, encoder_info->res);
        sscanf(encoder_info->res, "%dx%d", &hw_lcd_width, &hw_lcd_height);
        if( hw_lcd_width  < RESOLUTION_WIDTH_MIN  || hw_lcd_width  > RESOLUTION_WIDTH_MAX ||
            hw_lcd_height < RESOLUTION_HEIGHT_MIN || hw_lcd_height > RESOLUTION_HEIGHT_MAX) {
            e_Log->Error("%s : %d : streaming resolution can NOT be set as %dx%d.\n", __func__, __LINE__, hw_lcd_width, hw_lcd_height);
            e_Log->Error("%s : %d : resolution constraints: width %d-%d, height %d-%d.\n", __func__, __LINE__,
                         RESOLUTION_WIDTH_MIN, RESOLUTION_WIDTH_MAX, RESOLUTION_HEIGHT_MIN, RESOLUTION_HEIGHT_MAX);
            e_Log->Error("%s : %d : using default %dx%d\n", __func__, __LINE__, RESOLUTION_WIDTH_DEFAULT, RESOLUTION_HEIGHT_DEFAULT);
            hw_lcd_width  = RESOLUTION_WIDTH_DEFAULT;
            hw_lcd_height = RESOLUTION_HEIGHT_DEFAULT;
        }
    }

#ifndef USE_QUICK
    IrrStreamer::Register(id, hw_lcd_width, hw_lcd_height, 30.f);
#else
    e_Log->Debug("%s : %d : enable quick\n", __func__, __LINE__);
    IrrStreamer::Register(id, hw_lcd_width, hw_lcd_height, 0.f);
#endif

    render_port = getenv("render_server_port");

    if (encoder_info->streaming){
        IrrStreamer::IrrStreamInfo info = { 0 };
        if (encoder_info->framerate)
            info.framerate = encoder_info->framerate;

        info.codec = encoder_info->codec;
        info.pix_format = encoder_info->nPixfmt;

        switch (get_output_mux_type(encoder_info->url)) {
        case IRRV_MUX:
            render_port = getenv("render_server_port");
            if (render_port == NULL || atoi(render_port) > max_render_port) {
                e_Log->Error("%s : %d : fail to create irrv server for %s! Please check your render server port setting in env!\n", __func__, __LINE__, encoder_info->url);
                e_Log->Info("%s: ret=%d: -\n", __func__, AVERROR(EINVAL));
                return AVERROR(EINVAL);
            } else {
                if (irrv_server.Get() == NULL)
                {
                    bool res = irrv_server.Init(atoi(render_port) + 1000 + encoder_info->encoderInstanceID);
                    if (!res) {
                        e_Log->Error("%s : %d : failed to create irrv server for %s!\n", __func__, __LINE__, encoder_info->url);
                        e_Log->Info("%s: ret=%d: -\n", __func__, AVERROR(EINVAL));
                        return AVERROR(EINVAL);
                    }
                    e_Log->Info("%s : %d : created irrv server successfully for %s!\n", __func__, __LINE__, encoder_info->url);
                    auth_pass[irrv_server.Get()]           = false;
                    clients[irrv_server.Get()]             = NULL;
                }

                const char * auxiliary_server = getenv("auxiliary_server");
                if(auxiliary_server && 0 == strncmp(auxiliary_server, "1", strlen("1"))) {
                    // If the environment variables auxiliary_server is set to "1",
                    // an auxiliary socket server will be created using port "render_port_value + 2000".
                    // When the primary socket have no ability to change parameters at runtime,
                    // the auxiliary server can be used for another connection to deal with dynamic settings.
                    // test_irrv_client is a typicallyl application which can use auxiliary server.
                    if (irrv_auxiliary_server.Get() == NULL)
                    {
                        int aux_port = atoi(render_port) + 2000 + encoder_info->encoderInstanceID;
                        bool res = irrv_auxiliary_server.Init(aux_port);
                        if (!res) {
                            e_Log->Error("%s : %d : failed to create irrv auxiliary server, port: %d\n", __func__, __LINE__, aux_port);
                            e_Log->Info("%s: ret=%d: -\n", __func__, AVERROR(EINVAL));
                            return AVERROR(EINVAL);
                        }
                        e_Log->Info("auxiliary_server is enabled, port = %d\n", aux_port);
                        auth_pass[irrv_auxiliary_server.Get()] = false;
                        clients[irrv_auxiliary_server.Get()]   = NULL;
                    }
                } else {
                    e_Log->Info("auxiliary_server is disabled\n");
                }

                info.cb_params.opaque   = irrv_server.Get();
                info.cb_params.opaque2  = irrv_auxiliary_server.Get();
                info.cb_params.cbWrite  = irrv_writeback;
                info.cb_params.cbWrite2 = irrv_writeback2;
                info.cb_params.cbClose  = irrv_close;
                info.cb_params.cbCheckNewConn = irrv_checknewconn;
                info.cb_params.cbSendMessage = irrv_send_message;

                info.url = encoder_info->url;
            }
        case LOCAL_MUX:
            info.url = encoder_info->url;
            e_Log->Debug("%s : %d : local output mux enable for %s!\n", __func__, __LINE__, encoder_info->url);
            break;
        default:
            e_Log->Error("%s : %d : fail to find a suit output mux type for %s!\n", __func__, __LINE__, encoder_info->url);
            e_Log->Info("%s: ret=%d: -\n", __func__, AVERROR(EINVAL));
            return AVERROR(EINVAL);
        }

        if (encoder_info->low_power) {
            e_Log->Info("%s : %d : low power enabled.\n", __func__, __LINE__);
            info.low_power = 1;
        }
        info.exp_vid_param = encoder_info->exp_vid_param;

        // if (encoder_info->encodeType == VASURFACE_ID) {
        //     info.bVASurface = true;
        //     info.bQSVSurface = false;
        // }

        if (encoder_info->rate_contrl_param.ratectrl)
            info.rc_params = encoder_info->rate_contrl_param;

        if (encoder_info->max_frame_size)
            info.max_frame_size = encoder_info->max_frame_size;

        if (encoder_info->ref_info.int_ref_type)
            info.ref_info = encoder_info->ref_info;

        if (encoder_info->roi_info.roi_enabled)
            info.roi_info = encoder_info->roi_info;

        if (encoder_info->slices)
            info.slices = encoder_info->slices;

        info.quality = (encoder_info->quality > 0) ? encoder_info->quality : 4;
        if (info.quality <= 0 || info.quality > 7)
            info.quality = 4;
        e_Log->Info("%s : %d : quality level = %d.\n", __func__, __LINE__, info.quality);

        if (encoder_info->auth)
            info.auth = encoder_info->auth;

        if (encoder_info->sei <= 0) {
            e_Log->Info("%s : %d : SEI value encoder_info->sei:%d not set to be greater than 0.\n", __func__, __LINE__, encoder_info->sei);
        }

        info.sei = (encoder_info->sei > 0) ? encoder_info->sei : 0;
        if (info.sei) {
            if (info.sei & 1)
                e_Log->Info("%s : %d : SEI picture timing info enabled.\n", __func__, __LINE__);
            if (info.sei & 2)
                e_Log->Info("%s : %d : SEI encoder version identifier info enabled.\n", __func__, __LINE__);
            if (info.sei & 4)
                e_Log->Info("%s : %d : SEI recovery points info enabled.\n", __func__, __LINE__);
            if (info.sei & 8)
                e_Log->Info("%s : %d : SEI host timestamp info enabled.\n", __func__, __LINE__);
        }

        if (encoder_info->latency_opt == 0) {
            sock_log("Encoding latency optimization disabled.\n");
            info.latency_opt = 0;
        }
        else {
            sock_log("Encoding latency optimization enabled.\n");
            info.latency_opt = 1;
        }

        if (encoder_info->renderfps_enc == 0) {
            sock_log("Encoding by rendering fps disabled\n");
            info.renderfps_enc = 0;
        }
        else {
            sock_log("Encoding by rendering fps enabled.\n");
            info.renderfps_enc = 1;
        }

        info.minfps_enc = encoder_info->minfps_enc;

        info.gop_size = (encoder_info->gop_size > 0) ? encoder_info->gop_size : 120;

        if (encoder_info->profile)
            info.profile = encoder_info->profile;

        if (encoder_info->level)
            info.level = encoder_info->level;

        info.filter_nbthreads = encoder_info->filter_nbthreads;
        info.res = encoder_info->res;
        info.low_delay_brc    = encoder_info->low_delay_brc;

        info.skip_frame = encoder_info->skip_frame;

        info.plugin           = encoder_info->plugin;
        info.tcaeEnabled      = encoder_info->tcaeEnabled;
        info.tcaeLogPath      = encoder_info->tcaeLogPath;

        if (encoder_info->encodeType == VASURFACE_ID) {
            if (strncmp(info.plugin, "qsv", strlen("qsv")) == 0) {
                info.bVASurface = false;
                info.bQSVSurface = true;
                encoder_info->encodeType = QSVSURFACE_ID;
            } else if (strncmp(info.plugin, "vaapi", strlen("vaapi")) == 0) {
                info.bVASurface = true;
                info.bQSVSurface = false;
            } else {
                info.bVASurface = true;
                info.bQSVSurface = false;
                e_Log->Info("%s : %d : incorrect plugin setting, using vaapi by default \n", __func__, __LINE__);
            }
        }

//         if (!va_dpy) {
//             if (info.bQSVSurface && !info.bVASurface) {
// #ifdef ENABLE_QSV
//                 AVBufferRef *pHwDev = CQSVAPIDevice::getInstance()->getQSVapiDev();
//                 if (NULL != pHwDev) {
//                     AVHWDeviceContext *device_ctx = (AVHWDeviceContext*)pHwDev->data;
//                     AVQSVDeviceContext *hwctx = (AVQSVDeviceContext*)device_ctx->hwctx;
//                     mfxSession session = hwctx->session;
//                     mfxHDL handle;
//                     mfxStatus sts = MFXVideoCORE_GetHandle(session, MFX_HANDLE_VA_DISPLAY, &handle);
//                     e_Log->Debug("%s : %d : MFXVideoCORE_GetHandle return = %d\n", __func__, __LINE__, sts);
//                     va_dpy = (VADisplay)handle;
//                 }
// #else
//                 e_Log->Error("%s : %d : QSV path is not enabled in build, can't enable QSV path in runtime\n", __func__, __LINE__);
//                 return -1;
// #endif
//             } else {
//                 AVBufferRef *pHwDev = CVAAPIDevice::getInstance()->getVaapiDev();
//                 if (NULL != pHwDev) {
//                     AVHWDeviceContext *device_ctx = (AVHWDeviceContext*)pHwDev->data;
//                     AVVAAPIDeviceContext *hwctx = (AVVAAPIDeviceContext*)device_ctx->hwctx;
//                     va_dpy = hwctx->display;
//                 }
//             }
//         }

        set_iostream_writer(encoder_info);

        ret = irr_stream_start(&info);
    }
    e_Log->Info("%s: ret=%d: -\n", __func__, ret);
    return ret;
}

void irr_encoder_stop() {
    e_Log->Info("%s: +\n", __func__);
    IrrStreamer::Unregister();
    e_Log->Info("%s: -\n", __func__);
}


irr_surface_t* irr_encoder_create_surface(irr_surface_info_t* surface_info)
{
    e_Log->Debug("%s : %d : surface_info = %p\n", __func__, __LINE__, surface_info);

    irr_surface_t* surface = NULL;
    if (!va_dpy) {
        int iVASurface = irr_get_VASurfaceFlag();
        int iQSVSurface = irr_get_QSVSurfaceFlag();
        if (iVASurface)
        {
            AVBufferRef *pHwDev = CVAAPIDevice::getInstance()->getVaapiDev();
            if (NULL != pHwDev) {
                AVHWDeviceContext *device_ctx = (AVHWDeviceContext*)pHwDev->data;
                AVVAAPIDeviceContext *hwctx = (AVVAAPIDeviceContext*)device_ctx->hwctx;
                va_dpy = hwctx->display;
            }
        }
#ifdef ENABLE_QSV
        else if (iQSVSurface)
        {
            AVBufferRef *pHwDev = CQSVAPIDevice::getInstance()->getQSVapiDev();
            if (NULL != pHwDev) {
                AVHWDeviceContext *device_ctx = (AVHWDeviceContext*)pHwDev->data;
                AVQSVDeviceContext *hwctx = (AVQSVDeviceContext*)device_ctx->hwctx;
                mfxSession session = hwctx->session;
                mfxHDL handle;
                mfxStatus sts = MFXVideoCORE_GetHandle(session, MFX_HANDLE_VA_DISPLAY, &handle);
                va_dpy = (VADisplay)handle;
            }
        }
#endif
    }

    //if got a prime id, create a VASurface from the prime id
    if (surface_info->type == FD) {
        surface = create_surface_from_fd(surface_info);
    }
    else if (surface_info->type == BUFFER)
    {
        surface = create_surface_from_buf(surface_info);    // Only for testing now. but it seem fail by VA_STATUS_ERROR_UNSUPPORTED_RT_FORMA
    }

    if(surface) {
        e_Log->Debug("%s : %d : create surface succeed, surface = %p, prime fd = %d, vaSurfaceID = %x, ref_count = %d, encode = %d\n",
            __func__, __LINE__, surface, surface->info.fd[0], surface->vaSurfaceID, surface->ref_count, surface->encode_type);
    }
    else {
        e_Log->Error("%s : %d : create surface failed!\n", __func__, __LINE__);
    }

    return surface;
}

irr_surface_t* irr_encoder_create_blank_surface(irr_surface_info_t* surface_info)
{
    e_Log->Debug("%s : %d : surface_info = %p\n", __func__, __LINE__, surface_info);

    int iQSVSurface = irr_get_QSVSurfaceFlag();

    irr_surface_t* surface = (irr_surface_t*)calloc(1, sizeof(irr_surface_t));
    if (!surface) {
        e_Log->Error("%s : %d : create blank surface failed!\n", __func__, __LINE__);
        return NULL;
    }


    surface->info = *surface_info;
    surface->info.pdata = NULL;
    surface->info.data_size = 0;

    surface->vaSurfaceID = VA_INVALID_SURFACE;
    surface->ref_count   = 1;
    surface->encode_type = iQSVSurface ? QSVSURFACE_ID : VASURFACE_ID;

    return surface;
}

void irr_encoder_destroy_surface(irr_surface_t* surface)
{
    e_Log->Debug("%s : %d : surface = %p\n", __func__, __LINE__, surface);

    if(surface) {
        if(surface->info.type == FD) {
            if(surface->info.pdata) {
                free(surface->info.pdata);
            }

            if(surface->vaSurfaceID != VA_INVALID_SURFACE) {
                e_Log->Debug("%s : %d : vaDestroySurface, prime fd=%d, vaSurfaceID=%d\n", __func__, __LINE__, surface->info.fd[0], surface->vaSurfaceID);
                vaDestroySurfaces(va_dpy, &(surface->vaSurfaceID), 1);
            }

            for (int i = 0; i < MAX_PLANE_NUM; i++) {
                if(surface->info.fd[i] != -1) {
                    e_Log->Debug("%s : %d : close fd %d\n", __func__, __LINE__, surface->info.fd[i]);
                    close(surface->info.fd[i]);
                    surface->info.fd[i] = -1;
                }
            }
#ifdef ENABLE_QSV
            if(surface->mfxSurf) {
                e_Log->Debug("free mfxSurface data\n");
                mfxFrameSurface1* srf = (mfxFrameSurface1*)surface->mfxSurf;
                if (srf->Data.MemId) {
                    free(srf->Data.MemId);
                }
                free(srf);
            }
#endif
        }
        free(surface);
    }
}

void irr_encoder_ref_surface(irr_surface_t* surface)
{
    e_Log->Debug("%s : %d : surface = %p\n", __func__, __LINE__, surface);

    if(surface) {
        surface->ref_count++;
        e_Log->Debug("%s : %d : prime fd=%d, vaSurfaceID=%d, ref_count=%d\n", __func__, __LINE__, surface->info.fd[0], surface->vaSurfaceID, surface->ref_count);
    }
}

void irr_encoder_unref_surface(irr_surface_t* surface)
{
    e_Log->Debug("%s : %d : surface = %p\n", __func__, __LINE__, surface);

    if(surface) {
        surface->ref_count--;
        e_Log->Debug("%s : %d : prime fd=%d, vaSurfaceID=%d, ref_count=%d\n", __func__, __LINE__, surface->info.fd[0], surface->vaSurfaceID, surface->ref_count);
        if(0 == surface->ref_count) {
            e_Log->Debug("%s : %d : begin to destory surface\n", __func__, __LINE__);
            irr_encoder_destroy_surface(surface);
        }
    }
}

int irr_encoder_write(irr_surface_t* surface) {
    e_Log->Debug("%s : %d : surface = %p\n", __func__, __LINE__, surface);

    IrrStreamer* pStreamer = IrrStreamer::get();
    if (!pStreamer)
         return -1;

    int ret = 0;
    int iVASurface = irr_get_VASurfaceFlag();
    int iQSVSurface = irr_get_QSVSurfaceFlag();
    if (iVASurface == 1)  {
        surface->encode_type = VASURFACE_ID;
    }
    else if (iQSVSurface == 1) {
        surface->encode_type = QSVSURFACE_ID;
    }
    else if (iVASurface == 0 && iQSVSurface == 0) {
        surface->encode_type = DATA_BUFFER;
    }
    else {
        e_Log->Error("%s : %d : fail to get VASurfaceFlag!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    // if get the surface buffer , pass it to encode directy.
    if (surface->encode_type == DATA_BUFFER) {
        if (surface->info.type == FD) {//type == FD, this mean the buffer data is not ready, so we need to get buffer from the fd.
            VAStatus vaStatus;
            if (!va_dpy) {
                AVBufferRef *pHwDev = CVAAPIDevice::getInstance()->getVaapiDev();
                if (NULL != pHwDev) {
                    AVHWDeviceContext *device_ctx = (AVHWDeviceContext*)pHwDev->data;
                    AVVAAPIDeviceContext *hwctx = (AVVAAPIDeviceContext*)device_ctx->hwctx;
                    va_dpy = hwctx->display;
                }
            }

            VAImage surface_image;
            void *surface_p = NULL;
            vaStatus = vaDeriveImage(va_dpy, surface->vaSurfaceID, &surface_image);
            if (vaStatus != VA_STATUS_SUCCESS) {
                e_Log->Error("%s : %d : fail to vaDeriveImage from surface->vaSurfaceID=%d, vaStatus = %d!\n", __func__, __LINE__, surface->vaSurfaceID, vaStatus);
                return -1;
            }
            vaStatus = vaMapBuffer(va_dpy, surface_image.buf, &surface_p);
            if (vaStatus != VA_STATUS_SUCCESS) {
                e_Log->Error("%s : %d : fail to vaMapBuffer from surface->vaSurfaceID=%d, vaStatus = %d!\n", __func__, __LINE__, surface->vaSurfaceID, vaStatus);
                return -1;
            }
            //surface->pdata = surface_p + surface_image.offsets[0];// This also can work, but the buffer really still in GPU.
            if(!surface->info.pdata) {
                surface->info.data_size = surface->info.stride[0] * surface->info.height;
                surface->info.pdata = (unsigned char*)malloc(surface->info.data_size * sizeof(unsigned char));
            }
            if((surface->info.pdata) && (surface_p)) {
                memcpy(surface->info.pdata, (char *)surface_p + surface_image.offsets[0], surface->info.data_size);
            }

#ifdef DUMP_RGBA
            e_Log->Info("%s : %d : success  to vaMapBuffer from surface_id=%d, vaStatus = %d!\n", __func__, __LINE__, surface->vaSurfaceID, vaStatus);

            int writeSize;
            if ((fpraw == NULL) && (framecount < rgba_frames)) {
                fpraw = fopen(filename, "wb+");
                if (NULL == fpraw)
                {
                    e_Log->Error("%s : %d : open file %s failed!\n", __func__, __LINE__, filename);
                }
                e_Log->Info("%s : %d : open file %s succesfully!\n", __func__, __LINE__, filename);
            }

            if (fpraw != NULL) {
                writeSize = fwrite(surface->info.pdata, sizeof(char), surface->info.data_size, fpraw);
                e_Log->Info("%s : %d : fwrite frame(%d) succesfully, writeSize =%d!\n", __func__, __LINE__, framecount, writeSize);
            }

            if (framecount > rgba_frames) {
                if (NULL != fpraw)
                {
                    fclose(fpraw);
                    fpraw = NULL;
                    e_Log->Info("%s : %d : close file %s succesfully!\n", __func__, __LINE__, filename);
                }
            }
            framecount++;
#endif

            vaStatus = vaUnmapBuffer(va_dpy, surface_image.buf);
            if (vaStatus != VA_STATUS_SUCCESS) {
                e_Log->Error("%s : %d : fail to vaUnmapBuffer from surface->vaSurfaceID=%d, vaStatus = %d!\n", __func__, __LINE__, surface->vaSurfaceID, vaStatus);
                return -1;
            }
            vaStatus = vaDestroyImage(va_dpy, surface_image.image_id);
            if (vaStatus != VA_STATUS_SUCCESS) {
                e_Log->Error("%s : %d : fail to vaDestroyImage from surface->vaSurfaceID=%d, vaStatus = %d!\n", __func__, __LINE__, surface->vaSurfaceID, vaStatus);
                return -1;
            }
        }
    }
    return pStreamer->write(surface);
}

static void set_iostream_writer(encoder_info_t* info)
{
    // output file must be specified
    if (!info->foutput) {
        return;
    }

    e_Log->Info("%s : %d : %s!\n", __func__, __LINE__, info->foutput);

    int width  = 0;
    int height = 0;
    if (info->res) {
        sscanf(info->res, "%dx%d", &width, &height);
    }

    irr_stream_set_iostream_writer_params(info->finput, width, height, info->foutput, info->vframe);
}

void irr_encoder_write_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                            int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop) {
    irr_stream_set_crop(client_rect_right, client_rect_bottom, fb_rect_right, fb_rect_bottom, 
                        crop_top, crop_bottom, crop_left, crop_right, valid_crop);
}


int irr_encoder_change_codec(AVCodecID codec_type) {
    int ret = 0;
    ret = irr_stream_change_codec(codec_type);
    e_Log->Info("%s : %d : codec_type=%d!\n", __func__, __LINE__, codec_type);
    return ret;
}

int irr_encoder_change_resolution(int width,int height){
    return irr_stream_change_resolution(width, height);
}

void irr_encoder_set_alpha_channel_mode(bool isAlpha) {
    irr_stream_set_alpha_channel_mode(isAlpha);
}

void irr_encoder_set_buffer_size(int width, int height) {
    irr_stream_set_buffer_size(width, height);
}

int irr_encoder_get_VASurfaceFlag() {
    int ret = 0;
    ret = irr_get_VASurfaceFlag();
    return ret;
}


int irr_encoder_get_QSVSurfaceFlag() {
    int ret = 0;
    ret = irr_get_QSVSurfaceFlag();
    return ret;
}

int irr_encoder_get_framerate(void) {
    return irr_stream_get_framerate();
}

void irr_encoder_set_encode_renderfps_flag(bool bRenderFpsEnc) {
    irr_stream_set_encode_renderfps_flag(bRenderFpsEnc);
}

int irr_encoder_get_encode_renderfps_flag(void) {
    int ret = 0;
    ret = irr_stream_get_encode_renderfps_flag();
    return ret;
}

void irr_encoder_set_skipframe(bool bSkipFrame) {
    irr_stream_set_skipframe(bSkipFrame);
}

int irr_encoder_get_skipframe(void) {
    int ret = 0;
    ret = irr_stream_get_skipframe();
    return ret;
}


void irr_encoder_set_latency_optflag(bool bLatencyOpt) {
    irr_stream_set_latency_optflag(bLatencyOpt);
}

int irr_encoder_get_latency_optflag(void) {
    int ret = 0;
    ret = irr_stream_get_latency_optflag();
    return ret;
}
