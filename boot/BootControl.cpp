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

#define LOG_TAG "android.hardware.boot-service.intel"

#include "BootControl.h"
#include "boot_control_avb.h"
#include <cstdint>
#include <log/log.h>

#include <android-base/logging.h>

using HIDLMergeStatus = ::android::bootable::BootControl::MergeStatus;
using ndk::ScopedAStatus;

namespace aidl::android::hardware::boot {

BootControl::BootControl() {
    int ret = 0;

    boot_control_module_t *module = NULL;
    hw_module_t **hwm = reinterpret_cast<hw_module_t **>(&module);
    ret = hw_get_module(BOOT_CONTROL_HARDWARE_MODULE_ID, const_cast<const hw_module_t **>(hwm));
    if (ret) {
        ALOGE("hw_get_module %s failed: %d", BOOT_CONTROL_HARDWARE_MODULE_ID, ret);
    } else {
        mModule = module;
    }

    mModule->init(mModule);
}

ScopedAStatus BootControl::getActiveBootSlot(int32_t* _aidl_return) {
    *_aidl_return = mModule->getActiveBootSlot(mModule);
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::getCurrentSlot(int32_t* _aidl_return) {
    *_aidl_return = mModule->getCurrentSlot(mModule);
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::getNumberSlots(int32_t* _aidl_return) {
    *_aidl_return = mModule->getNumberSlots(mModule);
    return ScopedAStatus::ok();
}

namespace {

static constexpr MergeStatus ToAIDLMergeStatus(HIDLMergeStatus status) {
    switch (status) {
        case HIDLMergeStatus::NONE:
            return MergeStatus::NONE;
        case HIDLMergeStatus::UNKNOWN:
            return MergeStatus::UNKNOWN;
        case HIDLMergeStatus::SNAPSHOTTED:
            return MergeStatus::SNAPSHOTTED;
        case HIDLMergeStatus::MERGING:
            return MergeStatus::MERGING;
        case HIDLMergeStatus::CANCELLED:
            return MergeStatus::CANCELLED;
    }
}

static constexpr HIDLMergeStatus ToHIDLMergeStatus(MergeStatus status) {
    switch (status) {
        case MergeStatus::NONE:
            return HIDLMergeStatus::NONE;
        case MergeStatus::UNKNOWN:
            return HIDLMergeStatus::UNKNOWN;
        case MergeStatus::SNAPSHOTTED:
            return HIDLMergeStatus::SNAPSHOTTED;
        case MergeStatus::MERGING:
            return HIDLMergeStatus::MERGING;
        case MergeStatus::CANCELLED:
            return HIDLMergeStatus::CANCELLED;
    }
}

}

ScopedAStatus BootControl::getSnapshotMergeStatus(MergeStatus* _aidl_return) {
    private_boot_control_t *pModule = (private_boot_control_t *)(mModule);

    *_aidl_return = ToAIDLMergeStatus((HIDLMergeStatus)pModule->GetSnapshotMergeStatus());

    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::getSuffix(int32_t in_slot, std::string* _aidl_return) {
    const char *suffix = mModule->getSuffix(mModule, in_slot);

    if (suffix) {
        *_aidl_return = suffix;
    }

    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::isSlotBootable(int32_t in_slot, bool* _aidl_return) {
    int ret = mModule->isSlotBootable(mModule, in_slot);

    if (ret < 0) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }
    *_aidl_return = ret;

    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::isSlotMarkedSuccessful(int32_t in_slot, bool* _aidl_return) {
    int ret = mModule->isSlotMarkedSuccessful(mModule, in_slot);

    if (ret < 0) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }
    *_aidl_return = ret;
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::markBootSuccessful() {
    if (mModule->markBootSuccessful(mModule)) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(COMMAND_FAILED,
                                                                  "Operation failed");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::setActiveBootSlot(int32_t in_slot) {
    int ret = mModule->setActiveBootSlot(mModule, in_slot);

    if (ret != 0) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }

    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::setSlotAsUnbootable(int32_t in_slot) {
    int ret = mModule->setSlotAsUnbootable(mModule, in_slot);

    if (ret != 0) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(
                INVALID_SLOT, (std::string("Invalid slot ") + std::to_string(in_slot)).c_str());
    }

    return ScopedAStatus::ok();
}

ScopedAStatus BootControl::setSnapshotMergeStatus(MergeStatus in_status) {
    private_boot_control_t *pModule = (private_boot_control_t *)(mModule);

    if (!pModule->SetSnapshotMergeStatus((uint8_t)ToHIDLMergeStatus(in_status))) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(COMMAND_FAILED,
                                                                  "Operation failed");
    }
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::boot
