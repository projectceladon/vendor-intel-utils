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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_MEMORY_MANAGER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_MEMORY_MANAGER_H

#include "base_executor.h"
#include "vk_pool_info.h"
#include "vk_memory_info.h"

NAME_SPACE_BEGIN

class VkMemoryManager
{
public:
    VkMemoryManager() {}
    ~VkMemoryManager() {}

    bool initFromModel(const Model& model);
    bool resetFromRequest(const Request& request);
    bool sync();
    void clean();

    VkPoolInfo* getModelPoolInfo(size_t index)
    {
        ASSERT(index < modelPoolInfos.size());
        return &modelPoolInfos[index];
    }
    VkMemoryInfo* createModelMemoryInfo(uint8_t* userptr, size_t length);

    VkPoolInfo* getRequestPoolInfo(size_t index)
    {
        ASSERT(index < requestPoolInfos.size());
        return &requestPoolInfos[index];
    }
    VkMemoryInfo* createRequestMemoryInfo(uint8_t* userptr, size_t length);

    VkMemoryInfo* createIntermediumMemoryInfo(size_t length);
    VkMemoryInfo* createIntermediumMemoryInfo(uint8_t* userptr, size_t length);

private:
    std::vector<VkPoolInfo> modelPoolInfos;
    std::vector<VkMemoryInfo> modelMemInfos;

    std::vector<VkPoolInfo> requestPoolInfos;
    std::vector<VkMemoryInfo> requestMemInfos;

    std::vector<VkMemoryInfo> intermediumMemInfos;

    void cleanPoolInfos(std::vector<VkPoolInfo>& poolInfos) const;
    VkMemoryInfo* createMemoryInfo(std::vector<VkMemoryInfo>& memInfos, uint8_t* userptr, size_t length) const;
};

NAME_SPACE_STOP

#endif
