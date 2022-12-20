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

/**
 * @file
 * CRC function headers
 */

#ifndef __GA_CRC_H__
#define __GA_CRC_H__

#include "ga-common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char crc5_t;

/// for CRC5 (CRC5-USB and CRC5-CCITT)
/**
 * Initialization function for CRC5 functions */
static inline crc5_t crc5_init(void)        { return 0x1f << 3; }
/**
 * Finalize function for CRC5 functions */
static inline crc5_t crc5_finalize(crc5_t crc)    { return (crc>>3) ^ 0x1f; }
EXPORT crc5_t crc5_reflect(crc5_t data, int data_len);
EXPORT crc5_t crc5_update(crc5_t crc, const unsigned char *data, int data_len, const crc5_t *table);
EXPORT crc5_t crc5_update_usb(crc5_t crc, const unsigned char *data, int data_len);
EXPORT crc5_t crc5_update_ccitt(crc5_t crc, const unsigned char *data, int data_len);

#ifdef __cplusplus
}
#endif

#endif
