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
 */

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_POOL_INFO_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_POOL_INFO_H

#include "base_executor.h"
#include "vk_memory_info.h"

NAME_SPACE_BEGIN

class VkMemoryInfo;

class VkPoolInfo {
public:
    VkPoolInfo() : userptr(nullptr), size(0) {}
    ~VkPoolInfo() {}
    bool set(const hidl_memory& hidlMemory);
    bool sync();
    bool clean();
    void addMemInfo(VkMemoryInfo* memInfo) { memInfos.push_back(memInfo); }
    uint8_t* getUserptr() { return userptr; }

private:
    sp<IMemory> memory;
    hidl_memory hidlMemory;
    std::string name;
    uint8_t* userptr;
    size_t size;

    std::vector<VkMemoryInfo*> memInfos;
};

NAME_SPACE_STOP

#endif
