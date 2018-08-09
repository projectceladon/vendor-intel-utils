# for Open-avb gPTP daemon
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/gptp

# for AVB Streamhandler daemon
BOARD_SEPOLICY_M4DEFS += module_gptp=true
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/avbstreamhandler

# Common CFLAGS for IAS components
#IAS eng and userdebug build variant
ifneq ($(TARGET_BUILD_VARIANT),user)
  IAS_COMMON_CFLAGS := -DENG_DEBUG
else
  IAS_COMMON_CFLAGS :=
endif

# define IAS_TREBLE_COMPLIANT flag for IAS components
IAS_COMMON_CFLAGS += -DIAS_TREBLE_COMPLIANT

TARGET_FS_CONFIG_GEN += $(INTEL_PATH_COMMON)/eavb/filesystem_config/config.fs
