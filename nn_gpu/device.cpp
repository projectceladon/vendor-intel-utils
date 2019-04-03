#include <algorithm>
#include <memory.h>
#include <string.h>

#include <android/log.h>
#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

#include "device.h"
#include "prepare_model.h"
#include "executor_manager.h"
#include "validate.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

Return<void> Device::getCapabilities(getCapabilities_cb cb)
{
    ALOGV("Device::getCapabilities");
    Capabilities capabilities;
    ExecutorManager::getCapabilities(capabilities);
    cb(ErrorStatus::NONE, capabilities);
    return Void();
}

Return<void> Device::getSupportedOperations(const Model& model,
                                                     getSupportedOperations_cb cb)
{
    ALOGV("Device::getSupportedOperations");
    if (!validateModel(model))
    {
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }
    std::vector<bool> supported = ExecutorManager::getSupportedOperations(model);
    cb(ErrorStatus::NONE, supported);
    return Void();
}

Return<ErrorStatus> Device::prepareModel(const Model& model,
                                               const sp<IPreparedModelCallback>& callback)
{
    ALOGV("Device::prepareModel");
    if (callback.get() == nullptr) {
        LOG(ERROR) << "invalid callback passed to prepareModel";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }
    sp<PreparedModel> preparedModel = new PreparedModel(model);
    if (!preparedModel->initialize())
    {
       callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
       return ErrorStatus::INVALID_ARGUMENT;
    }
    callback->notify(ErrorStatus::NONE, preparedModel);
    return ErrorStatus::NONE;
}

Return<DeviceStatus> Device::getStatus()
{
    ALOGV("Device::getStatus");
    return DeviceStatus::AVAILABLE;
}

int Device::run()
{
    ALOGV("Device::run");
    if (!ExecutorManager::initPerProcess())
    {
        ALOGE("Unable to do ExecutorManager::initPerProcess, service exited!");
        return 1;
    }

    android::hardware::configureRpcThreadpool(4, true);
    if (registerAsService(mName) != android::OK)
    {
        ALOGE("Could not register service");
        return 1;
    }
    android::hardware::joinRpcThreadpool();

    ALOGE("Service exited!");
    ExecutorManager::deinitPerProcess();
    return 1;
}

}// namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
