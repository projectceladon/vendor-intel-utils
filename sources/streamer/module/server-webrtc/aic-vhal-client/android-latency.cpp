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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "ga-common.h"
#define TAG "android-latency: "

namespace android {

static int atrace_marker_fd = -1;
static int pid = getpid();

static inline void write_msg(const std::string& msg) {
    ssize_t ret = write(atrace_marker_fd, msg.c_str(), msg.size());
    if (-1 == ret) {
        ga_logger(Severity::ERR, TAG "failed to write to ftrace trace_marker\n");
    }
}

void atrace_init() {
    if (atrace_marker_fd >= 0)
        return;

    atrace_marker_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
    if (atrace_marker_fd == -1) {
        ga_logger(Severity::ERR, "Error opening trace file: %s (%d)", strerror(errno), errno);
    }
}

void atrace_deinit() {
    if (atrace_marker_fd >= 0) {
        close(atrace_marker_fd);
        atrace_marker_fd = -1;
    }
}

void atrace_begin(const std::string& name) {
    std::string msg = "B|" + std::to_string(pid) + "|" + name;
    write_msg(msg);
}

void atrace_end() {
    std::string msg = "E|" + std::to_string(pid);
    write_msg(msg);
}

bool is_atrace_enabled() {
    return atrace_marker_fd >= 0;
}

}
