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

#pragma once

#include "ga-conf.h"

namespace android {

static inline std::string ip()
{
  std::string address;
  if (ga_conf_readbool("k8s", 0) == 1) {
    address = "127.0.0.1";
  } else {
    int session = ga_conf_readint("android-session");
    if (session < 0)
      session = 0;
    address = "172.100." +
      std::to_string((session+2)/256) + "." +
      std::to_string((session+2)%256);
  }

  return address;
}

void atrace_init();
void atrace_deinit();
void atrace_begin(const std::string& name);
void atrace_end();
bool is_atrace_enabled();

}
