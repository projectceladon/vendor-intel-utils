#include "device.h"

using android::hardware::neuralnetworks::V1_2::implementation::Device;
using android::sp;

int main()
{
    sp<Device> device(new Device("gpgpu"));
    return device->run();
}
