ifeq ({{boot-arch}},efi)
# Adds edify commands swap_entries and copy_partition for robust
# update of the EFI system partition
TARGET_RECOVERY_UPDATER_LIBS := libupdater_esp
# Extra libraries needed to be rolled into recovery updater
# libgpt_static and libefivar are needed by libupdater_esp
TARGET_RECOVERY_UPDATER_EXTRA_LIBS := libcommon_recovery libgpt_static
ifeq ($(TARGET_SUPPORT_BOOT_OPTION),true)
TARGET_RECOVERY_UPDATER_EXTRA_LIBS += libefivar
endif
endif


BOARD_RECOVERYIMAGE_PARTITION_SIZE ?= 31457280
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE ?= 104857600
