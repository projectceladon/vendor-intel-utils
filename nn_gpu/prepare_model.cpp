#include <algorithm>
#include <memory.h>
#include <string.h>

#include <hidl/LegacySupport.h>
#include <thread>

#include "prepare_model.h"
#include "executor_manager.h"
#include "ValidateHal.h"

NAME_SPACE_BEGIN

using namespace android::nn;

static const Timing kNoTiming = {.timeOnDevice = UINT64_MAX, .timeInDriver = UINT64_MAX};

auto now()
{
    return std::chrono::steady_clock::now();
};

PreparedModel::PreparedModel(const Model& model)
      : // Make a copy of the model, as we need to preserve it.
        mModel(model)
{
    NN_GPU_CALL();
    exec = ExecutorManager::createExecutor(mModel);
}

bool PreparedModel::initialize()
{
    NN_GPU_CALL();
    return exec->initPerModel();
}

void PreparedModel::asyncExecute_1_2(const Request& request,
                                     const sp<V1_2::IExecutionCallback>& callback)
{
    NN_GPU_CALL();
    exec->initPerExecThread();
    bool succ = exec->run(request);
    exec->deinitPerExecThread();
    if (succ)
    {
        callback->notify_1_2(ErrorStatus::NONE, {}, kNoTiming);
    }
    else
    {
        callback->notify_1_2(ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
    }
}

void PreparedModel::asyncExecute(const Request& request,
                                 const sp<V1_0::IExecutionCallback>& callback)
{
    NN_GPU_CALL();

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
                                           const sp<V1_0::IExecutionCallback>& callback)
{
    NN_GPU_CALL();

    if (callback.get() == nullptr)
    {
        LOGE("invalid callback passed to execute");
        return ErrorStatus::INVALID_ARGUMENT;
    }

    if (!validateRequest(request, mModel))
    {
        callback->notify(ErrorStatus::INVALID_ARGUMENT);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    execThreads.push_back(std::thread([this, request, callback]{ asyncExecute(request, callback); }));

    return ErrorStatus::NONE;
}

Return<ErrorStatus> PreparedModel::execute_1_2(const Request& request,
                                               MeasureTiming measure,
                                               const sp<V1_2::IExecutionCallback>& callback)
{
    NN_GPU_CALL();

    time_point driverStart;
    if (measure == MeasureTiming::YES)
    {
        driverStart = now();
    }

    if (callback.get() == nullptr)
    {
        LOGE("invalid callback passed to execute");
        return ErrorStatus::INVALID_ARGUMENT;
    }

    if (!validateRequest(request, mModel))
    {
        callback->notify_1_2(ErrorStatus::INVALID_ARGUMENT, {}, kNoTiming);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    execThreads.push_back(std::thread([this, request, callback]{ asyncExecute(request, callback); }));

    return ErrorStatus::NONE;
}

Return<void> PreparedModel::executeSynchronously(const Request& request,
                                                 MeasureTiming measure,
                                                 executeSynchronously_cb cb)
{
    NN_GPU_CALL();
    time_point driverStart;

    if (measure == MeasureTiming::YES)
    {
        driverStart = now();
    }

    if (!validateRequest(request, mModel))
    {
        cb(ErrorStatus::INVALID_ARGUMENT, {}, kNoTiming);
        return Void();
    }

    exec->initPerExecThread();
    bool succ = exec->run(request);
    exec->deinitPerExecThread();

    if (succ)
    {
        cb(ErrorStatus::NONE, {}, kNoTiming);
    }
    else
    {
        cb(ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
    }

    return Void();
}

Return<void> PreparedModel::configureExecutionBurst(const sp<V1_2::IBurstCallback>& callback,
                                                    const MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
                                                    const MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
                                                    configureExecutionBurst_cb cb)
{
    NN_GPU_CALL();

    UNUSED(callback);
    UNUSED(requestChannel);
    UNUSED(resultChannel);
    UNUSED(cb);

    return Void();
}

PreparedModel::~PreparedModel()
{
    NN_GPU_CALL();
    for (auto& th : execThreads) th.join();
    exec->deinitPerModel();
}

NAME_SPACE_STOP
