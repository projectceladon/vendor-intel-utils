/*
 * Copyright 2018 The Android Open Source Project
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

#ifndef _STORAGE_HAL_H_
#define _STORAGE_HAL_H_

#include <vector>
#include <android/hardware/health/1.0/types.h>
#include <android/hardware/health/2.0/IHealth.h>

#define MMC_DISK_BUS "1c.0"
#define VDA_DISK_BUS "03.0"
#define SDA_DISK_BUS "17.0"
#define NVME_DISK_BUS "1D.0"

using android::hardware::health::V2_0::StorageInfo;
using android::hardware::health::V2_0::DiskStats;
const char diskbus_build_property[] = "ro.boot.diskbus";

/*
 * Storaged ::Get disk statistics and get storage information utilities.
 */
class StorageInf {
public:
    virtual void get_diskstats(std::vector<struct DiskStats>& stats) = 0;
    virtual void get_storageinfo(std::vector<struct StorageInfo>& info) = 0;
    virtual ~StorageInf() {}
};

class SdaDev : public StorageInf {
public:
    void get_diskstats(std::vector<struct DiskStats>& stats);
    void get_storageinfo(std::vector<struct StorageInfo>& info);
};

class NvmeDev : public StorageInf {
public:
    void get_diskstats(std::vector<struct DiskStats>& stats);
    void get_storageinfo(std::vector<struct StorageInfo>& info);
};

class EmmcDev : public StorageInf {
public:
    void get_diskstats(std::vector<struct DiskStats>& stats);
    void get_storageinfo(std::vector<struct StorageInfo>& info);
};

class VdaDev : public StorageInf {
public:
    void get_diskstats(std::vector<struct DiskStats>& stats);
    void get_storageinfo(std::vector<struct StorageInfo>& info);
};

/*
 * publish diskstat metrics to storaged framework.
 */
void get_diskstats_io(std::vector<struct DiskStats>& stats);

/*
 * Read diskstat metrics from disk block path.
 */
void get_disk_blk(const char blkpath[], std::vector<struct DiskStats>& stats);
#endif  /* _STORAGE_HAL_H_ */
