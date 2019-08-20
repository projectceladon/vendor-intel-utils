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
#define LOG_TAG "android.hardware.health@2.0-impl.intel"
#include <utils/String8.h>
#include <cutils/properties.h>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/logging.h>
#include <health2/Health.h>
#include <health2/storage_hal.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

using namespace std;
std::ifstream File;
char diskbus_prop_value[PROP_VALUE_MAX];
enum storage_type{EMMC, NVME, SDA, VDA, NODISK}; // TODO: Support shall be extended to UFS in future version.
enum storage_type get_storage_type();
class StorageInf* get_storage_dev(int dev_type);

/*
 * The function exposes the lower layer storage device wear information such as lifetime information.
 * The function focus to expose the boot media information.
 */
void get_storage_info(std::vector<struct StorageInfo>& info) {
    LOG(INFO) << LOG_TAG << "get_storage_info:START\n";
    enum storage_type type;
    type = get_storage_type();
    StorageInf *IDev = get_storage_dev(type);
    if(IDev != nullptr) {
        IDev->get_storageinfo(info);
    }
    delete IDev;
}

/*
 * The function identifies the boot medium from the available storage devices on board.
 */
enum storage_type get_storage_type() {
    memset(diskbus_prop_value, 0, sizeof(diskbus_prop_value));
        property_get(diskbus_build_property, diskbus_prop_value, "0");
    if (strcmp(diskbus_prop_value, SDA_DISK_BUS) == 0)
        return SDA;
    else if (strcmp(diskbus_prop_value, NVME_DISK_BUS) == 0)
        return NVME;
    else if (strcmp(diskbus_prop_value, MMC_DISK_BUS) == 0)
        return EMMC;
    else if (strcmp(diskbus_prop_value, VDA_DISK_BUS) == 0)
        return VDA;
    else
        return NODISK;
}

/*
 * The function returns the boot media Storage device class
 */
class StorageInf* get_storage_dev(int dev_type) {
    if(dev_type == SDA)
        return new SdaDev();
    else if(dev_type == EMMC)
        return new EmmcDev();
    else if(dev_type == VDA)
        return new VdaDev();
    else if(dev_type == NVME)
        return new NvmeDev();
    else
        return nullptr;
}

/*
 * The function exposes the lower layer storage device statistics since the system boot.
 */
void get_disk_stats(std::vector<struct DiskStats>& stats) {
    LOG(INFO) << LOG_TAG << "get_disk_stats:START\n";
    enum storage_type type;
    type = get_storage_type();
    StorageInf *SDev = get_storage_dev(type);
    if(SDev != nullptr) {
        SDev->get_diskstats(stats);
    }
    delete SDev;
}
