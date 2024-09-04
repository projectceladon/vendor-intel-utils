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

#define LOG_TAG "thermal_service_intel"

#include "Thermal.h"

#include <cmath>
#include <android-base/logging.h>
#include <log/log.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <linux/vm_sockets.h>

#define SYSFS_TEMPERATURE_CPU       "/sys/class/thermal/thermal_zone0/temp"
#define CPU_NUM_MAX                 Thermal::getNumCpu()
#define CPU_USAGE_PARAS_NUM         5
#define CPU_USAGE_FILE              "/proc/stat"
#define CPU_ONLINE_FILE             "/sys/devices/system/cpu/online"
#define TEMP_UNIT                   1000.0
#define THERMAL_PORT                14096
#define MAX_ZONES                   40

namespace aidl::android::hardware::thermal::impl::example {

using ndk::ScopedAStatus;

namespace {

bool interfacesEqual(const std::shared_ptr<::ndk::ICInterface>& left,
                     const std::shared_ptr<::ndk::ICInterface>& right) {
    if (left == nullptr || right == nullptr || !left->isRemote() || !right->isRemote()) {
        return left == right;
    }
    return left->asBinder() == right->asBinder();
}

}  // namespace

static const char *THROTTLING_SEVERITY_LABEL[] = {
                                                  "NONE",
                                                  "LIGHT",
                                                  "MODERATE",
                                                  "SEVERE",
                                                  "CRITICAL",
                                                  "EMERGENCY",
                                                  "SHUTDOWN"};

static bool is_vsock_present;

struct zone_info {
	uint32_t temperature;
	uint32_t trip_0;
	uint32_t trip_1;
	uint32_t trip_2;
	uint16_t number;
	int16_t type;
};

struct header {
	uint8_t intelipcid[9];
	uint16_t notifyid;
	uint16_t length;
};

struct temp_info {
    int16_t type;
    uint32_t temp;
};

Temperature kTemp_1_0,kTemp_2_0;
TemperatureThreshold kTempThreshold;


static void parse_temp_info(struct temp_info *t)
{
    int temp = t->temp / TEMP_UNIT;
    switch(t->type) {
        case 0:
            kTemp_2_0.value = t->temp / TEMP_UNIT;
            break;
        default:
            break;
    }
}

static void parse_zone_info(struct zone_info *zone)
{
    int temp = zone->temperature;
    kTempThreshold.hotThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, 99, 108}};
         switch (zone->type) {
              case 0:
                    kTempThreshold.hotThrottlingThresholds[5] = zone->trip_0 / TEMP_UNIT;
                    kTempThreshold.hotThrottlingThresholds[6] = zone->trip_1 / TEMP_UNIT;
                    kTemp_2_0.value = zone->temperature / TEMP_UNIT;;
                    break;
              default:
                    break;
         }
}

static int connect_vsock(int *vsock_fd)
{
     struct sockaddr_vm sa = {
        .svm_family = AF_VSOCK,
        .svm_cid = VMADDR_CID_HOST,
        .svm_port = THERMAL_PORT,
    };
    *vsock_fd = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (*vsock_fd < 0) {
            ALOGI("Thermal HAL socket init failed\n");
            return -1;
    }

    if (connect(*vsock_fd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
            ALOGI("Thermal HAL connect failed\n");
            close(*vsock_fd);
            return -1;
    }
    return 0;
}

static int recv_vsock(int *vsock_fd)
{
    char msgbuf[1024];
    int ret;
    struct header *head = (struct header *)malloc(sizeof(struct header));
    if (!head)
        return -ENOMEM;
    memset(msgbuf, 0, sizeof(msgbuf));
    ret = recv(*vsock_fd, msgbuf, sizeof(msgbuf), MSG_DONTWAIT);
    if (ret < 0 && errno == EBADF) {
        if (connect_vsock(vsock_fd) == 0)
            ret = recv(*vsock_fd, msgbuf, sizeof(msgbuf), MSG_DONTWAIT);
    }
    if (ret > 0) {
        memcpy(head, msgbuf, sizeof(struct header));
        int num_zones = 0;
        int ptr = 0;
        int i;
        if (head->notifyid == 1) {
            num_zones = head->length / sizeof(struct zone_info);
            num_zones = num_zones < MAX_ZONES ? num_zones : MAX_ZONES;
            ptr += sizeof(struct header);
            struct zone_info *zinfo = (struct zone_info *)malloc(sizeof(struct zone_info));
            if (!zinfo) {
                free(head);
                return -ENOMEM;
            }
            for (i = 0; i < num_zones; i++) {
                memcpy(zinfo, msgbuf + ptr, sizeof(struct zone_info));
                parse_zone_info(zinfo);
                ptr = ptr + sizeof(struct zone_info);
            }
            free(zinfo);
        } else if (head->notifyid == 2) {
            ptr = sizeof(struct header); //offset of num_zones field
            struct temp_info *tinfo = (struct temp_info *)malloc(sizeof(struct temp_info));
            if (!tinfo) {
                free(head);
                return -ENOMEM;
            }
            memcpy(&num_zones, msgbuf + ptr, sizeof(num_zones));
            num_zones = num_zones < MAX_ZONES ? num_zones : MAX_ZONES;
            ptr += 4; //offset of first zone type info
            for (i = 0; i < num_zones; i++) {
                memcpy(&tinfo->type, msgbuf + ptr, sizeof(tinfo->type));
                memcpy(&tinfo->temp, msgbuf + ptr + sizeof(tinfo->type), sizeof(tinfo->temp));
                parse_temp_info(tinfo);
                ptr += sizeof(tinfo->type) + sizeof(tinfo->temp);
            }
            free(tinfo);
        }
    }
    free(head);
    return 0;
}

static int get_soc_pkg_temperature(float* temp)
{
    float fTemp = 0;
    int len = 0;
    FILE *file = NULL;

    file = fopen(SYSFS_TEMPERATURE_CPU, "r");

    if (file == NULL) {
        ALOGE("%s: failed to open file: %s", __func__, strerror(errno));
        return -errno;
    }

    len = fscanf(file, "%f", &fTemp);
    if (len < 0) {
        ALOGE("%s: failed to read file: %s", __func__, strerror(errno));
        fclose(file);
        return -errno;
    }

    fclose(file);
    *temp = fTemp / TEMP_UNIT;

    return 0;
}

Thermal::Thermal() {
    mCheckThread = std::thread(&Thermal::CheckThermalServerity, this);
    mCheckThread.detach();
    mNumCpu = get_nprocs();
}
void Thermal::CheckThermalServerity() {
    float temp = NAN;
    int res = -1;
    int vsock_fd;
    kTempThreshold.hotThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, 99, 108}};
    ALOGI("Start check temp thread.\n");

    if (!connect_vsock(&vsock_fd))
        is_vsock_present = true;

    while (1) {
        if (is_vsock_present) {
            res = 0;
            recv_vsock(&vsock_fd);
        }else {
            res = get_soc_pkg_temperature(&temp);
            kTemp_2_0.value = temp;
        }
        if (res) {
            ALOGE("Can not get temperature of type %d", kTemp_1_0.type);
        } else {
           ALOGI("Size of kTempThreshold.hotThrottlingThresholds=%f, kTemp_2_0.value=%f",kTempThreshold.hotThrottlingThresholds[5], kTemp_2_0.value);
            for (size_t i = kTempThreshold.hotThrottlingThresholds.size() - 1; i > 0; i--) {
                if (kTemp_2_0.value >= kTempThreshold.hotThrottlingThresholds[i]) {
                    kTemp_2_0.type = TemperatureType::CPU;
                    kTemp_2_0.name = "TCPU";
                    ALOGI("CheckThermalServerity: hit ThrottlingSeverity %s, temperature is %f",
                          THROTTLING_SEVERITY_LABEL[i], kTemp_2_0.value);
                    kTemp_2_0.throttlingStatus = (ThrottlingSeverity)i;
                    {
                        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
                        for (auto& cb : callbacks_) {
                            ::ndk::ScopedAStatus ret = cb.callback->notifyThrottling(kTemp_2_0);
                            if (!ret.isOk()) {
                                ALOGE("Thermal callback FAILURE");
                            }
                            else {
                                ALOGE("Thermal callback SUCCESS");
                            }
                        }
                    }
                }
            }
         }
        sleep(1);
    }
}


ScopedAStatus Thermal::getCoolingDevices(std::vector<CoolingDevice>* out_devices ) {
    LOG(VERBOSE) << __func__;
    fillCoolingDevices(out_devices);
    return ScopedAStatus::ok();
}

ScopedAStatus  Thermal::fillCoolingDevices(std::vector<CoolingDevice>*  out_devices) {
    std::vector<CoolingDevice> ret;

    *out_devices = ret;
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getCoolingDevicesWithType(CoolingType in_type,
                                                 std::vector<CoolingDevice>* /* out_devices */) {
    LOG(VERBOSE) << __func__ << " CoolingType: " << static_cast<int32_t>(in_type);
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatures(std::vector<Temperature>*  out_temperatures) {
    LOG(VERBOSE) << __func__;
    fillTemperatures(out_temperatures);
    return ScopedAStatus::ok();
}

ScopedAStatus  Thermal::fillTemperatures(std::vector<Temperature>*  out_temperatures) {
    *out_temperatures = {};
    std::vector<Temperature> ret;

    //CPU temperature
    kTemp_2_0.type = TemperatureType::CPU;
    kTemp_2_0.name = "TCPU";
    ret.emplace_back(kTemp_2_0);

    *out_temperatures = ret;
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperaturesWithType(TemperatureType in_type,
                                               std::vector<Temperature>* out_temperatures) {
    LOG(VERBOSE) << __func__ << " TemperatureType: " << static_cast<int32_t>(in_type);

    *out_temperatures={};

    if (in_type == TemperatureType::CPU) {
        kTemp_2_0.type = TemperatureType::CPU;
        kTemp_2_0.name = "TCPU";

        out_temperatures->emplace_back(kTemp_2_0);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatureThresholds(
        std::vector<TemperatureThreshold>* out_temperatureThresholds ) {
    LOG(VERBOSE) << __func__;
    *out_temperatureThresholds = {};
    std::vector<TemperatureThreshold> ret;

    kTempThreshold.type = TemperatureType::CPU;
    kTempThreshold.name = "TCPU";
    kTempThreshold.hotThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, 99, 108}};
    kTempThreshold.coldThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, NAN, NAN}};
    ret.emplace_back(kTempThreshold);

    *out_temperatureThresholds = ret;
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::getTemperatureThresholdsWithType(
        TemperatureType in_type,
        std::vector<TemperatureThreshold>* out_temperatureThresholds) {
    LOG(VERBOSE) << __func__ << " TemperatureType: " << static_cast<int32_t>(in_type);

    *out_temperatureThresholds = {};

    if (in_type == TemperatureType::CPU) {
        kTempThreshold.type = TemperatureType::CPU;
        kTempThreshold.name = "TCPU";
        kTempThreshold.hotThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, 99, 108}};
        kTempThreshold.coldThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, NAN, NAN}};
        out_temperatureThresholds->emplace_back(kTempThreshold);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& in_callback) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback;
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
        if (std::any_of(thermal_callbacks_.begin(), thermal_callbacks_.end(),
                        [&](const std::shared_ptr<IThermalChangedCallback>& c) {
                            return interfacesEqual(c, in_callback);
                        })) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback already registered");
        }
        thermal_callbacks_.push_back(in_callback);
    }

    std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
    if (std::any_of(callbacks_.begin(), callbacks_.end(), [&](const CallbackSetting &c) {
            return interfacesEqual(c.callback, in_callback);
        })) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Callback already registered");
    }
    callbacks_.emplace_back(in_callback,false,TemperatureType::UNKNOWN);


    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::registerThermalChangedCallbackWithType(
        const std::shared_ptr<IThermalChangedCallback>& in_callback, TemperatureType in_type) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback
                 << ", TemperatureType: " << static_cast<int32_t>(in_type);
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
        if (std::any_of(thermal_callbacks_.begin(), thermal_callbacks_.end(),
                        [&](const std::shared_ptr<IThermalChangedCallback>& c) {
                            return interfacesEqual(c, in_callback);
                        })) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback already registered");
        }
        thermal_callbacks_.push_back(in_callback);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus Thermal::unregisterThermalChangedCallback(
        const std::shared_ptr<IThermalChangedCallback>& in_callback) {
    LOG(VERBOSE) << __func__ << " IThermalChangedCallback: " << in_callback;
    if (in_callback == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid nullptr callback");
    }
    {
        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
        bool removed = false;
        thermal_callbacks_.erase(
                std::remove_if(thermal_callbacks_.begin(), thermal_callbacks_.end(),
                               [&](const std::shared_ptr<IThermalChangedCallback>& c) {
                                   if (interfacesEqual(c, in_callback)) {
                                       removed = true;
                                       return true;
                                   }
                                   return false;
                               }),
                thermal_callbacks_.end());
        if (!removed) {
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Callback wasn't registered");
        }
    }
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::thermal::impl::example
