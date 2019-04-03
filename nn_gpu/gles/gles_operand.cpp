#include "gles_operand.h"
#include "gles_memory_manager.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesOperand::setArg(const RequestArgument& from)
{
    ASSERT(lifetime == OperandLifeTime::MODEL_INPUT || lifetime == OperandLifeTime::MODEL_OUTPUT);

    length = from.location.length;
    offset = from.location.offset;

    if (from.dimensions.size() > 0)
    {
//        ASSERT(!"what's happened");
        dimensions = from.dimensions;
    }

    if (from.hasNoValue) {
        ASSERT(!"what's happened");
        lifetime = OperandLifeTime::NO_VALUE;
    }
    else
    {
        poolIndex = from.location.poolIndex;
        GlesPoolInfo* poolInfo = memMgr.getRequestPoolInfo(poolIndex);
        uint8_t* userptr = poolInfo->getUserptr() + offset;
        memInfo = memMgr.createRequestMemoryInfo(userptr, length);
        poolInfo->addMemInfo(memInfo);

        // only output need sync?
        if (lifetime == OperandLifeTime::MODEL_OUTPUT)
        {
            memInfo->setNeedSync();
        }
    }

    return true;
}

void GlesOperand::restore(const Operand& from)
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

bool GlesOperand::set(const Operand& from, uint8_t* vp, uint32_t index)
{
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
            numberOfUsesLeft = from.numberOfConsumers;
            length = getElementCount() * getBasicTypeSize();
            ASSERT(offset == 0);
            break;
        case OperandLifeTime::CONSTANT_COPY:
            valPtr = vp;
            break;
        case OperandLifeTime::CONSTANT_REFERENCE:
        {
            poolIndex = from.location.poolIndex;
            GlesPoolInfo* poolInfo = memMgr.getModelPoolInfo(poolIndex);
            uint8_t* userptr = poolInfo->getUserptr() + offset;
            memInfo = memMgr.createModelMemoryInfo(userptr, length);
            poolInfo->addMemInfo(memInfo);
            break;
        }
        case OperandLifeTime::MODEL_INPUT:
        case OperandLifeTime::MODEL_OUTPUT:
            // do nothing here
            break;
        case OperandLifeTime::NO_VALUE:
            ASSERT(!"to know what's this");
            break;
        default:
            NOT_REACH_HERE;
            break;
    }

    return true;
}

size_t GlesOperand::getBasicTypeSize()
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

uint32_t GlesOperand::getElementCount()
{
    uint32_t count = 1;
    for (auto d : dimensions)
    {
        count *= d;
    }
    return count;
}

uint32_t GlesOperand::getElementCount(int32_t axis)
{
    ASSERT(axis >= 0);
    ASSERT(axis < (int32_t)dimensions.size());
    uint32_t count = 1;
    for (int i = axis; i < (int32_t)dimensions.size(); i++)
    {
        count *= dimensions[i];
    }
    return count;
}

GLuint GlesOperand::getSSbo()
{
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
    return memInfo->getSSbo();
}

void GlesOperand::shareGpuStorage(GlesOperand& from)
{
    // assure the storage allocation of from
    from.getSSbo();

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

void GlesOperand::retrieveData()
{
    if (memInfo != NULL)
    {
        GLuint ssbo = memInfo->getSSbo();
        ASSERT(ssbo != 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        uint8_t* p = (uint8_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, length, GL_MAP_READ_BIT);
        UNUSED(p);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
}

void GlesOperand::markOpFinished()
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

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
