#include <algorithm>
#include <memory.h>
#include <string.h>

#include <android/log.h>
#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

#include "prepare_model.h"
#include "executor_manager.h"
#include "validate.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

PreparedModel::PreparedModel(const Model& model)
      : // Make a copy of the model, as we need to preserve it.
        mModel(model)
{
    exec = ExecutorManager::createExecutor(mModel);
}

bool PreparedModel::initialize()
{
    exec->initPerModel();
    return true;
}

void PreparedModel::asyncExecute(const Request& request,
                                       const sp<IExecutionCallback>& callback)
{
    ALOGV("PreparedModel::asyncExecute");
    exec->initPerExecThread();
    bool succ = exec->run(request);
    exec->deinitPerExecThread();
    if (succ)
    {
        callback->notify(ErrorStatus::NONE);
    }
    else
    {
        callback->notify(ErrorStatus::GENERAL_FAILURE);
    }
}

Return<ErrorStatus> PreparedModel::execute(const Request& request,
                                                 const sp<IExecutionCallback>& callback)
{
    ALOGV("PreparedModel::execute");
    if (callback.get() == nullptr)
    {
        ALOGE("invalid callback passed to execute");
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateRequest(request, mModel)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    execThreads.push_back(std::thread([this, request, callback]{ asyncExecute(request, callback); }));

    return ErrorStatus::NONE;
}

PreparedModel::~PreparedModel()
{
    for (auto& th : execThreads) th.join();
    exec->deinitPerModel();
}

}// namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
