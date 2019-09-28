#include <algorithm>
#include <memory.h>
#include <string.h>

#include <hidl/LegacySupport.h>
#include <thread>

#include "device.h"
#include "prepare_model.h"
#include "executor_manager.h"
#include "ValidateHal.h"

NAME_SPACE_BEGIN

using namespace android::nn;

Return<void> Device::getCapabilities(getCapabilities_cb cb)
{
    NN_GPU_ENTRY();

    V1_0::Capabilities capabilities;
    ExecutorManager::getCapabilities(capabilities);
    cb(ErrorStatus::NONE, capabilities);

    NN_GPU_EXIT();

    return Void();
}

Return<void> Device::getCapabilities_1_1(getCapabilities_1_1_cb cb)
{
    NN_GPU_ENTRY();

    V1_0::Capabilities capabilities;
    ExecutorManager::getCapabilities(capabilities);
    cb(ErrorStatus::NONE, convertToV1_1(capabilities));

    NN_GPU_EXIT();
    return Void();
}

Return<void> Device::getCapabilities_1_2(getCapabilities_1_2_cb cb)
{
    NN_GPU_ENTRY();

    V1_0::Capabilities capabilities;
    ExecutorManager::getCapabilities(capabilities);
    cb(ErrorStatus::NONE, convertToV1_2(capabilities));

    NN_GPU_EXIT();
    return Void();
}

Return<void> Device::getSupportedOperations(const V1_0::Model& model,
                                            getSupportedOperations_cb cb)
{
    UNUSED(model);
    UNUSED(cb);

    NN_GPU_ENTRY();
    NN_GPU_EXIT();

    return Void();
}

Return<void> Device::getSupportedOperations_1_1(const V1_1::Model& model,
                                                getSupportedOperations_1_1_cb cb)
{
    UNUSED(model);
    UNUSED(cb);

    NN_GPU_ENTRY();
    NN_GPU_EXIT();
    return Void();
}

Return<void> Device::getSupportedOperations_1_2(const V1_2::Model& model,
                                                getSupportedOperations_1_2_cb cb)
{
    NN_GPU_ENTRY();
    if (!validateModel(model))
    {
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }

    std::vector<bool> supported = ExecutorManager::getSupportedOperations(model);
    cb(ErrorStatus::NONE, supported);
    NN_GPU_EXIT();
    return Void();
}

Return<ErrorStatus> Device::prepareModel(const V1_0::Model& model,
                                         const sp<V1_0::IPreparedModelCallback>& callback)
{
    UNUSED(model);
    UNUSED(callback);

    NN_GPU_ENTRY();
    NN_GPU_EXIT();

    return ErrorStatus::NONE;
}

Return<ErrorStatus> Device::prepareModel_1_1(const V1_1::Model& model,
                                             ExecutionPreference preference,
                                             const sp<V1_0::IPreparedModelCallback>& callback)
{
    NN_GPU_ENTRY();

    if (callback.get() == nullptr)
    {
        LOGE("invalid callback passed to prepareModel");
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model) || !validateExecutionPreference(preference))
    {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    sp<PreparedModel> preparedModel = new PreparedModel(convertToV1_2(model));
    if (!preparedModel->initialize())
    {
       callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
       return ErrorStatus::INVALID_ARGUMENT;
    }
    callback->notify(ErrorStatus::NONE, preparedModel);

    NN_GPU_EXIT();
    return ErrorStatus::NONE;
}

Return<ErrorStatus> Device::prepareModel_1_2(const V1_2::Model& model,
                                             ExecutionPreference preference,
                                             const hidl_vec<hidl_handle>& modelCache,
                                             const hidl_vec<hidl_handle>& dataCache,
                                             const HidlToken& token,
                                             const sp<V1_2::IPreparedModelCallback>& callback)
{
    UNUSED(modelCache);
    UNUSED(dataCache);
    UNUSED(token);

    NN_GPU_ENTRY();

    if (callback.get() == nullptr)
    {
        LOGE("invalid callback passed to prepareModel");
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model) || !validateExecutionPreference(preference))
    {
        callback->notify_1_2(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    sp<PreparedModel> preparedModel = new PreparedModel(model);
    if (!preparedModel->initialize())
    {
       callback->notify_1_2(ErrorStatus::INVALID_ARGUMENT, nullptr);
       return ErrorStatus::INVALID_ARGUMENT;
    }
    callback->notify_1_2(ErrorStatus::NONE, preparedModel);

    NN_GPU_EXIT();
    return ErrorStatus::NONE;
}

Return<DeviceStatus> Device::getStatus()
{
    NN_GPU_CALL();
    return DeviceStatus::AVAILABLE;
}

Return<void> Device::getVersionString(getVersionString_cb cb)
{
    NN_GPU_CALL();
    cb(ErrorStatus::NONE, "for gpgpu");
    return Void();
}

Return<void> Device::getType(getType_cb cb)
{
    UNUSED(cb);

    NN_GPU_CALL();
    return Void();
}

Return<void> Device::getSupportedExtensions(getSupportedExtensions_cb cb)
{
    NN_GPU_CALL();
    cb(ErrorStatus::NONE, {/* No extensions. */});
    return Void();
}

Return<void> Device::getNumberOfCacheFilesNeeded(getNumberOfCacheFilesNeeded_cb cb)
{
    NN_GPU_CALL();
    cb(ErrorStatus::NONE, /*numModelCache=*/0, /*numDataCache=*/0);
    return Void();
}

Return<ErrorStatus> Device::prepareModelFromCache(const hidl_vec<hidl_handle>&,
                                                  const hidl_vec<hidl_handle>&,
                                                  const HidlToken&,
                                                  const sp<V1_2::IPreparedModelCallback>& callback)
{
    NN_GPU_CALL();
    callback->notify_1_2(ErrorStatus::GENERAL_FAILURE, nullptr);
    return ErrorStatus::GENERAL_FAILURE;
}

int Device::run()
{
    NN_GPU_CALL();
    if (!ExecutorManager::initPerProcess())
    {
        LOGE("Unable to do ExecutorManager::initPerProcess, service exited!");
        return 1;
    }

    android::hardware::configureRpcThreadpool(4, true);
    if (registerAsService(mName) != android::OK)
    {
        LOGE("Could not register service");
        return 1;
    }
    android::hardware::joinRpcThreadpool();

    LOGE("Service exited!");
    ExecutorManager::deinitPerProcess();

    return 1;
}

NAME_SPACE_STOP
