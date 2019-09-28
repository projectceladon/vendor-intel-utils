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
#include "vk_memory_info.h"
#include "vk_buffer.h"
#include "vk_wrapper.h"

NAME_SPACE_BEGIN

void VkMemoryInfo::clean()
{
    userptr = nullptr;
}

void VkMemoryInfo::setNotInUsing()
{
    ASSERT(refCount > 0);
    refCount--;
    if (refCount == 0)
    {
        inUsing = false;
    }
}

bool VkMemoryInfo::sync(std::string name)
{
    if (name == "mmap_fd" || name == "ashmem")
    {
        if (needSync)
        {
            uint8_t* data;
            data = buffer->map();
            for (size_t i = 0; i < length; ++i)
            {
                userptr[i] = data[i];
            }
            buffer->unMap();
            if (name == "mmap_fd")
                msync(userptr, length, MS_SYNC);
        }
    }
    else
    {
        NOT_IMPLEMENTED;
    }

    return true;
}

VkBuffer VkMemoryInfo::getVkBuffer()
{
	if (!buffer)
		buffer.reset(new Buffer(length, userptr)); 
	return buffer->getVkBuffer();
}

void VkMemoryInfo::dump()
{
    if (buffer)
    {
        buffer->dump();
    }
}

void VkMemoryInfo::dumpToFile(const char* file_name, const int channels)
{
    if (buffer)
    {
        buffer->dumpToFile(file_name, channels);
    }
}

void VkMemoryInfo::resetForTune()
{
    if (buffer)
    {
        buffer->resetForTune();
    }
}

void VkMemoryInfo::copyToBuffer(float* to_buf, const size_t buf_size)
{
    if (buffer)
    {
        buffer->copyToBuffer(to_buf, buf_size);
    }
}

NAME_SPACE_STOP
