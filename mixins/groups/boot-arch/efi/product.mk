TARGET_UEFI_ARCH := {{{uefi_arch}}}
BIOS_VARIANT := {{{bios_variant}}}

{{^avb}}
$(call inherit-product,build/target/product/verity.mk)

PRODUCT_SYSTEM_VERITY_PARTITION := /dev/block/by-name/system
{{/avb}}

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/fstab:root/fstab.$(TARGET_PRODUCT) \
	frameworks/native/data/etc/android.software.verified_boot.xml:vendor/etc/permissions/android.software.verified_boot.xml

{{#acpi_permissive}}
# Kernelflinger won't check the ACPI table oem_id, oem_table_id and
# revision fields
KERNELFLINGER_ALLOW_UNSUPPORTED_ACPI_TABLE := true
{{/acpi_permissive}}
{{#use_power_button}}
# Allow Kernelflinger to use the power key as an input source.
# Doesn't work on most boards.
KERNELFLINGER_USE_POWER_BUTTON := true
{{/use_power_button}}
{{^disable_watchdog}}
# Allow Kernelflinger to start watchdog prior to boot the kernel
KERNELFLINGER_USE_WATCHDOG := true
{{/disable_watchdog}}
{{#use_charging_applet}}
# Allow Kernelflinger to use the non-standard ChargingApplet protocol
# to get battery and charger status and modify the boot flow in
# consequence.
KERNELFLINGER_USE_CHARGING_APPLET := true
{{/use_charging_applet}}
{{#ignore_rsci}}
# Allow Kernelflinger to ignore the non-standard RSCI ACPI table
# to get reset and wake source from PMIC for bringup phase if
# the table reports incorrect data
KERNELFLINGER_IGNORE_RSCI := true
{{/ignore_rsci}}
{{#bootloader_policy}}
# It activates the Bootloader policy and RMA refurbishing
# features. TARGET_BOOTLOADER_POLICY is the desired bitmask for this
# device.
# * bit 0:
#   - 0: GVB class B.
#   - 1: GVB class A.  Device unlock is not permitted.  The only way
#     to unlock is to use the secured force-unlock mechanism.
# * bit 1 and 2 defines the minimal boot state required to boot the
#   device:
#   - 0x0: BOOT_STATE_RED (GVB default behavior)
#   - 0x1: BOOT_STATE_ORANGE
#   - 0x2: BOOT_STATE_YELLOW
#   - 0x3: BOOT_STATE_GREEN
# If TARGET_BOOTLOADER_POLICY is equal to 'static' the bootloader
# policy is not built but is provided statically in the repository.
# If TARGET_BOOTLOADER_POLICY is equal to 'external' the bootloader
# policy OEMVARS should be installed manually in
# $(BOOTLOADER_POLICY_OEMVARS).
TARGET_BOOTLOADER_POLICY := {{bootloader_policy}}
# If the following variable is set to false, the bootloader policy and
# RMA refurbishing features does not use time-based authenticated EFI
# variables to store the BPM and OAK values.  The BPM value is defined
# compilation time by the TARGET_BOOTLOADER_POLICY variable.
TARGET_BOOTLOADER_POLICY_USE_EFI_VAR := {{blpolicy_use_efi_var}}
ifeq ($(TARGET_BOOTLOADER_POLICY),$(filter $(TARGET_BOOTLOADER_POLICY),0x0 0x2 0x4 0x6))
# OEM Unlock reporting
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	ro.oem_unlock_supported=1
endif
ifeq ($(TARGET_BOOTLOADER_POLICY),$(filter $(TARGET_BOOTLOADER_POLICY),static external))
# The bootloader policy is not generated build time but is supplied
# statically in the repository or in $(PRODUCT_OUT)/.  If your
# bootloader policy allows the device to be unlocked, uncomment the
# following lines:
# PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
# 	ro.oem_unlock_supported=1
endif
{{/bootloader_policy}}
{{^bootloader_policy}}
# OEM Unlock reporting
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	ro.oem_unlock_supported=1
{{/bootloader_policy}}
{{#ignore_not_applicable_reset}}
# Allow Kernelflinger to ignore the RSCI reset source "not_applicable"
# when setting the bootreason
KERNELFLINGER_IGNORE_NOT_APPLICABLE_RESET := true
{{/ignore_not_applicable_reset}}
{{#verity_warning}}
{{^avb}}
PRODUCT_PACKAGES += \
	slideshow \
	verity_warning_images
{{/avb}}
{{/verity_warning}}
{{#txe_bind_root_of_trust}}
# It makes kernelflinger bind the device state to the root of trust
# using the TXE.
KERNELFLINGER_TXE_BIND_ROOT_TRUST := true
{{/txe_bind_root_of_trust}}
{{#os_secure_boot}}
# Kernelfligner will set the global variable OsSecureBoot
# The BIOS must support this variable to enable this feature
KERNELFLINGER_OS_SECURE_BOOT := true
{{/os_secure_boot}}
# Android Kernelflinger uses the OpenSSL library to support the
# bootloader policy
KERNELFLINGER_SSL_LIBRARY := openssl

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.frp.pst=/dev/block/by-name/persistent
