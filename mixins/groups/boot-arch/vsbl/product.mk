IFWI_VARIANT := {{{ifwi_variant}}}

{{^avb}}
$(call inherit-product,build/target/product/verity.mk)

PRODUCT_SYSTEM_VERITY_PARTITION := /dev/block/by-name/system
{{/avb}}
{{^acrn-fstab-flag}}
PRODUCT_COPY_FILES += \
{{#treble}}
	$(LOCAL_PATH)/fstab:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.$(TARGET_DEVICE) \
{{/treble}}
{{^treble}}
	$(LOCAL_PATH)/fstab:root/fstab.$(TARGET_DEVICE) \
{{/treble}}
	frameworks/native/data/etc/android.software.verified_boot.xml:vendor/etc/permissions/android.software.verified_boot.xml
{{/acrn-fstab-flag}}
{{#acrn-fstab-flag}}
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/fstab.acrn:root/fstab.$(TARGET_DEVICE) \
	frameworks/native/data/etc/android.software.verified_boot.xml:vendor/etc/permissions/android.software.verified_boot.xml
{{/acrn-fstab-flag}}
TARGET_NO_DEVICE_UNLOCK := false
TARGET_BOOT_SIGNER := ias
TARGET_IAFW_ARCH := i386

# Libpayload configuration for ABL
LIBPAYLOAD_BASE_ADDRESS := 0x12800000
LIBPAYLOAD_HEAP_SIZE := 125829120
LIBPAYLOAD_STACK_SIZE := 1048576

LIBEFIWRAPPER_DRIVERS := s8250mem32 lprtc lpmemmap virtual_media dw3 cf9 abl

CAPSULE_SOURCE := sbl

# Disable Kernelflinger UI support
KERNELFLINGER_USE_UI := false
# Support boot from osloader
KERNELFLINGER_SUPPORT_NON_EFI_BOOT := true
KERNELFLINGER_SECURITY_PLATFORM := vsbl
# Disable kernelflinger debug print to save boot time
KERNELFLINGER_DISABLE_DEBUG_PRINT := true
KERNELFLINGER_DISABLE_EFI_MEMMAP := true

ABL_OS_KERNEL_KEY := $(INTEL_PATH_BUILD)/testkeys/slimboot
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.frp.pst=/dev/block/by-name/persistent

# Kernelflinger will set IOC_USE_SLCAN variable
# Distinguish between MRB IOC and NON_MRB_IOC
IOC_USE_SLCAN := false
# Use SHA256 implemented with Intel SHA extension instructions
KERNELFLINGER_USE_IPP_SHA256 := {{use_ipp_sha256}}

{{#rpmb}}
KERNELFLINGER_USE_RPMB := true
{{/rpmb}}

{{#use_ec_uart}}
EFIWRAPPER_USE_EC_UART := true
{{/use_ec_uart}}

{{#x64}}
TARGET_UEFI_ARCH := x86_64
{{/x64}}
{{#trusty}}
KERNELFLINGER_TRUSTY_PLATFORM := vsbl
{{/trusty}}
