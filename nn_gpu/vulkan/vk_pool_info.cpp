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
#include "vk_common.h"
#include "vk_pool_info.h"

NAME_SPACE_BEGIN

bool VkPoolInfo::clean()
{
    ASSERT(userptr != nullptr);
    if (name == "mmap_fd")
    {
        munmap(userptr, size);
    }
    userptr = nullptr;

    for (auto& mem : memInfos)
    {
        mem->clean();
    }
    memInfos.clear();

    return true;
}

bool VkPoolInfo::sync()
{
    if (name == "mmap_fd")
    {
        int prot = hidlMemory.handle()->data[1];
        if (prot & PROT_WRITE)
        {
            for (auto& mem : memInfos)
            {
                mem->sync(name);
            }
        }
    }
    else if (name == "ashmem")
    {
        for (auto& mem : memInfos)
        {
            mem->sync(name);
        }
        memory->commit();
    }
    else
    {
        NOT_IMPLEMENTED;
    }

    return true;
}

bool VkPoolInfo::set(const hidl_memory& hidlMemory)
{
    this->hidlMemory = hidlMemory;
    name = hidlMemory.name();
    if (name == "mmap_fd")
    {
        size = hidlMemory.size();
        int fd = hidlMemory.handle()->data[0];
        int prot = hidlMemory.handle()->data[1];
        size_t offset = getSizeFromInts(hidlMemory.handle()->data[2],
                                        hidlMemory.handle()->data[3]);
        userptr = static_cast<uint8_t*>(mmap(nullptr, size, prot, MAP_SHARED, fd, offset));
        if (userptr == MAP_FAILED)
        {
            LOGE("Can't mmap the file descriptor.");
            return false;
        }
        return true;
    }
    else if (name == "ashmem")
    {
        memory = mapMemory(hidlMemory);
        if (memory == nullptr) {
            LOGE("Can't map shared memory.");
            return false;
        }
        memory->update();
        userptr = reinterpret_cast<uint8_t*>(static_cast<void*>(memory->getPointer()));
        if (userptr == nullptr) {
            LOGE("Can't access shared memory.");
            return false;
        }
        return true;
    }
    else
    {
        NOT_IMPLEMENTED;
    }
}

NAME_SPACE_STOP
