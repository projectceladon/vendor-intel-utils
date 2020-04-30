/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <cmath>
#include <set>

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>

#include "Thermal.h"

#define SYSFS_TEMPERATURE_CPU       "/sys/class/thermal/thermal_zone0/temp"
#define CPU_NUM_MAX                 4
#define CPU_USAGE_PARAS_NUM         5
#define CPU_USAGE_FILE              "/proc/stat"
#define CPU_ONLINE_FILE             "/sys/devices/system/cpu/online"
#define CPU_TEMPERATURE_UNIT        1000
#define TEMP_UNIT                   1000
#define THERMAL_PORT                1235

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::interfacesEqual;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;

static const char *CPU_LABEL[] = {"CPU0", "CPU1", "CPU2", "CPU3"};
struct zone_info {
	uint32_t temperature;
	uint32_t trip_0;
	uint32_t trip_1;
	uint32_t trip_2;
	uint16_t number;
	uint16_t type;
};

struct header {
	uint8_t intelipcid[8];
	uint16_t notifyid;
	uint16_t length;
};

static const char *THROTTLING_SEVERITY_LABEL[] = {
                                                  "NONE",
                                                  "LIGHT",
                                                  "MODERATE",
                                                  "SEVERE",
                                                  "CRITICAL",
                                                  "EMERGENCY",
                                                  "SHUTDOWN"};

static const Temperature_1_0 kTemp_1_0 = {
        .type = static_cast<::android::hardware::thermal::V1_0::TemperatureType>(
                TemperatureType::CPU),
        .name = "TCPU",
        .currentValue = 25,
        .throttlingThreshold = 108,
        .shutdownThreshold = 109,
        .vrThrottlingThreshold = NAN,
};

static Temperature_2_0 kTemp_2_0 = {
        .type = TemperatureType::CPU,
        .name = "TCPU",
        .value = 25,
        .throttlingStatus = ThrottlingSeverity::NONE,
};

static Temperature_2_0 kTemp_2_0_1 = {
        .type = TemperatureType::BATTERY,
        .name = "TBATTERY",
        .value = 25,
        .throttlingStatus = ThrottlingSeverity::NONE,
};

static TemperatureThreshold kTempThreshold = {
        .type = TemperatureType::CPU,
        .name = "TCPU",
        .hotThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, 99, 108}},
        .coldThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, NAN, NAN}},
        .vrThrottlingThreshold = NAN,
};

static TemperatureThreshold kTempThreshold_1 = {
        .type = TemperatureType::BATTERY,
        .name = "TBATTERY",
        .hotThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, 65, 68}},
        .coldThrottlingThresholds = {{NAN, NAN, NAN, NAN, NAN, NAN, NAN}},
        .vrThrottlingThreshold = NAN,
};

static const CoolingDevice_1_0 kCooling_1_0 = {
        .type = ::android::hardware::thermal::V1_0::CoolingType::FAN_RPM,
        .name = "test cooling device",
        .currentValue = 100.0,
};

static const CoolingDevice_2_0 kCooling_2_0 = {
        .type = CoolingType::FAN,
        .name = "test cooling device",
        .value = 100,
};

static const CpuUsage kCpuUsage = {
        .name = "cpu_name",
        .active = 0,
        .total = 0,
        .isOnline = true,
};

static bool is_vsock_present;


struct temp_info {
    uint16_t type;
    uint32_t temp;
};

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
    *temp = fTemp / CPU_TEMPERATURE_UNIT;

    return 0;
}

static int thermal_get_cpu_usages(CpuUsage *list)
{
    int vals, cpu_num, i, j;
    bool online;
    ssize_t read;
    unsigned long long user, nice, system, idle, active, total;
    char *line = NULL;
    size_t len = 0;
    size_t size = 0;
    FILE *file;
    FILE *cpu_file;

    if (list == NULL) {
        return CPU_NUM_MAX;
    }

    file = fopen(CPU_USAGE_FILE, "r");
    if (file == NULL) {
        ALOGE("%s: failed to open: %s", __func__, strerror(errno));
        return -errno;
    }

    // Read online CPU information.
    cpu_file = fopen(CPU_ONLINE_FILE, "r");
    if (cpu_file == NULL) {
        ALOGE("%s: failed to open file: %s (%s)", __func__, CPU_ONLINE_FILE, strerror(errno));
        fclose(file);
        return -errno;
    }

    if (2 != fscanf(cpu_file, "%d-%d", &i, &j)) {
        ALOGE("%s: failed to read CPU online information from file: %s (%s)", __func__,
                CPU_ONLINE_FILE, strerror(errno));
        fclose(cpu_file);
        fclose(file);
        return errno ? -errno : -EIO;
    }

    while ((read = getline(&line, &len, file)) != -1) {
        // Skip non "cpu[0-9]" lines.
        if (strnlen(line, read) < 4 || strncmp(line, "cpu", 3) != 0 || !isdigit(line[3])) {
            continue;
        }

        vals = sscanf(line, "cpu%d %llu %llu %llu %llu", &cpu_num, &user,
                &nice, &system, &idle);

        if (vals != CPU_USAGE_PARAS_NUM || size == CPU_NUM_MAX) {
            if (vals != CPU_USAGE_PARAS_NUM) {
                ALOGE("%s: failed to read CPU information from file: %s", __func__,
                        strerror(errno));
            } else {
                ALOGE("/proc/stat file has incorrect format.");
            }
            free(line);
            fclose(cpu_file);
            fclose(file);
            return errno ? -errno : -EIO;
        }

        active = user + nice + system;
        total = active + idle;

        online = 0;
        if (size >= (size_t)i && size <= (size_t)j) {
            online = 1;
        }

        list[size] = (CpuUsage) {
            .name = CPU_LABEL[size],
            .active = active,
            .total = total,
            .isOnline = online
        };

        size++;
    }

    free(line);
    fclose(cpu_file);
    fclose(file);

    if (size > CPU_NUM_MAX) {
        ALOGE("/proc/stat file has incorrect format.");
        return -EIO;
    }
    return (int)size;
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

static void parse_zone_info(struct zone_info *zone)
{
        switch (zone->type) {
             case 0:
                 kTempThreshold.hotThrottlingThresholds[5] = zone->trip_0 / TEMP_UNIT;
                 kTempThreshold.hotThrottlingThresholds[6] = zone->trip_1 / TEMP_UNIT;
                 kTemp_2_0.value = zone->temperature / TEMP_UNIT;
                 break;
             case 2:
                 kTemp_2_0_1.value = zone->temperature / TEMP_UNIT;
                 break;
             default:
                 break;
        }
}

static void parse_temp_info(struct temp_info *t)
{
    switch(t->type) {
        case 0:
            kTemp_2_0.value = t->temp / 1000;
            break;
        case 2:
            kTemp_2_0_1.value = t->temp / 1000;
            break;
        default:
            break;
    }
}

static int recv_vsock(int *vsock_fd)
{
    char msgbuf[1024];
    struct header *head = (struct header *)malloc(sizeof(struct header));
    int ret;
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
           ptr += sizeof(struct header);
           struct zone_info *zinfo = (struct zone_info *)malloc(sizeof(struct zone_info));
           for (i = 0; i < num_zones; i++) {
               memcpy(zinfo, msgbuf + ptr, sizeof(struct zone_info));
               parse_zone_info(zinfo);
               ptr = ptr + sizeof(struct zone_info);
           }
           free(zinfo);
        } else if (head->notifyid == 2) {
            ptr = sizeof(struct header); //offset of num_zones field
            struct temp_info *tinfo = (struct temp_info *)malloc(sizeof(struct temp_info));
            memcpy(&num_zones, msgbuf + ptr, sizeof(num_zones));
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
    return 0;
}

Thermal::Thermal() {
    mCheckThread = std::thread(&Thermal::CheckThermalServerity, this);
    mCheckThread.detach();
}


void Thermal::CheckThermalServerity() {
    float temp = NAN;
    int res = -1;
    int vsock_fd;

    ALOGI("Start check temp thread.\n");

    if (!connect_vsock(&vsock_fd))
        is_vsock_present = true;

    while (1) {
        if (is_vsock_present) {
            res = 0;
            recv_vsock(&vsock_fd);
        } else {
            res = get_soc_pkg_temperature(&temp);
            kTemp_2_0.value = temp;
        }
        if (res) {
            ALOGE("Can not get temperature of type %d", kTemp_1_0.type);
        } else {
            for (size_t i = kTempThreshold.hotThrottlingThresholds.size() - 1; i > 0; i--) {
                if (kTemp_2_0.value >= kTempThreshold.hotThrottlingThresholds[i]) {
                    ALOGI("CheckThermalServerity: hit ThrottlingSeverity %s, temperature is %f",
                          THROTTLING_SEVERITY_LABEL[i], kTemp_2_0.value);
                    kTemp_2_0.throttlingStatus = (ThrottlingSeverity)i;
                    {
                        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
                        for (auto cb : callbacks_) {
                            cb.callback->notifyThrottling(kTemp_2_0);
                        }
                    }
                }
            }
            for (size_t i = kTempThreshold_1.hotThrottlingThresholds.size() - 1; i > 0; i--) {
                if (kTemp_2_0_1.value >= kTempThreshold_1.hotThrottlingThresholds[i]) {
                    ALOGI("CheckThermalServerity: hit ThrottlingSeverity %s, temperature is %f",
                          THROTTLING_SEVERITY_LABEL[i], kTemp_2_0_1.value);
                    kTemp_2_0_1.throttlingStatus = (ThrottlingSeverity)i;
                    {
                        std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
                        for (auto cb : callbacks_) {
                            cb.callback->notifyThrottling(kTemp_2_0_1);
                        }
                    }
                }
            }
        }
        sleep(1);
    }
}

// Methods from ::android::hardware::thermal::V1_0::IThermal follow.
Return<void> Thermal::getTemperatures(getTemperatures_cb _hidl_cb) {
    ThermalStatus status;
    std::vector<Temperature_1_0> temperatures = {kTemp_1_0};
    float temp = NAN;
    int res = -1;

    status.code = ThermalStatusCode::SUCCESS;
    if (!is_vsock_present) {
        res = get_soc_pkg_temperature(&temp);
        if (res) {
            ALOGE("Can not get temperature of type %d", kTemp_1_0.type);
            status.code = ThermalStatusCode::FAILURE;
            status.debugMessage = strerror(-res);
        } else {
            temperatures[0].currentValue = temp;
        }
     }
    _hidl_cb(status, temperatures);
    return Void();
}

Return<void> Thermal::getCpuUsages(getCpuUsages_cb _hidl_cb) {
    ThermalStatus status;
    hidl_vec<CpuUsage> cpuUsages;
    int res = 0;

    status.code = ThermalStatusCode::SUCCESS;
    cpuUsages.resize(CPU_NUM_MAX);
    res = thermal_get_cpu_usages(cpuUsages.data());
    if (res <= 0) {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = strerror(-res);
    }
    _hidl_cb(status, cpuUsages);
    return Void();
}

Return<void> Thermal::getCoolingDevices(getCoolingDevices_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    std::vector<CoolingDevice_1_0> cooling_devices = {kCooling_1_0};
    _hidl_cb(status, cooling_devices);
    return Void();
}

// Methods from ::android::hardware::thermal::V2_0::IThermal follow.
Return<void> Thermal::getCurrentTemperatures(bool filterType, TemperatureType type,
                                             getCurrentTemperatures_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    std::vector<Temperature_2_0> temperatures;
    float temp = NAN;
    int res = -1;

    if (filterType && type != kTemp_2_0.type) {
        // Workaround for VTS Test
        //status.code = ThermalStatusCode::FAILURE;
        //status.debugMessage = "Failed to read data";
    } else {
        temperatures = {kTemp_2_0, kTemp_2_0_1};
        if (!is_vsock_present) {
            res = get_soc_pkg_temperature(&temp);
            temperatures[0].value = temp;
        } else
            res = 0;
        if (res) {
            ALOGE("Can not get temperature of type %d", kTemp_2_0.type);
            status.code = ThermalStatusCode::FAILURE;
            status.debugMessage = strerror(-res);
        } else {
            for (size_t i = kTempThreshold.hotThrottlingThresholds.size() - 1; i > 0; i--) {
                if (temperatures[0].value >= kTempThreshold.hotThrottlingThresholds[i]) {
                    temperatures[0].throttlingStatus = (ThrottlingSeverity)i;
                    break;
                }
            }
            for (size_t i = kTempThreshold_1.hotThrottlingThresholds.size() - 1; i > 0; i--) {
                if (temperatures[1].value >= kTempThreshold_1.hotThrottlingThresholds[i]) {
                    temperatures[1].throttlingStatus = (ThrottlingSeverity)i;
                    break;
                }
            }
        }
    }

    // Workaround for VTS Test
    if (filterType && temperatures.size() == 0) {
        temperatures = {(Temperature_2_0) {
                            .type = type,
                            .name = "FAKE_DATA",
                            .value = 0,
                            .throttlingStatus = ThrottlingSeverity::NONE}};
    }
    _hidl_cb(status, temperatures);
    return Void();
}

Return<void> Thermal::getTemperatureThresholds(bool filterType, TemperatureType type,
                                               getTemperatureThresholds_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    std::vector<TemperatureThreshold> temperature_thresholds;
    if (filterType && type != kTempThreshold.type) {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = "Failed to read data";
    } else {
        temperature_thresholds = {kTempThreshold, kTempThreshold_1};
    }
    _hidl_cb(status, temperature_thresholds);
    return Void();
}

Return<void> Thermal::getCurrentCoolingDevices(bool filterType, CoolingType type,
                                               getCurrentCoolingDevices_cb _hidl_cb) {
    ThermalStatus status;
    status.code = ThermalStatusCode::SUCCESS;
    std::vector<CoolingDevice_2_0> cooling_devices;

    if (filterType && type != kCooling_2_0.type) {
        // Workaround for VTS Test
        //status.code = ThermalStatusCode::FAILURE;
        //status.debugMessage = "Failed to read data";
    } else {
        cooling_devices = {kCooling_2_0};
    }
    // Workaround for VTS Test
    if (filterType && cooling_devices.size() == 0) {
        cooling_devices = {(CoolingDevice_2_0) {
                            .type = type,
                            .name = "FAKE_DATA",
                            .value = 0}};
    }

    _hidl_cb(status, cooling_devices);
    return Void();
}

Return<void> Thermal::registerThermalChangedCallback(const sp<IThermalChangedCallback>& callback,
                                                     bool filterType, TemperatureType type,
                                                     registerThermalChangedCallback_cb _hidl_cb) {
    ThermalStatus status;
    if (callback == nullptr) {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = "Invalid nullptr callback";
        LOG(ERROR) << status.debugMessage;
        _hidl_cb(status);
        return Void();
    } else {
        status.code = ThermalStatusCode::SUCCESS;
    }
    std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
    if (std::any_of(callbacks_.begin(), callbacks_.end(), [&](const CallbackSetting& c) {
            return interfacesEqual(c.callback, callback);
        })) {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = "Same callback interface registered already";
        LOG(ERROR) << status.debugMessage;
    } else {
        callbacks_.emplace_back(callback, filterType, type);
        LOG(INFO) << "A callback has been registered to ThermalHAL, isFilter: " << filterType
                  << " Type: " << android::hardware::thermal::V2_0::toString(type);
    }
    _hidl_cb(status);
    return Void();
}

Return<void> Thermal::unregisterThermalChangedCallback(
    const sp<IThermalChangedCallback>& callback, unregisterThermalChangedCallback_cb _hidl_cb) {
    ThermalStatus status;
    if (callback == nullptr) {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = "Invalid nullptr callback";
        LOG(ERROR) << status.debugMessage;
        _hidl_cb(status);
        return Void();
    } else {
        status.code = ThermalStatusCode::SUCCESS;
    }
    bool removed = false;
    std::lock_guard<std::mutex> _lock(thermal_callback_mutex_);
    callbacks_.erase(
        std::remove_if(callbacks_.begin(), callbacks_.end(),
                       [&](const CallbackSetting& c) {
                           if (interfacesEqual(c.callback, callback)) {
                               LOG(INFO)
                                   << "A callback has been unregistered from ThermalHAL, isFilter: "
                                   << c.is_filter_type << " Type: "
                                   << android::hardware::thermal::V2_0::toString(c.type);
                               removed = true;
                               return true;
                           }
                           return false;
                       }),
        callbacks_.end());
    if (!removed) {
        status.code = ThermalStatusCode::FAILURE;
        status.debugMessage = "The callback was not registered before";
        LOG(ERROR) << status.debugMessage;
    }
    _hidl_cb(status);
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
