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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_MEMORY_INFO_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_MEMORY_INFO_H

#include "base_executor.h"
#include "vk_common.h"
#include "vk_buffer.h"

NAME_SPACE_BEGIN

class VkMemoryInfo
{
public:
    //todo, device is not set
    VkMemoryInfo(uint8_t* us, size_t le) :
                userptr(us), length(le), inUsing(true), refCount(1), needSync(false)
                {}
    ~VkMemoryInfo() {}
    bool sync(std::string name);
    void clean();
    void setNeedSync() { needSync = true; }
    void setNotInUsing();
    void incRef() { refCount++; }
    void resetRef() { refCount = 0;}
    void shareFrom(VkMemoryInfo* from) { buffer = from->buffer; from->incRef();}
    VkBuffer getVkBuffer();
    void dump();
    void dumpToFile(const char* file_name, const int channels = 0);
    void resetForTune();
    void copyToBuffer(float* to_buf, const size_t buf_size);
private:
    uint8_t* userptr;
    size_t length;
    bool inUsing;
    uint32_t refCount;
    bool needSync;
    friend class VkMemoryManager;
    std::shared_ptr<Buffer> buffer;
};

NAME_SPACE_STOP

#endif
