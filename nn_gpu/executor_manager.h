#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_EXECUTOR_MANAGER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_EXECUTOR_MANAGER_H

#include <vector>

#include "base_executor.h"

NAME_SPACE_BEGIN

class ExecutorManager
{
public:
    static bool initPerProcess();
    static void deinitPerProcess();
    static void getCapabilities(V1_0::Capabilities& cap);
    static std::vector<bool> getSupportedOperations(const Model& model);
    static BaseExecutor* createExecutor(const Model& model);
private:
    enum ExecutorType
    {
        ET_GLES_CS,
        ET_VK_CS,
    };
    static ExecutorType type;
};

NAME_SPACE_STOP

#endif
