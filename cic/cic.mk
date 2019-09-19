# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/board/generic_x86_64/device.mk)
$(call inherit-product, $(LOCAL_PATH)/device.mk)

TARGET_USE_GRALLOC_VHAL := true
TARGET_AIC_DEVICE_INPUT_FILTER := true
TARGET_AIC_PERF := true

PRODUCT_CHARACTERISTICS := tablet
PRODUCT_AAPT_CONFIG := normal large xlarge mdpi hdpi
PRODUCT_AAPT_PREF_CONFIG := mdpi

INTEL_PATH_DEVICE_CIC := device/intel/project-celadon/cic
INTEL_PATH_KERNEL_MODULES_CIC := kernel/modules/cic
INTEL_PATH_VENDOR_CIC := vendor/intel/cic
INTEL_PATH_VENDOR_CIC_GRAPHIC := $(INTEL_PATH_VENDOR_CIC)/target/graphics
INTEL_PATH_VENDOR_CIC_HAL := $(INTEL_PATH_VENDOR_CIC)/target/hals

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.ethernet.xml:system/etc/permissions/android.hardware.ethernet.xml \
    frameworks/native/data/etc/android.software.app_widgets.xml:system/etc/permissions/android.software.app_widgets.xml \
    $(LOCAL_PATH)/android-removed-permissions.xml:system/etc/permissions/android-removed-permissions.xml \
    $(LOCAL_PATH)/fstab:root/fstab.$(TARGET_PRODUCT) \
    $(LOCAL_PATH)/init.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.$(TARGET_PRODUCT).rc \
    $(LOCAL_PATH)/ueventd.rc:$(TARGET_COPY_OUT_VENDOR)/ueventd.rc \
    $(LOCAL_PATH)/manifest.xml:$(TARGET_COPY_OUT_VENDOR)/manifest.xml \
    out/target/product/$(TARGET_PRODUCT)/system/bin/sdcard-fuse:system/bin/sdcard \

PRODUCT_NAME := cic
PRODUCT_DEVICE := cic
PRODUCT_BRAND := Intel
PRODUCT_MODEL := CIC Container image
