#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_DEVICE_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_DEVICE_H

#include "hal_types.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class Device : public IDevice {
public:
    Device(const char* name) : mName(name) {}
    ~Device() override {}
    Return<void> getCapabilities(getCapabilities_cb _hidl_cb) override;
    Return<void> getSupportedOperations(const Model& model, getSupportedOperations_cb cb) override;

    Return<ErrorStatus> prepareModel(const Model& model,
                                     const sp<IPreparedModelCallback>& callback) override;
    Return<DeviceStatus> getStatus() override;

    // Starts and runs the driver service.  Typically called from main().
    // This will return only once the service shuts down.
    int run();
protected:
    std::string mName;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif