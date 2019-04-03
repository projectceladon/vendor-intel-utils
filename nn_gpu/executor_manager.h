#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_EXECUTOR_MANAGER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_EXECUTOR_MANAGER_H

#include <vector>

#include "base_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class ExecutorManager
{
public:
    static bool initPerProcess();
    static void deinitPerProcess();
    static void getCapabilities(Capabilities& cap);
    static std::vector<bool> getSupportedOperations(const Model& model);
    static BaseExecutor* createExecutor(const Model& model);
private:
    enum ExecutorType
    {
        ET_GLES_CS,
    };
    static ExecutorType type;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
