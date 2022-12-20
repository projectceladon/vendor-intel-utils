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

#ifndef _CLI_HPP_
#define _CLI_HPP_

#include "data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void icr_print_usage(const char *arg0);

void icr_parse_args(int argc, char *argv[], encoder_info_t &info);

void show_env();

void clear_env();

void show_encoder_info(encoder_info_t *info);

void rir_type_sanity_check(encoder_info_t *info);

int bitrate_ctrl_sanity_check(encoder_info_t *info);

#ifndef BUILD_FOR_HOST
void encoder_properties_sanity_check(encoder_info_t *info);
#endif

#ifdef __cplusplus
}
#endif

#endif
