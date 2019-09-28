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

#include <sys/mman.h>
#include "vk_memory_manager.h"
#include "vk_common.h"

NAME_SPACE_BEGIN

VkMemoryInfo* VkMemoryManager::createMemoryInfo(std::vector<VkMemoryInfo>& memInfos, uint8_t* userptr, size_t length) const
{
    size_t count = memInfos.size();

    ASSERT(memInfos.capacity() > count);

    VkMemoryInfo info(userptr, length);
    memInfos.push_back(info);

    return &memInfos[count];
}

VkMemoryInfo* VkMemoryManager::createRequestMemoryInfo(uint8_t* userptr, size_t length)
{
    return createMemoryInfo(requestMemInfos, userptr, length);
}

VkMemoryInfo* VkMemoryManager::createModelMemoryInfo(uint8_t* userptr, size_t length)
{
    return createMemoryInfo(modelMemInfos, userptr, length);
}

VkMemoryInfo* VkMemoryManager::createIntermediumMemoryInfo(size_t length)
{
    size_t count = intermediumMemInfos.size();
    for (size_t i = 0; i < count; i++)
    {
        VkMemoryInfo& info = intermediumMemInfos[i];
        if (!info.inUsing && info.length == length && info.userptr == nullptr)
        {
            info.inUsing = true;
            info.refCount = 1;
            return &info;
        }
    }

    return createMemoryInfo(intermediumMemInfos, nullptr, length);
}

VkMemoryInfo* VkMemoryManager::createIntermediumMemoryInfo(uint8_t* userptr, size_t length)
{
    return createMemoryInfo(intermediumMemInfos, userptr, length);
}

bool VkMemoryManager::initFromModel(const Model& model)
{
    modelPoolInfos.resize(model.pools.size());
    for (size_t i = 0; i < model.pools.size(); i++)
    {
        if (!modelPoolInfos[i].set(model.pools[i]))
        {
            LOGE("Could not map pool");
            return false;
        }
    }

    // just reserve size of big enough
    modelMemInfos.reserve(model.operands.size());
    requestMemInfos.reserve(model.operands.size());
    intermediumMemInfos.reserve(model.operands.size());
    return true;
}

bool VkMemoryManager::resetFromRequest(const Request& request)
{
    ASSERT(requestPoolInfos.size() == request.pools.size() ||
                                requestPoolInfos.size() == 0);
    cleanPoolInfos(requestPoolInfos);
    requestMemInfos.clear();

    requestPoolInfos.resize(request.pools.size());
    for (size_t i = 0; i < request.pools.size(); i++)
    {
        if (!requestPoolInfos[i].set(request.pools[i]))
        {
            LOGE("Could not map pool");
            return false;
        }
    }

    for (auto& mem : intermediumMemInfos)
    {
        mem.resetRef();
    }

    return true;
}

bool VkMemoryManager::sync()
{
    // should we try for modelPoolInfos?

    for (size_t i = 0; i < requestPoolInfos.size(); i++)
    {
        requestPoolInfos[i].sync();
    }
    return true;
}

void VkMemoryManager::cleanPoolInfos(std::vector<VkPoolInfo>& poolInfos) const
{
    for (size_t i = 0; i < poolInfos.size(); i++)
    {
        poolInfos[i].clean();
    }
}

void VkMemoryManager::clean()
{
    cleanPoolInfos(modelPoolInfos);
    cleanPoolInfos(requestPoolInfos);

    for (auto& mem : intermediumMemInfos)
    {
        mem.clean();
    }
}

NAME_SPACE_STOP