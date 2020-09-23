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
#define LOG_TAG "storageinfo"
#include <utils/String8.h>
#include <cutils/properties.h>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/logging.h>
#include <health2/Health.h>
#include <health2/storage_hal.h>

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

using namespace std;
std::ifstream InfoFile;
uint16_t ext_csd_stat_val[3];
uint16_t ext_csd_lifetime_val;
uint16_t ext_csd_lifetime_counter = 0;
uint16_t Max_eol = 3;
uint16_t Max_lifetimeA = 0x0B;
uint16_t Max_lifetimeB = 0x0B;
const char mmc1_sysfs_eol[] = "/sys/bus/mmc/devices/mmc1:0001/pre_eol_info";
const char mmc1_sysfs_lifetime[] = "/sys/bus/mmc/devices/mmc1:0001/life_time";

/*
 * mmc version sys node is not supported.
 * TODO: in Storaged future version
 */
//const char mmc1_sysfs_version[] = "/sys/bus/mmc/devices/mmc1:0001/version";

/*
 * Sda Device type :: get_storageinfo utility
 */
void SdaDev :: get_storageinfo(std::vector<struct StorageInfo>& info) {
    /*
         * SATA driver does not expose end of life and lifetime information.
         * So assigning eol and lifetimeA, lifetimeB values manually. FIXME
         */
    StorageInfo storageinfo;
    storageinfo.eol = Max_eol;
    storageinfo.lifetimeA = Max_lifetimeA;
    storageinfo.lifetimeB = Max_lifetimeB;
    storageinfo.version = "3.0";
    info.push_back(storageinfo);
}

/*
 * eMMC Device type :: get_storageinfo utility
 */
void EmmcDev :: get_storageinfo(std::vector<struct StorageInfo>& info) {
    StorageInfo storageinfo;
    ext_csd_lifetime_counter = 0;
    InfoFile.open(mmc1_sysfs_eol);      //parsing ext_csd end of life information
    if (!InfoFile.is_open()) {
        return;
    }
    InfoFile >> std::hex >> storageinfo.eol;
    InfoFile.close();
    InfoFile.open(mmc1_sysfs_lifetime); //parsing ext_csd life time estimates
    if (!InfoFile.is_open()) {
        return;
    }
    while(InfoFile >> std::hex >> ext_csd_lifetime_val) {
        ext_csd_stat_val[ext_csd_lifetime_counter] = ext_csd_lifetime_val;
        ext_csd_lifetime_counter++;
    }
    storageinfo.lifetimeA = ext_csd_stat_val[0];
    storageinfo.lifetimeB = ext_csd_stat_val[1];
    InfoFile.close();

    /*
         * TO DO: ext_csd version parsing in Storaged version 0.1
         * std::string ext_csd_version_val;
         * File.open(mmc1_sysfs_version);
         * File >> ext_csd_version_val;
         * storageinfo.version = ext_csd_version_val;
         * File.close();
         */
    storageinfo.version = "5.0";
    info.push_back(storageinfo);
}

/*
 * Virtual Device type :: get_storageinfo utility
 */
void VdaDev :: get_storageinfo(std::vector<struct StorageInfo>& info) {
        StorageInfo storageinfo;
    /*
     * Acrn has virtual nodes. So, it does not expose end of life and lifetime information.
         * So assigning eol and lifetimeA, lifetimeB values manually. FIXME
         */
    storageinfo.eol = Max_eol;
    storageinfo.lifetimeA = Max_lifetimeA;
    storageinfo.lifetimeB = Max_lifetimeB;
    storageinfo.version = "5.0";
    info.push_back(storageinfo);
}

/*
 * NVMe Device type :: get_storageinfo utility
 */
void NvmeDev :: get_storageinfo(std::vector<struct StorageInfo>& info) {
    /*
         * NVME driver does not expose end of life and lifetime information.
         * So assigning eol and lifetimeA, lifetimeB values manually. FIXME
         */
    StorageInfo storageinfo;
    storageinfo.eol = Max_eol;
    storageinfo.lifetimeA = Max_lifetimeA;
    storageinfo.lifetimeB = Max_lifetimeB;
    storageinfo.version = "1.3c";
    info.push_back(storageinfo);
}
