#
# -- OTA RELATED DEFINES --
#

# tell build system where to get the recovery.fstab
TARGET_RECOVERY_FSTAB ?= $(TARGET_DEVICE_DIR)/fstab.recovery

# Used by ota_from_target_files to add platform-specific directives
# to the OTA updater scripts
TARGET_RELEASETOOLS_EXTENSIONS ?= $(INTEL_PATH_HARDWARE)/bootctrl/recovery/bootloader

# By default recovery minui expects RGBA framebuffer
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"

{{^slot-ab}}
# Needed for ABL firmware update
TARGET_RECOVERY_UPDATER_LIBS := libabl_recovery
{{/slot-ab}}

#
# FILESYSTEMS
#

# NOTE: These values must be kept in sync with BOARD_GPT_INI
BOARD_BOOTIMAGE_PARTITION_SIZE ?= 31457280
BOARD_SYSTEMIMAGE_PARTITION_SIZE ?= 3221225472
BOARD_TOSIMAGE_PARTITION_SIZE ?= 10485760
BOARD_BOOTLOADER_PARTITION_SIZE ?= $$(({{bootloader_len}} * 1024 * 1024))
BOARD_BOOTLOADER_BLOCK_SIZE := {{{bootloader_block_size}}}
{{^slot-ab}}
BOARD_RECOVERYIMAGE_PARTITION_SIZE ?= 31457280
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE ?= 104857600
{{/slot-ab}}
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := {{system_fs}}

ifeq ({{data_use_f2fs}},true)
TARGET_USERIMAGES_USE_F2FS := true
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := f2fs
INTERNAL_USERIMAGES_EXT_VARIANT := f2fs
BOARD_USERDATAIMAGE_PARTITION_SIZE ?= 3774873600
else
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := ext4
INTERNAL_USERIMAGES_EXT_VARIANT := ext4
endif

TARGET_USERIMAGES_SPARSE_EXT_DISABLED := false

ifeq ($(BOARD_FLASH_UFS), 1)
BOARD_FLASH_BLOCK_SIZE := 4096
else
BOARD_FLASH_BLOCK_SIZE := 512
endif

#
#kernel always use primary gpt without command line option "gpt",
#the label let kernel use the alternate GPT if primary GPT is corrupted.
#
BOARD_KERNEL_CMDLINE += gpt

#
# Trusted Factory Reset - persistent partition
#
DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_HARDWARE)/bootctrl/boot/overlay

{{#run_tco_on_shutdown}}
BOARD_KERNEL_CMDLINE += iTCO_wdt.stop_on_shutdown=0
{{/run_tco_on_shutdown}}

BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/boot-arch/generic
{{#slot-ab}}
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/boot-arch/slotab_ota/generic
# no need to add $(INTEL_PATH_SEPOLICY)/boot-arch/slotab_ota/xbl
# because we do not have postinstall step with AaaG/vSBL
{{/slot-ab}}

TARGET_BOOTLOADER_BOARD_NAME := $(TARGET_DEVICE)

{{#fastboot_in_ifwi}}
FASTBOOT_IN_IFWI := true
{{/fastboot_in_ifwi}}

{{#slot-ab}}
{{#avb}}
BOARD_AVB_ENABLE := true
AB_OTA_PARTITIONS += vbmeta
AB_OTA_PARTITIONS += bootloader
{{#trusty}}
AB_OTA_PARTITIONS += tos
{{/trusty}}
{{/avb}}
{{/slot-ab}}

TARGET_USE_SBL := true
