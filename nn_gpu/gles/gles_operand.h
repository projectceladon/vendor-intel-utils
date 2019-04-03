#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_OPERAND_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_OPERAND_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

#include "base_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class GlesMemoryManager;
class GlesPoolInfo;
class GlesMemoryInfo;

class GlesOperand
{
public:
    GlesOperand(GlesMemoryManager& mgr) :
            memMgr(mgr), memInfo(nullptr), poolIndex(0),
            offset(0), length(0), valPtr(nullptr), numberOfUsesLeft(0), operandIndex(-1) {}
    ~GlesOperand() {}

    bool set(const Operand& from, uint8_t* vp, uint32_t index);
    void restore(const Operand& from);
    bool setArg(const RequestArgument& from);
    GLuint getSSbo();

    GLuint getTexture();
    void shareGpuStorage(GlesOperand& shareFrom);

    void markOpFinished();
    void retrieveData();
    uint32_t getElementCount();
    uint32_t getElementCount(int32_t axis);

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
            ALOGW("Invalid dimension idx: %d\n", idx);
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

private:
    size_t getBasicTypeSize();

    GlesMemoryManager& memMgr;
    GlesMemoryInfo* memInfo;

    uint32_t poolIndex;
    size_t offset;                  //in bytes
    size_t length;                  //in bytes
    uint8_t* valPtr;                //for operand value not provided by pool
    OperandType type;
    std::vector<uint32_t> dimensions;
    OperandLifeTime lifetime;
    size_t numberOfUsesLeft;
    float scale;
    int32_t zeroPoint;

    // just for debug
    uint32_t operandIndex;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
