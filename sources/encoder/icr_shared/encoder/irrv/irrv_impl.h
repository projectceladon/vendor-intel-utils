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

#ifndef IRRV_IMPL_H
#define IRRV_IMPL_H

#include "irrv_protocol.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int irrv_checknewconn(void *opaque);
bool irrv_check_authentication(irrv_uuid_t id, irrv_uuid_t key);
int irrv_writeback(void *opaque, uint8_t *data, size_t size);
int irrv_writeback2(void *opaque, uint8_t *data, size_t size, int type);
int irrv_send_message(void *opaque, int msg, unsigned int value);
void irrv_close(void *opaque);

#ifdef __cplusplus
}
#endif

#endif // IRRV_IMPL_H

