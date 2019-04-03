#include "executor_manager.h"
#include "gles/gles_cs_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

ExecutorManager::ExecutorType ExecutorManager::type = ExecutorManager::ET_GLES_CS;

bool ExecutorManager::initPerProcess()
{

    //might check setting to change type

    if (type == ET_GLES_CS)
    {
        return GlesCsExecutor::initPerProcess();
    }

    return false;
}

void ExecutorManager::deinitPerProcess()
{
    GlesCsExecutor::deinitPerProcess();
}

void ExecutorManager::getCapabilities(Capabilities &cap)
{
    GlesCsExecutor::getCapabilities(cap);
}

std::vector<bool> ExecutorManager::getSupportedOperations(const Model& model)
{
    return GlesCsExecutor::getSupportedOperations(model);
}

BaseExecutor* ExecutorManager::createExecutor(const Model& model)
{
    return new GlesCsExecutor(model);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android