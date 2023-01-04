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

#ifndef __PROFILELEVEL_H
#define __PROFILELEVEL_H

#include <stdio.h>
#include <map>

extern "C" {
#include <libavcodec/avcodec.h>
}

const static std::map<std::string, int> vaapi_encode_h264_profiles = {
    { "high",                 FF_PROFILE_H264_HIGH                 },
    { "main",                 FF_PROFILE_H264_MAIN                 },
    { "constrained_baseline", FF_PROFILE_H264_CONSTRAINED_BASELINE },
    { "default",              FF_PROFILE_H264_HIGH                 },
};

const static std::map<std::string, int> vaapi_encode_h265_profiles = {
    { "main",                 FF_PROFILE_HEVC_MAIN    },
    { "main10",               FF_PROFILE_HEVC_MAIN_10 },
    { "rext",                 FF_PROFILE_HEVC_REXT    },
#ifdef FFMPEG_v42
    { "scc",                  FF_PROFILE_HEVC_SCC     },
#endif
    { "default",              FF_PROFILE_HEVC_MAIN    },
};

const static std::map<std::string, int> vaapi_encode_mjpeg_profiles = {
    { "baseline",                 FF_PROFILE_MJPEG_HUFFMAN_BASELINE_DCT },
    { "default",                  FF_PROFILE_MJPEG_HUFFMAN_BASELINE_DCT },
};

const static std::map<std::string, int> vaapi_encode_av1_profiles = {
    { "main",                 FF_PROFILE_AV1_MAIN    },
    { "high",                 FF_PROFILE_AV1_HIGH    },
    { "professional",         FF_PROFILE_AV1_PROFESSIONAL },
    { "default",              FF_PROFILE_AV1_MAIN    },
};

const static std::map<std::string, int> vaapi_encode_h264_levels = {
    { "1",           10 },
    { "1.1",         11 },
    { "1.2",         12 },
    { "1.3",         13 },
    { "2",           20 },
    { "2.1",         21 },
    { "2.2",         22 },
    { "3",           30 },
    { "3.1",         31 },
    { "3.2",         32 },
    { "4",           40 },
    { "4.1",         41 },
    { "4.2",         42 },
    { "5",           50 },
    { "5.1",         51 },
    { "5.2",         52 },
    { "6",           60 },
    { "6.1",         61 },
    { "6.2",         62 },
    { "default",     42 },
};

const static std::map<std::string, int> vaapi_encode_h265_levels = {
    { "1",           30 },
    { "2",           60 },
    { "2.1",         63 },
    { "3",           90 },
    { "3.1",         93 },
    { "4",          120 },
    { "4.1",        123 },
    { "5",          150 },
    { "5.1",        153 },
    { "5.2",        156 },
    { "6",          180 },
    { "6.1",        183 },
    { "6.2",        186 },
    { "default",    120 },
};

const static std::map<std::string, int> vaapi_encode_av1_levels = {
    { "2",           20 },
    { "2.1",         21 },
    { "2.2",         22 },
    { "2.3",         23 },
    { "3",           30 },
    { "3.1",         31 },
    { "3.2",         32 },
    { "3.3",         33 },
    { "4",           40 },
    { "4.1",         41 },
    { "4.2",         42 },
    { "4.3",         43 },
    { "5",           50 },
    { "5.1",         51 },
    { "5.2",         52 },
    { "5.3",         53 },
    { "6",           60 },
    { "6.1",         61 },
    { "6.2",         62 },
    { "6.3",         63 },
    { "7 ",          70 },
    { "7.1",         71 },
    { "7.2",         72 },
    { "7.3",         73 },
    { "default",     40 },
};

#endif  /* __PROFILELEVEL_H */
