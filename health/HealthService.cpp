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
#define LOG_TAG "android.hardware.health@2.0-service.intel"
#include <android-base/logging.h>

#include <healthd/healthd.h>
#include <health2/Health.h>
#include <health2/service.h>
#include <health2/powerSupplyType.h>
#include <hidl/HidlTransportSupport.h>

#include <utils/String8.h>
#include <cutils/klog.h>
#include <android-base/file.h>
#include <android-base/strings.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>


using android::hardware::health::V1_0::BatteryStatus;
using android::hardware::health::V1_0::BatteryHealth;
using namespace android;

#define POWER_SUPPLY_SUBSYSTEM "power_supply"
#define POWER_SUPPLY_SYSFS_PATH "/sys/class/" POWER_SUPPLY_SUBSYSTEM


unsigned int platformPowerSupplyType = BATTERY;

void healthd_board_init(struct healthd_config*)
{
    String8 path;

    DIR* dir = opendir(POWER_SUPPLY_SYSFS_PATH);
    if (dir == NULL) {
        KLOG_ERROR(LOG_TAG, "Could not open %s\n", POWER_SUPPLY_SYSFS_PATH);
        platformPowerSupplyType = CONSTANT_POWER;
        return;
    } else {
        struct dirent* entry;

        while ((entry = readdir(dir))) {
            const char* name = entry->d_name;

            if (!strcmp(name, ".") || !strcmp(name, "..")){
                platformPowerSupplyType = CONSTANT_POWER;
                continue;
            }else
                platformPowerSupplyType = BATTERY;
        }

    }
    closedir(dir);
}

int healthd_board_battery_update(struct android::BatteryProperties *props)
{

    if (platformPowerSupplyType == CONSTANT_POWER) {
        props->batteryStatus = android::BATTERY_STATUS_FULL;
        props->batteryHealth = android::BATTERY_HEALTH_GOOD;
        props->batteryLevel = 100;
        props->batteryChargeCounter= 1000000;
        props->batteryCurrent= 1000000;
        props->chargerAcOnline = true;
        props->chargerUsbOnline= false;
        props->chargerWirelessOnline = false;
        props->maxChargingCurrent= 2500000;
        props->maxChargingVoltage= 4300000;
        props->batteryPresent= true;
        props->batteryVoltage= 1200000;
        props->batteryTemperature= 25;
        props->batteryFullCharge= 4200000;
    } else
        UNUSED(props);
    return 0;
}


int main(void) {
    return health_service_main();
}
