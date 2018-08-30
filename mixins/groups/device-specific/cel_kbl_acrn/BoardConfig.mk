DEVICE_PACKAGE_OVERLAYS += $(TARGET_DEVICE_DIR)/overlay

BOARD_KERNEL_CMDLINE += \
	no_timer_check \
	noxsaves \
	reboot_panic=p,w \
	i915.hpd_sense_invert=0x7 \
	intel_iommu=off

BOARD_FLASHFILES += $(TARGET_DEVICE_DIR)/bldr_utils.img:bldr_utils.img

# for USB OTG WA
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/bxt_usb

TARGET_USES_HWC2 := true
BOARD_USES_GENERIC_AUDIO := true
BOARD_USERDATAIMAGE_PARTITION_SIZE := 3040870400
