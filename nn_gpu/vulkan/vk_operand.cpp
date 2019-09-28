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
#include "vk_operand.h"
#include "vk_memory_manager.h"
#include "vk_memory_info.h"

NAME_SPACE_BEGIN

void VkOperand::reset(const int batch, const int width, const int height, const int channel)
{
    dimensions[kShapeIdxBatch]   = batch;
    dimensions[kShapeIdxHeight]  = height;
    dimensions[kShapeIdxWidth]   = width;
    dimensions[kShapeIdxChannel] = channel;

    length   = getElementCount() * getBasicTypeSize();
    lifetime = OperandLifeTime::TEMPORARY_VARIABLE;
    memInfo  = nullptr;
    valPtr   = nullptr;

    getVkBuffer();

    return;
}

bool VkOperand::setArg(const RequestArgument& from)
{
    NN_GPU_ENTRY();
    ASSERT(lifetime == OperandLifeTime::MODEL_INPUT || lifetime == OperandLifeTime::MODEL_OUTPUT);

    length = from.location.length;
    offset = from.location.offset;
    NN_GPU_DEBUG("length is %zu, offset is %zu", length, offset);

    if (from.dimensions.size() > 0)
    {
//        ASSERT(!"what's happened");
        dimensions = from.dimensions;
    }

    NN_GPU_DEBUG("dimensions info is: shape size is %d", (int)dimensions.size());
    for(int i = 0; i < (int)dimensions.size(); i++)
    {
        NN_GPU_DEBUG("shape index %d size is %d", i, dimensions[i]);
    }

    if (from.hasNoValue) {
        lifetime = OperandLifeTime::NO_VALUE;
        ASSERT(!"what's happened");
    }
    else
    {
        poolIndex = from.location.poolIndex;
        VkPoolInfo* poolInfo = memMgr.getRequestPoolInfo(poolIndex);
        uint8_t* userptr = poolInfo->getUserptr() + offset;
        NN_GPU_DEBUG("userptr is %f, %f, %f, %f, %f, %f, %f, %f",
            *(reinterpret_cast<float *>(userptr)),
            *(reinterpret_cast<float *>(userptr)+1),
            *(reinterpret_cast<float *>(userptr)+2),
            *(reinterpret_cast<float *>(userptr)+3),
            *(reinterpret_cast<float *>(userptr)+4),
            *(reinterpret_cast<float *>(userptr)+5),
            *(reinterpret_cast<float *>(userptr)+6),
            *(reinterpret_cast<float *>(userptr)+7));

        memInfo = memMgr.createRequestMemoryInfo(userptr, length);
        poolInfo->addMemInfo(memInfo);

        // only output need sync?
        if (lifetime == OperandLifeTime::MODEL_OUTPUT)
        {
            memInfo->setNeedSync();
        }
    }
    NN_GPU_EXIT();

    return true;
}

void VkOperand::restore(const Operand& from)
{
    switch (from.lifetime)
    {
        case OperandLifeTime::TEMPORARY_VARIABLE:
            numberOfUsesLeft = from.numberOfConsumers;
            break;
        default:
            //do nothing
            break;
    }
}

bool VkOperand::set(const Operand& from, uint8_t* vp, uint32_t index)
{
    NN_GPU_ENTRY();
    operandIndex = index;
    poolIndex = 0;
    valPtr = nullptr;
    type = from.type;
    dimensions = from.dimensions;
    lifetime = from.lifetime;
    scale = from.scale;
    zeroPoint = from.zeroPoint;
    length = from.location.length;
    offset = from.location.offset;

    switch (from.lifetime)
    {
        case OperandLifeTime::TEMPORARY_VARIABLE:
            NN_GPU_DEBUG("operand index is %d, lifetime is temp", index);
            numberOfUsesLeft = from.numberOfConsumers;
            length = getElementCount() * getBasicTypeSize();
            ASSERT(offset == 0);
            break;
        case OperandLifeTime::CONSTANT_COPY:
            NN_GPU_DEBUG("operand index is %d, lifetime is constant copy", index);
            valPtr = vp;
            break;
        case OperandLifeTime::CONSTANT_REFERENCE:
        {
            poolIndex = from.location.poolIndex;
            VkPoolInfo* poolInfo = memMgr.getModelPoolInfo(poolIndex);
            uint8_t* userptr = poolInfo->getUserptr() + offset;
            memInfo = memMgr.createModelMemoryInfo(userptr, length);
            poolInfo->addMemInfo(memInfo);
            NN_GPU_DEBUG("operand index is %d, lifetime is temp", index);
            break;
        }
        case OperandLifeTime::MODEL_INPUT:
        case OperandLifeTime::MODEL_OUTPUT:
            // do nothing here
            NN_GPU_DEBUG("operand index is %d, lifetime is input/output", index);
            break;
        case OperandLifeTime::NO_VALUE:
            ASSERT(!"to know what's this");
            break;
        default:
            NOT_REACH_HERE;
            break;
    }
    NN_GPU_EXIT();
    return true;
}

size_t VkOperand::getBasicTypeSize()
{
    switch (type)
    {
        case OperandType::TENSOR_FLOAT32:
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
            return 4;
            break;
        case OperandType::TENSOR_QUANT8_ASYMM:
            return 1;
            break;
        default:
            NOT_IMPLEMENTED;
            break;
    }

    return 0;
}

inline int elementCount(const Shape& shape, int start = -1, int end = -1)
{
    if (start == -1) start = 0;
    if (end == -1) end = (int)shape.size();

    if (shape.size() == 0)
        return 0;

    int elems = 1;
    ASSERT(start <= (int)shape.size() &&
           end <= (int)shape.size() &&
           start <= end);
    for(int i = start; i < end; i++)
    {
        elems *= shape[i];
    }
    return elems;
}

int VkOperand::getElementCount()
{
    int count = 0;
    count = elementCount(dimensions);
    return count;
}

int VkOperand::getElementCount(int32_t axis)
{
    int count = 0;
    count = elementCount(dimensions, axis);
    return count;
}

int VkOperand::getElementCount(int32_t start_axis, int32_t end_axis)
{
    int count = 0;
    count = elementCount(dimensions, start_axis, end_axis);
    return count;
}

VkBuffer VkOperand::getVkBuffer()
{
    NN_GPU_CALL();

    if (memInfo == nullptr)
    {
        if (lifetime == OperandLifeTime::TEMPORARY_VARIABLE)
        {
            memInfo = memMgr.createIntermediumMemoryInfo(length);
        }
        else
        {
            ASSERT(lifetime == OperandLifeTime::CONSTANT_COPY);
            ASSERT(dimensions.size() > 0);
            memInfo = memMgr.createIntermediumMemoryInfo(valPtr, length);
        }
    }
    return memInfo->getVkBuffer();
}

void VkOperand::dump()
{
    memInfo->dump();
}

void VkOperand::dumpToFile(const char* file_name, const int channels)
{
    memInfo->dumpToFile(file_name, channels);
}

void VkOperand::shareGpuStorage(VkOperand& from)
{
    // assure the storage allocation of from
    from.getVkBuffer();

    if (lifetime == OperandLifeTime::MODEL_OUTPUT)
    {
        // be careful when gpu object is able to be
        // created from userptr without copy
        memInfo->shareFrom(from.memInfo);
    }
    else
    {
        ASSERT(lifetime == OperandLifeTime::TEMPORARY_VARIABLE);
        ASSERT(memInfo == nullptr);
        memInfo = from.memInfo;
        memInfo->incRef();
    }
}

void VkOperand::markOpFinished()
{
    if (lifetime == OperandLifeTime::TEMPORARY_VARIABLE)
    {
        ASSERT(memInfo != nullptr);
        numberOfUsesLeft--;
        if (numberOfUsesLeft == 0)
        {
            memInfo->setNotInUsing();
            memInfo = nullptr;
        }
    }
}

inline bool checkType(OperandType type)
{
    return (type >= OperandType::FLOAT32 && type < OperandType::TENSOR_QUANT8_ASYMM) ? true : false;
}

#if 0
VkOperand VkOperand::reshape(const char* data, const hidl_vec<uint32_t>& shape)
{
	bool alloc;
    if (kDevice == VK_NULL_HANDLE)
    {
        LOGE("device is NULL");
        return *this;
    }

    ASSERT(shape.size() > 0 && shape.size() <= 6);

    if (dimensions != shape) dimensions = shape;

    size_t new_size = getElementCount() * getBasicTypeSize();
    if (new_size > length)
        alloc = true;
    length = new_size;

    if (alloc)
    {
        memInfo.reset(new VkMemoryInfo(length, data));
    }
    else if (data)
    {
        void* p = map();
        memcpy(p, data, length);
        unMap();
    }

    return *this;
}

void VkOperand::copyTo(VkOperand& dst)
{
    void* p = map();
    dst.reshape((const char*)p, dimensions);
    unMap();
}

#endif

void VkOperand::resetForTune()
{
    memInfo->resetForTune();
}

void VkOperand::copyToBuffer(float* to_buf, const size_t buf_size)
{
    memInfo->copyToBuffer(to_buf, buf_size);
}

//added done

NAME_SPACE_STOP
