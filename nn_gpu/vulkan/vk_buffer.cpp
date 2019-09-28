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
#include "vk_common.h"
#include "vk_buffer.h"
#include "vk_wrapper.h"

NAME_SPACE_BEGIN

static uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(kPhysicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((memoryTypeBits & (1 << i)) &&
                ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
            return i;
    }
    return -1;
}

bool Buffer::init(const uint8_t* data)
{
    if (buffer != VK_NULL_HANDLE)
    {
        LOGW("Buffer object already inited\n");
        return false;
    }

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = length;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, NULL, &buffer));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &allocateInfo, NULL, &memory));

    if (data)
    {
        uint8_t* dst;
        VK_CHECK_RESULT(vkMapMemory(device, memory, 0, length, 0, (void **)&dst));
        NN_GPU_DEBUG("call %s, userptr data is %f, size_in_bytes is %zu",
            __func__, 
            *(reinterpret_cast<const float *>(data)),
            length);
        memcpy(dst, data, length);
        vkUnmapMemory(device, memory);
    }
    VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, memory, 0));
    return true;
}

void Buffer::dump()
{
    if (memory != VK_NULL_HANDLE) {
        uint8_t* data;

        VK_CHECK_RESULT(vkMapMemory(device, memory, 0, length, 0, (void **)&data));
        NN_GPU_DEBUG("call %s, userptr data is %f, size_in_bytes is %zu",
            __func__, 
            *(reinterpret_cast<const float *>(data)),
            length);
        const float* fp = reinterpret_cast<const float *>(data);
        // only dump the first 16 float numberbs
        for (size_t i = 0; i < 15; ++i)
        {
            NN_GPU_DEBUG("dumpped out buffer content is: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
                fp[0], fp[1], fp[2], fp[3], fp[4], fp[5], fp[6], fp[7], fp[8], fp[9], fp[10], fp[11], fp[12],
                fp[13], fp[14], fp[15]);
        }
        vkUnmapMemory(device, memory);
    }
}

void Buffer::dumpToFile(const char* fileName, const int channels)
{
    static uint32_t file_no = 0;

    std::string file = std::string("/data/") + std::string("vk_") + std::string(fileName)
                    + std::to_string(file_no) + ".txt";
    FILE* file_ptr = fopen(file.c_str(), "w+");

    file_no++;

    if (file_ptr == nullptr)
    {
        NN_GPU_DEBUG("call %s, create file %s failed", __func__, file.c_str());
        return;
    }

    if (memory != VK_NULL_HANDLE)
    {
        uint8_t* data;

        VK_CHECK_RESULT(vkMapMemory(device, memory, 0, length, 0, (void **)&data));

        const float* fp = reinterpret_cast<const float *>(data);
        int cur_c = 1;
        const size_t f_len = length / 4;

        NN_GPU_DEBUG("%s: dumpped file length is %zu", __func__, f_len);

        for (size_t i = 0; i < f_len; i++)
        {
            fprintf(file_ptr, "%f,", fp[i]);
            if (channels != 0 && cur_c % channels == 0) {
                fprintf(file_ptr, "\n");
            }
            cur_c++;
        }
        vkUnmapMemory(device, memory);
    }

    fclose(file_ptr);
}

Buffer::Buffer(size_t size_in_bytes, const uint8_t* data)
{
    device = kDevice;
    buffer = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
    length = size_in_bytes;
    init(data);
}

Buffer::~Buffer()
{
    vkFreeMemory(device, memory, NULL);
    vkDestroyBuffer(device, buffer, NULL);
}

uint8_t* Buffer::map()
{
    void *p;
    ASSERT(memory != VK_NULL_HANDLE);

    VK_CHECK_RESULT(vkMapMemory(device, memory,
                                0, length, 0, (void **)&p));

    return (uint8_t*)p;
}

void Buffer::unMap()
{
    vkUnmapMemory(device, memory);
}

void Buffer::resetForTune()
{
    if (memory != VK_NULL_HANDLE)
    {
        uint8_t* data;
        VK_CHECK_RESULT(vkMapMemory(device, memory, 0, length, 0, (void **)&data));

        const size_t buf_size = length / 4;
        float* fp = reinterpret_cast<float *>(data);

        // reset output
        for (size_t i = 0; i < buf_size; ++i)
        {
            fp[i] = 7.28f;
        }

        vkUnmapMemory(device, memory);
    }
}

void Buffer::copyToBuffer(float* to_buf, const size_t buf_size)
{
    ASSERT(to_buf != nullptr && buf_size > 0);

    if (memory != VK_NULL_HANDLE)
    {
        uint8_t* data;
        VK_CHECK_RESULT(vkMapMemory(device, memory, 0, length, 0, (void **)&data));

        float* fp = reinterpret_cast<float*>(data);
        if (buf_size <= length)
        {
            memcpy(to_buf, fp, buf_size);
        }
        else
        {
            LOG(ERROR) << "copyToBuffer: buf_size is greater than vk buffer size";
        }

        vkUnmapMemory(device, memory);
    }
}

NAME_SPACE_STOP
