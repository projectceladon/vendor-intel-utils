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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_OPERAND_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_OPERAND_H

#include "base_executor.h"
#include "vk_common.h"
#include "vk_buffer.h"

NAME_SPACE_BEGIN

class VkMemoryManager;
class VkPoolInfo;
class VkMemoryInfo;

typedef hidl_vec<uint32_t> Shape;

class VkOperand
{
public:
    VkOperand(VkMemoryManager& mgr) :
            memMgr(mgr), memInfo(nullptr), poolIndex(0),
            offset(0), length(0), valPtr(nullptr), numberOfUsesLeft(0), operandIndex(-1) {}

    ~VkOperand() {}

    bool set(const Operand& from, uint8_t* vp, uint32_t index);
    void reset(const int batch, const int width, const int height, const int channel);
    void resetForTune();
    void restore(const Operand& from);
    bool setArg(const RequestArgument& from);
    void shareGpuStorage(VkOperand& shareFrom);
    void markOpFinished();
    int getElementCount();
    int getElementCount(int32_t axis);
    int getElementCount(int32_t start_axis, int32_t end_axis);

    template <typename T>
    T getScalarData() const
    {
        const T* data = reinterpret_cast<const T*>(valPtr);
        return data[0];
    }

    uint32_t getDimensionSize(uint32_t idx)
    {
        if (idx >= dimensions.size())
        {
            LOGW("Invalid dimension idx: %d\n", idx);
            return 0;
        }
        return dimensions[idx];
    }

    uint32_t getNumberOfDimensions()
    {
        return dimensions.size();
    }

    OperandType getType()
    {
        return type;
    }

    VkBuffer getVkBuffer();

    void dump();
    void dumpToFile(const char* file_name = "img_data", const int channels = 0);
    void copyToBuffer(float* to_buf, const size_t buf_size);

    Shape getShape() const { return dimensions; }

#if 0
    // Change shape and format to as passed in.
    // Copy data if data != NULL
    // Allocate new internal buffer if new size > old size or alloc flag is true
    VkOperand reshape(const char* data, const hidl_vec<uint32_t>& shape);
    void copyTo(VkOperand& dst);
#endif
    void bindOperand(VkOperand& operand, int binding, VkDescriptorSet descriptor_set);

    size_t size() const { return length; }
    bool isEmpty() { return length == 0 ? true : false; }
    std::shared_ptr<Buffer> getBuffer() { return buffer; }

private:
    size_t getBasicTypeSize();

    VkMemoryManager& memMgr;
    VkMemoryInfo* memInfo;

    uint32_t poolIndex;
    size_t offset;                  //in bytes
    size_t length;                  //in bytes
    uint8_t* valPtr;                //for operand value not provided by pool
    OperandType type;
    OperandLifeTime lifetime;
    size_t numberOfUsesLeft;
    float scale;
    int32_t zeroPoint;

    // just for debug
    uint32_t operandIndex;

    hidl_vec<uint32_t> dimensions;
    std::shared_ptr<Buffer> buffer;
};

NAME_SPACE_STOP

#endif
