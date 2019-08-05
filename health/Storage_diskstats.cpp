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
#define LOG_TAG "storagediskstats"
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
std::ifstream StatFile;
uint64_t sysfs_block_stat;
uint64_t sysfs_block_stat_val[20];
uint64_t sysfs_block_read_counter = 0;

void get_diskstats_io(std::vector<struct DiskStats>& stats) {
    // publishing diskstat metrics to storaged framework
    LOG(INFO) << LOG_TAG << "get_diskstats_io: Enter\n";
    DiskStats diskstats;
    diskstats.reads = sysfs_block_stat_val[0];
    diskstats.readMerges = sysfs_block_stat_val[1];
    diskstats.readSectors = sysfs_block_stat_val[2];
    diskstats.readTicks = sysfs_block_stat_val[3];
    diskstats.writes = sysfs_block_stat_val[4];
    diskstats.writeMerges = sysfs_block_stat_val[5];
    diskstats.writeSectors = sysfs_block_stat_val[6];
    diskstats.writeTicks = sysfs_block_stat_val[7];
    diskstats.ioInFlight = sysfs_block_stat_val[8];
    diskstats.ioTicks = sysfs_block_stat_val[9];
    diskstats.ioInQueue = sysfs_block_stat_val[10];
    stats.push_back(diskstats);
}

void get_disk_blk(const char blkpath[], std::vector<struct DiskStats>& stats) {
        // Reading diskstat metrics from disk block path.
    StatFile.open(blkpath);
    if (!StatFile.is_open()) {
        return;
    }
    while(StatFile >> sysfs_block_stat) {
        sysfs_block_stat_val[sysfs_block_read_counter]= sysfs_block_stat;
        sysfs_block_read_counter++;
    }
    get_diskstats_io(stats);
    StatFile.close();
}

/*
 * Device type:: get_diskstats utility
 */
void SdaDev :: get_diskstats(std::vector<struct DiskStats>& stats) {
    sysfs_block_read_counter = 0;
    const char diskstat_blkpath[] = "/sys/block/sda/stat";
    get_disk_blk(diskstat_blkpath, stats); //parsing SATA diskstat information
}

void EmmcDev :: get_diskstats(std::vector<struct DiskStats>& stats) {
    sysfs_block_read_counter = 0;
    const char diskstat_blkpath[] = "/sys/block/mmcblk1/stat";
    get_disk_blk(diskstat_blkpath, stats); //parsing eMMC diskstat information
}

void VdaDev :: get_diskstats(std::vector<struct DiskStats>& stats) {
    sysfs_block_read_counter = 0;
    const char diskstat_blkpath[] = "/sys/block/vda/stat";
    get_disk_blk(diskstat_blkpath, stats); //parsing VDA diskstat information
}

void NvmeDev :: get_diskstats(std::vector<struct DiskStats>& stats) {
    sysfs_block_read_counter = 0;
    const char diskstat_blkpath[] = "/sys/block/nvme0n1/stat";
    get_disk_blk(diskstat_blkpath, stats); //parsing NVMe diskstat information
}
