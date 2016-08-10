#Product Characteristics
PRODUCT_DIR := $(dir $(lastword $(filter-out device/common/%,$(filter device/%,$(ALL_PRODUCTS)))))

# Product overrides
$(call inherit-product,device/android_ia/common/product_overrides.mk)

PRODUCT_CHARACTERISTICS := tablet

$(call inherit-product,frameworks/native/build/tablet-10in-xhdpi-2048-dalvik-heap.mk)

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_AAPT_CONFIG := normal large xlarge mdpi hdpi xhdpi xxhdpi
PRODUCT_AAPT_PREF_CONFIG := xxhdpi

DEVICE_PACKAGE_OVERLAYS := device/android_ia/common/overlay

# Kernel
TARGET_KERNEL_ARCH := x86_64

KERNEL_MODULES_ROOT_PATH ?= /system/lib/modules
KERNEL_MODULES_ROOT ?= $(KERNEL_MODULES_ROOT_PATH)

TARGET_HAS_THIRD_PARTY_APPS := true

PRODUCT_PACKAGES := $(THIRD_PARTY_APPS)

FIRMWARES_DIR ?= device/android_ia/firmware

# Include common settings.
FIRMWARE_FILTERS ?= .git/% %.mk

# Copy all necessary settings.
$(call inherit-product,device/android_ia/common/product_files.mk)

# Firmware
$(call inherit-product,device/android_ia/common/firmware.mk)

# Packages
$(call inherit-product,device/android_ia/common/product_packages.mk)

# Get a list of languages.
$(call inherit-product,$(SRC_TARGET_DIR)/product/locales_full.mk)

# Get everything else from the parent package
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_no_telephony.mk)

# Get some sounds
$(call inherit-product-if-exists,frameworks/base/data/sounds/AudioPackage6.mk)

# Get Platform specific settings
$(call inherit-product-if-exists,vendor/vendor.mk)
