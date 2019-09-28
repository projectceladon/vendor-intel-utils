/*
 * Copyright @2017 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Guo Yejun <yejun.guo@intel.com>
 */

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GLES_POOL_INFO_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GLES_POOL_INFO_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

#include "base_executor.h"

NAME_SPACE_BEGIN

class GlesMemoryInfo;

class GlesPoolInfo {
public:
    GlesPoolInfo() : userptr(nullptr), size(0) {}
    ~GlesPoolInfo() {}
    bool set(const hidl_memory& hidlMemory);
    bool sync();
    bool clean();
    void addMemInfo(GlesMemoryInfo* memInfo) { memInfos.push_back(memInfo); }
    uint8_t* getUserptr() { return userptr; }
private:
    sp<IMemory> memory;
    hidl_memory hidlMemory;
    std::string name;
    uint8_t* userptr;
    size_t size;

    std::vector<GlesMemoryInfo*> memInfos;
};

NAME_SPACE_STOP

#endif
