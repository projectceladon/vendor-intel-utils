/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <set>

#include <aidl/android/hardware/thermal/BnThermal.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace thermal {
namespace impl {
namespace example {
struct CallbackSetting {
    CallbackSetting(std::shared_ptr<IThermalChangedCallback> callback, bool is_filter_type,
                    TemperatureType type)
        : callback(std::move(callback)), is_filter_type(is_filter_type), type(type) {}
    std::shared_ptr<IThermalChangedCallback> callback;
    bool is_filter_type;
    TemperatureType type;
};
class Thermal : public BnThermal {
  public:
    Thermal();
    void CheckThermalServerity();

    ndk::ScopedAStatus getCoolingDevices(std::vector<CoolingDevice>* out_devices) override;
    ndk::ScopedAStatus fillCoolingDevices(std::vector<CoolingDevice>* out_devices);
    ndk::ScopedAStatus getCoolingDevicesWithType(CoolingType in_type,
                                                 std::vector<CoolingDevice>* out_devices) override;

    ndk::ScopedAStatus getTemperatures(std::vector<Temperature>* out_temperatures) override;
    ndk::ScopedAStatus fillTemperatures(std::vector<Temperature>* out_temperatures);
    ndk::ScopedAStatus getTemperaturesWithType(TemperatureType in_type,
                                               std::vector<Temperature>* out_temperatures) override;

    ndk::ScopedAStatus getTemperatureThresholds(
            std::vector<TemperatureThreshold>* out_temperatureThresholds) override;

    ndk::ScopedAStatus getTemperatureThresholdsWithType(
            TemperatureType in_type,
            std::vector<TemperatureThreshold>* out_temperatureThresholds) override;

    ndk::ScopedAStatus registerThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& in_callback) override;
    ndk::ScopedAStatus registerThermalChangedCallbackWithType(
            const std::shared_ptr<IThermalChangedCallback>& in_callback,
            TemperatureType in_type) override;

    ndk::ScopedAStatus unregisterThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& in_callback) override;

  private:
    std::mutex thermal_callback_mutex_;
    std::vector<std::shared_ptr<IThermalChangedCallback>> thermal_callbacks_;
    std::thread mCheckThread;
    int mNumCpu;
    std::vector<CallbackSetting> callbacks_;
};

}  // namespace example
}  // namespace impl
}  // namespace thermal
}  // namespace hardware
}  // namespace android
}  // namespace aidl
