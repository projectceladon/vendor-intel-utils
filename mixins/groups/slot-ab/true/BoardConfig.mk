AB_OTA_UPDATER := true
AB_OTA_PARTITIONS := \
    boot \
    system
BOARD_BUILD_SYSTEM_ROOT_IMAGE := true
TARGET_NO_RECOVERY := true
BOARD_USES_RECOVERY_AS_BOOT := true
BOARD_SLOT_AB_ENABLE := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2684354560
PRODUCT_STATIC_BOOT_CONTROL_HAL := bootctrl.intel.static
BOARD_KERNEL_CMDLINE += rootfstype={{system_fs}}

ifeq ({{ota-upgrade}},False)
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/slot-ab/generic

ifeq ({{boot-arch}},efi)
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/slot-ab/efi
endif

ifeq ({{post_fw_update}},true)
AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_vendor=true \
    POSTINSTALL_PATH_vendor=bin/update_ifwi_ab \
    FILESYSTEM_TYPE_vendor={{system_fs}} \
    POSTINSTALL_OPTIONAL_vendor=true

BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/slot-ab/xbl

endif

ifeq ({{boot_fw_update}},true)

ifeq ({{post_fw_update}},False)
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/slot-ab/xbl
endif

BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/slot-ab/fw_update

endif
endif
