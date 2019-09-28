#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_DEVICE_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_DEVICE_H

#include "hal_types.h"

NAME_SPACE_BEGIN

using ::android::hardware::MQDescriptorSync;
// ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN = 32
using HidlToken = hidl_array<uint8_t, 32>;

class Device : public IDevice {
public:
    Device(const char* name) : mName(name) {}
    virtual ~Device() override {}
    virtual Return<void> getCapabilities(getCapabilities_cb _hidl_cb) override;
    virtual Return<void> getCapabilities_1_1(getCapabilities_1_1_cb cb) override;
    virtual Return<void> getCapabilities_1_2(getCapabilities_1_2_cb cb) override;
    virtual Return<void> getSupportedOperations(const V1_0::Model& model, getSupportedOperations_cb cb) override;
    virtual Return<void> getSupportedOperations_1_1(const V1_1::Model& model,
                                                    getSupportedOperations_1_1_cb cb) override;
    virtual Return<void> getSupportedOperations_1_2(const V1_2::Model& model,
                                                    getSupportedOperations_1_2_cb cb) override;
    virtual Return<ErrorStatus> prepareModel(const V1_0::Model& model,
                                             const sp<V1_0::IPreparedModelCallback>& callback) override;
    virtual Return<ErrorStatus> prepareModel_1_1(const V1_1::Model& model, ExecutionPreference preference,
                                                 const sp<V1_0::IPreparedModelCallback>& callback) override;
    virtual Return<ErrorStatus> prepareModel_1_2(const V1_2::Model& model, ExecutionPreference preference,
                                                 const hidl_vec<hidl_handle>& modelCache,
                                                 const hidl_vec<hidl_handle>& dataCache,
                                                 const HidlToken& token,
                                                 const sp<V1_2::IPreparedModelCallback>& callback) override;
    virtual Return<DeviceStatus> getStatus() override;

    virtual Return<void> getVersionString(getVersionString_cb cb) override;
    virtual Return<void> getType(getType_cb cb) override;
    virtual Return<void> getSupportedExtensions(getSupportedExtensions_cb cb) override;
    virtual Return<void> getNumberOfCacheFilesNeeded(getNumberOfCacheFilesNeeded_cb cb) override;
    virtual Return<ErrorStatus> prepareModelFromCache(const hidl_vec<hidl_handle>&,
                                                      const hidl_vec<hidl_handle>&,
                                                      const HidlToken&,
                                                      const sp<V1_2::IPreparedModelCallback>& callback) override;

    // Starts and runs the driver service.  Typically called from main().
    // This will return only once the service shuts down.
    int run();
protected:
    std::string mName;
};

NAME_SPACE_STOP

#endif
