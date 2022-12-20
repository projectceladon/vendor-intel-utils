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

#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <thread>
#include <mutex>
#include <map>
#include <string.h>
#include <vector>

#include "encoder.h"
#include "encoder_comm.h"
#include "sock_util.h"
#include "cli.hpp"

const static char *ENV_VAAPI_DEVICE   = "VAAPI_DEVICE";
const static char *ENV_ICR_NODE = "ICR_RNODE";
const static char *ENV_RENDER_PORT    = "render_server_port";
const static char *ENV_ENCODE_UNLIMIT = "encode_unlimit";
const static char *ENV_AUX_SERVER     = "auxiliary_server";

typedef struct _comparator {
    bool operator()(const std::string &s1, const std::string &s2) const
    {
        return strcasecmp(s1.c_str(), s2.c_str()) < 0;
    }
} nocase_comparator;

const static std::map<std::string, std::string> rir_type_map = {
    { "1", "vertical" },
    { "v", "vertical" },
    { "V", "vertical" },
    { "2", "horizontal" },
    { "h", "horizontal" },
    { "H", "horizontal" },
    { "3", "slice" },
    { "s", "slice" },
    { "S", "slice" },
};

const static std::map<std::string, std::string> default_profile = {
    { "264", "high" },
    { "265", "main" },
};

const static std::map<std::string, std::string> default_level = {
    { "264", "4.2" },
    { "265", "4"   },
};

// The default rate control configurations dict table:
// Every KEY is mapping to 3 levels based on resolution
const static std::map<std::string, std::vector<rate_control_info_t>,
                    nocase_comparator> default_rc_configs = {
    { "CQP",  { {            NULL,              NULL, .qp = "26",            NULL,   NULL, 0, },
                {            NULL,              NULL, .qp = "26",            NULL,   NULL, 0, },
                {            NULL,              NULL, .qp = "26",            NULL,   NULL, 0, }, } },
    { "CBR",  { { .bitrate = "3.3M",            NULL,       NULL, .maxrate = "3.3M", NULL, 0, },
                { .bitrate = "5M",              NULL,       NULL, .maxrate = "5M",   NULL, 0, },
                { .bitrate = "8M",              NULL,       NULL, .maxrate = "8M",   NULL, 0, } } },
    { "VBR",  { { .bitrate = "3.3M",            NULL,       NULL, .maxrate = "6.6M", NULL, 0, },
                { .bitrate = "5M",              NULL,       NULL, .maxrate = "10M",  NULL, 0, },
                { .bitrate = "8M",              NULL,       NULL, .maxrate = "16M",  NULL, 0, } } },
    { "QVBR", { { .bitrate = "3.3M", .qfactor = "20",       NULL, .maxrate = "6.6M", NULL, 0, },
                { .bitrate = "5M",   .qfactor = "20",       NULL, .maxrate = "10M",  NULL, 0, },
                { .bitrate = "8M",   .qfactor = "20",       NULL, .maxrate = "16M",  NULL, 0, } } },
    { "AVBR", { { .bitrate = "3.3M",            NULL,       NULL,            NULL,   NULL, 0, },
                { .bitrate = "5M",              NULL,       NULL,            NULL,   NULL, 0, },
                { .bitrate = "8M",              NULL,       NULL,            NULL,   NULL, 0, } } },
    { "ICQ",  { {            NULL,   .qfactor = "20",       NULL,            NULL,   NULL, 0, },
                {            NULL,   .qfactor = "20",       NULL,            NULL,   NULL, 0, },
                {            NULL,   .qfactor = "20",       NULL,            NULL,   NULL, 0, } } },
};

const static std::vector<std::string> valid_url = {
    "irrv:264",
    "irrv:265",
    "local:264",
    "local:265",
};

void icr_parse_args(int argc, char *argv[], encoder_info_t &info) {
    // parse the command parameters
    static const struct option long_options[] = {
        { "streaming",      no_argument,        0,  'a' },
        { "res",            required_argument,  0,  'b' },
        { "url",            required_argument,  0,  'c' },
        { "fr",             required_argument,  0,  'd' },
        { "codec",          required_argument,  0,  'e' },
        { "b",              required_argument,  0,  'f' },
        { "lowpower",       no_argument,        0,  'g' },
        { "buffer",         no_argument,        0,  'h' }, // "buffer" means that the encoded flow operating with rgba buffer
                                                           // in cpu at the begining not buffer of vasurface in GPU
        { "qp",             required_argument,  0,  'i' },
        { "qfactor",        required_argument,  0,  'j' },
        { "maxrate",        required_argument,  0,  'k' },
        { "ratectrl",       required_argument,  0,  'l' },
        { "mfs",            required_argument,  0,  'm' },
        { "rirtype",        required_argument,  0,  'n' },
        { "rircyclesize",   required_argument,  0,  'o' },
        { "rirdeltaqp",     required_argument,  0,  'p' },
        { "slices",         required_argument,  0,  'q' },
        { "qmaxI",          required_argument,  0,  'r' },
        { "qminI",          required_argument,  0,  's' },
        { "qmaxP",          required_argument,  0,  't' },
        { "qminP",          required_argument,  0,  'u' },
        { "quality",        required_argument,  0,  'v' },
        { "bufsize",        required_argument,  0,  'w' },
        { "sei",            required_argument,  0,  'x' },
        { "finput",         required_argument,  0,  'y' },
        { "vframe",         required_argument,  0,  'z' },
        { "foutput",        required_argument,  0,  'A' },
        { "loglevel",       required_argument,  0,  'B' }, // valid loglevel arguments: "debug" "verbose" "info" "warn" "error"
        { "latency_opt",    required_argument,  0,  'C' }, // -latency_opt 0 to disable latency optimization , -latency_opt 1 to enable.
        { "g",              required_argument,  0,  'D' }, // Group of Picture size
        { "auth",           no_argument,        0,  'E' }, // enable socket authentication
        { "renderfps_enc",  required_argument,  0,  'F' }, // Try to encode by render fps
        { "minfps_enc",     required_argument,  0,  'G' }, // Min encode fps when encode by render fps mode is used.
        { "roi_enabled",    required_argument,  0,  'H' }, // enable region of interest
                                                           // Support one region in command parameters. Multiple regions can be set at run time.
        { "roi_x",          required_argument,  0,  'I' }, // x pos of roi region
        { "roi_y",          required_argument,  0,  'J' }, // y pos of roi region
        { "roi_w",          required_argument,  0,  'K' }, // width of roi region
        { "roi_h",          required_argument,  0,  'L' }, // height of roi region
        { "roi_value",      required_argument,  0,  'M' }, // roi value
        { "profile",        required_argument,  0,  'N' }, // encode profile
        { "level",          required_argument,  0,  'O' }, // encode level
        { "filter_threads", required_argument,  0,  'P' }, // filter threads
        { "h",              no_argument,        0,  'Q' }, // print usage
        { "low_delay_brc",  no_argument,        0,  'R' }, // enable TCBRC that strictly obey average frame size set by target bitrate
        { "skipframe",      no_argument,        0,  'S' }, // enable Skip Frame
        { "hwc_sock",       required_argument,  0,  'T' }, // user defined socket name for hwc communication
        { "plugin",         required_argument,  0,  'U' }, // indicate vaapi or qsv is used
        { "tcae",           required_argument,  0,  'V' }, // enable tcae
        { "user",           required_argument,  0,  'W' }, // user id for multi-user in one android session
        { "tcae_log_path",  required_argument,  0,  'X' }, // enable tcae
        { 0, 0, 0, 0 }
    };

    int c, index;
    const char *optstring = "";
    while ((c = getopt_long_only(argc, argv, optstring, long_options, &index)) != -1) {
        switch (c) {
        case 'a':
            info.streaming = 1;
            break;
        case 'b':
            info.res = optarg;
            break;
        case 'c':
            info.url = optarg;
            break;
        case 'd':
            info.framerate = optarg;
            break;
        case 'e':
            info.codec = optarg;
            break;
        case 'f':
            info.rate_contrl_param.bitrate = optarg;
            break;
        case 'g':
            info.low_power = 1;
            break;
        case 'h':
            info.encodeType = DATA_BUFFER;
            break;
        case 'i':
            info.rate_contrl_param.qp = optarg;
            break;
        case 'j':
            info.rate_contrl_param.qfactor = optarg;
            break;
        case 'k':
            info.rate_contrl_param.maxrate = optarg;
            break;
        case 'l':
            info.rate_contrl_param.ratectrl = optarg;
            break;
        case 'm':
            info.max_frame_size = atoi(optarg);
            break;
        case 'n':
            info.ref_info.int_ref_type = optarg;
            break;
        case 'o':
            info.ref_info.int_ref_cycle_size = atoi(optarg);
            break;
        case 'p':
            info.ref_info.int_ref_qp_delta = atoi(optarg);
            break;
        case 'q':
            info.slices = atoi(optarg);
            break;
        case 'r':
            info.rate_contrl_param.qmaxI = atoi(optarg);
            break;
        case 's':
            info.rate_contrl_param.qminI = atoi(optarg);
            break;
        case 't':
            info.rate_contrl_param.qmaxP = atoi(optarg);
            break;
        case 'u':
            info.rate_contrl_param.qminP = atoi(optarg);
            break;
        case 'v':
            info.quality = atoi(optarg);
            break;
        case 'w':
            info.rate_contrl_param.bufsize = optarg;
            break;
        case 'x':
            info.sei = atoi(optarg);
            break;
        case 'y':
            info.finput = optarg;
            break;
        case 'z':
            info.vframe = atoi(optarg);
            break;
        case 'A':
            info.foutput = optarg;
            break;
        case 'B':
            info.loglevel = optarg;
            break;
        case 'C':
            info.latency_opt = atoi(optarg);
            break;
        case 'D':
            info.gop_size = atoi(optarg);
            break;
        case 'E':
            info.auth = true;
            break;
        case 'F':
            info.renderfps_enc = atoi(optarg);
            break;
        case 'G':
            info.minfps_enc = atoi(optarg);
            break;
        case 'H':
            info.roi_info.roi_enabled = atoi(optarg);
            break;
        case 'I':
            info.roi_info.x = atoi(optarg);
            break;
        case 'J':
            info.roi_info.y = atoi(optarg);
            break;
        case 'K':
            info.roi_info.width = atoi(optarg);
            break;
        case 'L':
            info.roi_info.height = atoi(optarg);
            break;
        case 'M':
            info.roi_info.roi_value = atoi(optarg);
            break;
        case 'N':
            info.profile = optarg;
            break;
        case 'O':
            info.level = optarg;
            break;
        case 'P':
            info.filter_nbthreads = atoi(optarg);
            break;
        case 'Q':
            icr_print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case 'R':
            info.low_delay_brc   = true;
            break;
        case 'S':
            info.skip_frame = true;
            break;
        case 'T':
            info.hwc_sock = optarg;
            break;
        case 'U':
            info.plugin = optarg;
            break;
        case 'V':
            info.tcaeEnabled = !!atoi(optarg);
            break;
        case 'W':
            info.user_id = atoi(optarg);
            break;
        case 'X':
            info.tcaeLogPath = optarg;
            break;
        default:
            break;
        }
    }
}

int bitrate_ctrl_sanity_check(encoder_info_t *info) {
    if (!info || !info->res || !info->rate_contrl_param.ratectrl) {
        sock_log("%s : %d : resolution or rate control related parameters error!\n", __func__, __LINE__);
        return -1;
    }

    // 0 means checking PASS
    if (0 == irr_check_rate_ctrl_options(info))
        return 0;

    sock_log("%s : %d : use default birate ctrl values!\n", __func__, __LINE__);

    int width  = 720, height = 1280;
    sscanf(info->res, "%dx%d", &width, &height);
    if (width  <= 0 || width > ENCODER_RESOLUTION_WIDTH_MAX ||
        height <= 0 || height > ENCODER_RESOLUTION_WIDTH_MAX) {
        sock_log("%s : %d : Invalid resolution:%s!\n", __func__, __LINE__, info->res);
        return -1;
    }
    int framesize = width * height;
    int level = framesize <= ICR_ENCODER_BITRATE_LOW ? 0 :
                (framesize > ICR_ENCODER_BITRATE_LOW && framesize < ICR_ENCODER_BITRATE_HIGH ? 1 : 2);

    auto it = default_rc_configs.find(info->rate_contrl_param.ratectrl);
    if (it != default_rc_configs.end()) {
        auto ratectrl = info->rate_contrl_param.ratectrl;
        info->rate_contrl_param = it->second[level];
        info->rate_contrl_param.ratectrl = ratectrl;
        return 0;
    }

    sock_log("%s : %d : birate rate ctrl mode[%s] not found!\n", __func__, __LINE__, info->rate_contrl_param.ratectrl);
    return -1;
}

void rir_type_sanity_check(encoder_info_t *info) {
    if (info->ref_info.int_ref_type) {
        auto it = rir_type_map.find(info->ref_info.int_ref_type);
        if (it != rir_type_map.end()) {
            info->ref_info.int_ref_type = it->second.c_str();
        }
    }
}

#ifndef BUILD_FOR_HOST
void encoder_properties_sanity_check(encoder_info_t *info) {
    ICRCommProp comm_prop;

    int rnode           = 0;
    int encodeUnlimit   = 0;
    int auxiliaryServer = 0;
    int streaming       = 0;
    int auth            = 0;
    int len             = 0;
    int ret             = 0;
    int para_cnt        = 0;
    int low_delay_brc   = 0;
    int skipFrame       = 0;

    const int_prop int_prop_table[] = {
        //  property name                     parameter                            default
        {ICR_ENCODER_RNODE,                   &rnode,                                 0      },
        {ICR_ENCODER_ENCODE_UNLIMIT_ENABLE,   &encodeUnlimit,                         0      },
        {ICR_ENCODER_AUXILIARY_SERVER_ENABLE, &auxiliaryServer,                       1      },
        {ICR_ENCODER_STREAMING,               &streaming,                             1      },
        {ICR_ENCODER_LOW_POWER,               &info->low_power,                       1      },
        {ICR_ENCODER_MAX_FRAME_SIZE,          &info->max_frame_size,              UNSET_PROP },
        {ICR_ENCODER_RIR_CYCLE_SIZE,          &info->ref_info.int_ref_cycle_size, UNSET_PROP },
        {ICR_ENCODER_RIR_DELTA_QP,            &info->ref_info.int_ref_qp_delta,   UNSET_PROP },
        {ICR_ENCODER_SLICES,                  &info->slices,                      UNSET_PROP },
        {ICR_ENCODER_QMAX_I,                  &info->rate_contrl_param.qmaxI,     UNSET_PROP },
        {ICR_ENCODER_QMIN_I,                  &info->rate_contrl_param.qminI,     UNSET_PROP },
        {ICR_ENCODER_QMAX_P,                  &info->rate_contrl_param.qmaxP,     UNSET_PROP },
        {ICR_ENCODER_QMIN_P,                  &info->rate_contrl_param.qminP,     UNSET_PROP },
        {ICR_ENCODER_QUALITY,                 &info->quality,                     UNSET_PROP },
        {ICR_ENCODER_SEI,                     &info->sei,                         UNSET_PROP },
        {ICR_ENCODER_VFRAME,                  &info->vframe,                      UNSET_PROP },
        {ICR_ENCODER_LATENCY_OPTION,          &info->latency_opt,                      1     },
        {ICR_ENCODER_GOP_SIZE,                &info->gop_size,                        120    },
        {ICR_ENCODER_AUTH,                    &auth,                                   0     },
        {ICR_ENCODER_RENDERFPS_ENC,           &info->renderfps_enc,                    0     },
        {ICR_ENCODER_MINFPS_ENC,              &info->minfps_enc,                       3     },
        {ICR_ENCODER_ROI_ENABLE,              (int*)&info->roi_info.roi_enabled,       0     },
        {ICR_ENCODER_ROI_X,                   (int*)&info->roi_info.x,            UNSET_PROP },
        {ICR_ENCODER_ROI_Y,                   (int*)&info->roi_info.y,            UNSET_PROP },
        {ICR_ENCODER_ROI_WIDTH,               (int*)&info->roi_info.width,        UNSET_PROP },
        {ICR_ENCODER_ROI_HEIGHT,              (int*)&info->roi_info.height,       UNSET_PROP },
        {ICR_ENCODER_ROI_VALUE,               (int*)&info->roi_info.roi_value,    UNSET_PROP },
        {ICR_ENCODER_FILTER_THREADS,          (int*)&info->filter_nbthreads,	       1     },
        {ICR_ENCODER_LOW_DELAY_BRC,           &low_delay_brc,                          0     },
        { ICR_ENCODER_SKIP_FRAME,             &skipFrame,                              0     },
    };

    const char_prop char_prop_table[] = {
        // property name         parameter                          default
        {ICR_ENCODER_RES,       &info->res,                        "720x1280"},
        {ICR_ENCODER_RC_MODE,   &info->rate_contrl_param.ratectrl,   "CBR"   },
        {ICR_ENCODER_FPS,       &info->framerate,                     "30"   },
        {ICR_ENCODER_URL,       &info->url,                        "irrv:264"},
        {ICR_ENCODER_CODEC,     &info->codec,                         NULL   },
        {ICR_ENCODER_BIT_RATE,  &info->rate_contrl_param.bitrate,     NULL   },
        {ICR_ENCODER_QP,        &info->rate_contrl_param.qp,          NULL   },
        {ICR_ENCODER_QFACTOR,   &info->rate_contrl_param.qfactor,     NULL   },
        {ICR_ENCODER_MAXRATE,   &info->rate_contrl_param.maxrate,     NULL   },
        {ICR_ENCODER_QFACTOR,   &info->rate_contrl_param.qfactor,     NULL   },
        {ICR_ENCODER_RIR_TYPE,  &info->ref_info.int_ref_type,         NULL   },
        {ICR_ENCODER_BUF_SIZE,  &info->rate_contrl_param.bufsize,     NULL   },
        {ICR_ENCODER_FINPUT,    &info->finput,                        NULL   },
        {ICR_ENCODER_FOUTPUT,   &info->foutput,                       NULL   },
        {ICR_ENCODER_LOG_LEVEL, &info->loglevel,                      NULL   },
        {ICR_ENCODER_PROFILE,   &info->profile,                       NULL   },
        {ICR_ENCODER_LEVEL,     &info->level,                         NULL   },
    };

    for(auto prop:int_prop_table) {
        int value = 0;
        len = comm_prop.getSystemPropInt(prop.name, &value);

        if(len > 0) {
            *prop.pvalue = value;
        } else {
            if(UNSET_PROP != prop.default_value) {
                *prop.pvalue = prop.default_value;
                comm_prop.setSystemProp(prop.name, *prop.pvalue);
            }
        }
    }

    // save these properites
    static char prop_value_res[PARA_MAX][PROP_VALUE_MAX] = {0};
    for(auto prop:char_prop_table) {
        len = comm_prop.getSystemProp(prop.name, prop_value_res[para_cnt], PROP_VALUE_MAX);

        if(len > 0 && para_cnt < PARA_MAX) {
            *prop.ppvalue = prop_value_res[para_cnt++];
        } else {
            if(NULL != prop.default_value) {
                *prop.ppvalue = prop.default_value;
                comm_prop.setSystemProp(prop.name, *prop.ppvalue);
            }
        }
    }

    if (info->url == nullptr || find(valid_url.begin(), valid_url.end(), info->url) == valid_url.end())
        info->url = "irrv:264";

    std::string s_url(info->url), delim = ":";
    std::string codec_type = s_url.substr(s_url.find(delim) + 1, s_url.length());
    // set default profile/level
    if (!info->profile) {
        auto it = default_profile.find(codec_type);
        if (it != default_profile.end()) {
            info->profile = it->second.c_str();
            comm_prop.setSystemProp(ICR_ENCODER_PROFILE, info->profile);
        }
    }

    if (!info->level) {
        auto it = default_level.find(codec_type);
        if (it != default_level.end()) {
            info->level = it->second.c_str();
            comm_prop.setSystemProp(ICR_ENCODER_LEVEL, info->level);
        }
    }

    std::string pre   = "/dev/dri/renderD";
    std::string index = std::to_string(VAAPI_RENDER_BASE + rnode);
    std::string icr_node=std::to_string(rnode);
    std::string vaapi_device = pre + index;

    ret = setenv(ENV_VAAPI_DEVICE, vaapi_device.c_str(), 1);
    ret += setenv(ENV_ICR_NODE,icr_node.c_str(),0);
    ret += setenv(ENV_RENDER_PORT, "23432", 1);

    if (encodeUnlimit) {
        ret += setenv(ENV_ENCODE_UNLIMIT, "1", 1);
    }

    if (auxiliaryServer) {
        ret += setenv(ENV_AUX_SERVER, "1", 1);
    }

    if (ret) {
        sock_log("set environment variables error, please check environment variables\n");
    }

    if (streaming) {
        info->streaming = true;
    }

    if (auth) {
        info->auth = true;
    }

    if (low_delay_brc) {
        info->low_delay_brc = true;
    }

    if (skipFrame) {
        info->skip_frame = true;
    }
}
#endif

void show_encoder_info(encoder_info_t *info) {
#define Name(X) (#X)
#define show_para_int(X) {std::string xname=Name(X);sock_log("%-30s: %d\n",xname.substr(6,xname.size()-1).c_str(), X);}
#define show_para_str(X) {std::string xname=Name(X);sock_log("%-30s: %s\n",xname.substr(6,xname.size()-1).c_str(), X);}
    sock_log("\nencoder_info:\n");
    show_para_int(info->nPixfmt);
    show_para_int(info->gop_size);
    show_para_str(info->codec);
    show_para_str(info->format);
    show_para_str(info->url);
    show_para_int(info->low_power);
    show_para_str(info->res);
    show_para_str(info->framerate);
    show_para_str(info->exp_vid_param);
    show_para_int(info->streaming);
    show_para_int(info->encodeType);
    show_para_int(info->encoderInstanceID);
    show_para_str(info->rate_contrl_param.ratectrl);
    show_para_str(info->rate_contrl_param.qp);
    show_para_str(info->rate_contrl_param.bitrate);
    show_para_str(info->rate_contrl_param.maxrate);
    show_para_str(info->rate_contrl_param.bufsize);
    show_para_str(info->rate_contrl_param.qfactor);
    show_para_int(info->rate_contrl_param.qmaxI);
    show_para_int(info->rate_contrl_param.qminI);
    show_para_int(info->rate_contrl_param.qmaxP);
    show_para_int(info->rate_contrl_param.qminP);
    show_para_int(info->quality);
    show_para_int(info->max_frame_size);
    show_para_str(info->ref_info.int_ref_type);
    show_para_int(info->ref_info.int_ref_cycle_size);
    show_para_int(info->ref_info.int_ref_qp_delta);
    show_para_int(info->slices);
    show_para_int(info->sei);
    show_para_str(info->finput);
    show_para_int(info->vframe);
    show_para_str(info->foutput);
    show_para_str(info->loglevel);
    show_para_int(info->latency_opt);
    show_para_int(info->auth);
    show_para_int(info->renderfps_enc);
    show_para_int(info->minfps_enc);
    show_para_int(info->roi_info.roi_enabled);
    show_para_int(info->roi_info.x);
    show_para_int(info->roi_info.y);
    show_para_int(info->roi_info.width);
    show_para_int(info->roi_info.height);
    show_para_int(info->roi_info.roi_value);
    show_para_str(info->profile);
    show_para_str(info->level);
    show_para_int(info->filter_nbthreads);
    show_para_int(info->low_delay_brc);
    show_para_int(info->skip_frame);
    show_para_str(info->plugin);
    show_para_int(info->user_id);
    show_para_int(info->tcaeEnabled);
    show_para_str(info->tcaeLogPath);
#undef Name
#undef show_para_int
#undef show_para_str
}

void show_env() {
    sock_log("\nicr_encoder environment variables:\n");

    const char *vaapi_device     = getenv(ENV_VAAPI_DEVICE);
    const char *render_port      = getenv(ENV_RENDER_PORT);
    const char *encode_unlimit   = getenv(ENV_ENCODE_UNLIMIT);
    const char *auxiliary_server = getenv(ENV_AUX_SERVER);

    if (vaapi_device)     sock_log("VAAPI_DEVICE       = %s\n", vaapi_device);
    if (render_port)      sock_log("render_server_port = %s\n", render_port);
    if (encode_unlimit)   sock_log("encode_unlimit     = %s\n", encode_unlimit);
    if (auxiliary_server) sock_log("auxiliary_server   = %s\n", auxiliary_server);
}

void clear_env() {
    unsetenv(ENV_VAAPI_DEVICE);
    unsetenv(ENV_RENDER_PORT);
    unsetenv(ENV_ENCODE_UNLIMIT);
    unsetenv(ENV_AUX_SERVER);
}

void icr_print_usage(const char *arg0) {
    sock_log(
        "\nUsage: %s <id> [options]\n"
        "\n"
        "Options:\n"
        "\n"
        "       -h\n"
        "           Print the help usage.\n"
        "\n"
        "       -streaming \n"
        "           Specify using streaming mode\n"
        "\n"
        "       -res <width>x<height> \n"
        "           Specify the resolution of display, for example: 720x1280 \n"
        "\n"
        "       -url value \n"
        "           Set the output muxer for video encoder\n"
        "           irrv muxer:  set -url as irrv:264 or irrv:265 \n"
        "           local muxer: set -url as local:YOUROWNFORMAT, for example local:h264 \n"
        "\n"
        "       -fr value \n"
        "           Specify target frame rate, default is 30 \n"
        "\n"
        "       -lowpower \n"
        "           Specify using low power encoding\n"
        "\n"
        "       -buffer \n"
        "           Specify the encoded input data format is RGBA in system memory \n"
        "           the default data is a vaSurface in GPU memory \n"
        "\n"
        "       -quality value \n"
        "           Specify target preset and the default is 4 for balance mode \n"
        "           range: 1,4,7 \n"
        "\n"
        "       -b value \n"
        "           Specify target bitrate, for example: -b 5M\n"
        "\n"
        "       -maxrate value \n"
        "           Maximum encode bitrate(bits/s), used with \"bufsize\"\n"
        "\n"
        "       -bufsize value \n"
        "           Set rate control buffer size(bits) \n"
        "\n"
        "       -qmaxI value \n"
        "           Maximum video quantizer scale for I frame \n"
        "           range: [0, 51]\n"
        "\n"
        "       -qminI value \n"
        "           Minimum video quantizer scale for I frame \n"
        "           range: [0, 51]\n"
        "\n"
        "       -qmaxP value \n"
        "           Maximum video quantizer scale for P frame \n"
        "           range: [0, 51]\n"
        "\n"
        "       -qminP value \n"
        "           Minimum video quantizer scale for P frame \n"
        "           range: [0, 51]\n"
        "\n"
        "       -finput path \n"
        "           Set the local file for input \n"
        "\n"
        "       -vframe value \n"
        "           Set the dumping output number of frames for local input \n"
        "\n"
        "       -foutput path \n"
        "           Set the local file for output \n"
        "\n"
        "       -g value \n"
        "           Set gop size(default gop size = 120) \n"
        "\n"
        "       -sei value \n"
        "           Set SEI to include :\n"
        "           2  -- encoder version info \n"
        "           8  -- encode timestamp \n"
        "           10 -- encoder version info and encode timestamp \n"
        "           the h264 encode support config encoder version/ timestamp info;\n"
        "           hevc encode only support config timestamp info at runtime.\n"
        "           The timestamp sei info can be parsered by decode_sei_timestamp_sample.\n"
        "\n"
        "       -ratectrl mode_name \n"
        "           Encoder rate control mode \n"
        "\n"
        "       -qfactor value \n"
        "           Quality factor, mainly used in QVBR/CIQ mode \n"
        "           range: [1, 51]\n"
        "\n"
        "       -qp value \n"
        "           Constant QP,  used in CQP mode \n"
        "           range: [1, 51]\n"
        "\n"
        "       -mfs value \n"
        "           Maximum encoded frame size in bytes, used in AVBR/CBR/VBR/QVBR mode \n"
        "\n"
        "       -rirtype value \n"
        "           Rolling intra refresh type, include: vertical/horizontal/slice \n"
        "           (slice type is only supported on ffmpeg4.2) \n"
        "           For \"vertical\" or \"horizontal\" type, -rircyclesize must be set too\n"
        "           For \"slice\" type, -slices muset be set too\n"
        "           valid values: v/h/s or 1/2/3 \n"
        "\n"
        "       -rircyclesize value \n"
        "           Number of frames in the intra refresh cycle \n"
        "\n"
        "       -rirdeltaqp value \n"
        "           QP difference for the refresh MBs \n"
        "           range: [-51, 51]\n"
        "\n"
        "       -slices value \n"
        "           Encoder number of slices, used in parallelized encoding\n"
        "\n"
        "       -profile value \n"
        "           Set encoder profile \n"
        "           The h264 encoder profile include: high/main/constrained_baseline \n"
        "           and the use high profile as default if not set or set error.\n"
        "           The hevc encoder profile include: main/main10/rext \n"
        "           and use main profile as default if not set or set error\n"
        "\n"
        "       -level value \n"
        "           Set encoder profile level \n"
        "           The h264 encoder profile level: \n"
        "           1/1.1/1.2/1.3/2/2.1/2.2/3/3.1/3.2/4/4.1/4.2/5/5.1/5.2/6/6.1/6.2 \n"
        "           and use 4.2 level as default level if not set or set error. \n"
        "           The hevc encoder profile level: \n"
        "           1/2/2.1/3/3.1/4/4.1/5/5.1/5.2/6/6.1/6.2/ \n"
        "           and use 4 level as default level if not set or set error. \n"
        "       -low_delay_brc \n"
        "           average frame size set by target bitrate \n"
        "           should be used at VBR/QVBR mode. \n"
        "       -skipframe \n"
        "           enable the skip frame function \n"
        "       -plugin value\n"
        "           value could be vaapi or qsv \n"
        "\n",
        arg0
    );
}

