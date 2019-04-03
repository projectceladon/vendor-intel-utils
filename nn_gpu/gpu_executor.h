#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GPU_EXECUTOR_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GPU_EXECUTOR_H

#include "base_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class GpuExecutor : public BaseExecutor
{
public:
    GpuExecutor(const Model& model) : BaseExecutor(model) {}
    ~GpuExecutor() override {}
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
