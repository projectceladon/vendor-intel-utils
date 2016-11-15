#
# BoardConfig.mk for android_ia platform
#

TARGET_BOARD_PLATFORM := android_ia

# Some framework code requires this to enable BT
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/generic/common/bluetooth

BOARD_USE_LEGACY_UI := true

BOARD_SYSTEMIMAGE_PARTITION_SIZE = 1610612736

# customize the malloced address to be 16-byte aligned
BOARD_MALLOC_ALIGNMENT := 16

# Enable dex-preoptimization to speed up the first boot sequence
# of an SDK AVD. Note that this operation only works on Linux for now
ifeq ($(HOST_OS),linux)
WITH_DEXPREOPT := true
WITH_DEXPREOPT_PIC := true
endif

# the following variables could be overridden
TARGET_PRELINK_MODULE := false
TARGET_NO_KERNEL ?= false
TARGET_NO_RECOVERY ?= true
TARGET_HAS_THIRD_PARTY_APPS := true

BOARD_USES_GENERIC_AUDIO ?= false
BOARD_USES_TINY_ALSA_AUDIO ?= true
BOARD_USES_ALSA_AUDIO ?= true
BOARD_HAS_GPS_HARDWARE ?= true

# Don't build emulator
BUILD_EMULATOR ?= false

# Install Native Bridge
WITH_NATIVE_BRIDGE := true

# Native Bridge ABI List
NB_ABI_LIST_32_BIT := armeabi-v7a armeabi
NB_ABI_LIST_64_BIT := arm64-v8a

# Enable ARM codegen for x86 with Native Bridge
BUILD_ARM_FOR_X86 := true

ENABLE_NATIVEBRIDGE_64BIT := false
ifeq ($(BOARD_USE_64BIT_USERSPACE),true)
  ENABLE_NATIVEBRIDGE_64BIT = true
else
  ifeq ($(TARGET_SUPPORTS_64_BIT_APPS),true)
    ENABLE_NATIVEBRIDGE_64BIT = true
  endif
endif

# Support 64 Bit Apps
ifeq ($(ENABLE_NATIVEBRIDGE_64BIT),true)
  TARGET_CPU_ABI_LIST_64_BIT ?= $(TARGET_CPU_ABI) $(TARGET_CPU_ABI2)
  TARGET_CPU_ABI_LIST_32_BIT ?= $(TARGET_2ND_CPU_ABI) $(TARGET_2ND_CPU_ABI2)
  TARGET_CPU_ABI_LIST := \
      $(TARGET_CPU_ABI_LIST_64_BIT) \
      $(TARGET_CPU_ABI_LIST_32_BIT) \
      $(NB_ABI_LIST_32_BIT) \
      $(NB_ABI_LIST_64_BIT)
  TARGET_CPU_ABI_LIST_32_BIT += $(NB_ABI_LIST_32_BIT)
  TARGET_CPU_ABI_LIST_64_BIT += $(NB_ABI_LIST_64_BIT)
else
  TARGET_CPU_ABI_LIST_32_BIT ?= $(TARGET_CPU_ABI) $(TARGET_CPU_ABI2)
  TARGET_CPU_ABI_LIST_32_BIT += $(NB_ABI_LIST_32_BIT)
  TARGET_CPU_ABI_LIST := $(TARGET_CPU_ABI_LIST_32_BIT)
endif

BOARD_USE_LIBVA_INTEL_DRIVER := true
BOARD_USE_LIBVA := true
BOARD_USE_LIBMIX := true
BOARD_USES_WRS_OMXIL_CORE := true
USE_INTEL_OMX_COMPONENTS := true

USE_OPENGL_RENDERER := true
NUM_FRAMEBUFFER_SURFACE_BUFFERS ?= 3
BOARD_USES_DRM_HWCOMPOSER := true
BOARD_USES_IA_PLANNER := true
BOARD_USE_DMA_BUF_IMPORT := true

USE_CAMERA_STUB ?= false

# This enables the wpa wireless driver
BOARD_WPA_SUPPLICANT_DRIVER ?= NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB ?= lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_PRIVATE_LIB ?= lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER ?= NL80211
WIFI_DRIVER_MODULE_PATH ?= auto
BOARD_USES_WPA_SUPPLICANT_8_LIB ?= true
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WLAN_DEVICE := bcmdhd

# Kernel Flinger
TARGET_UEFI_ARCH := x86_64
# Kernelflinger won't check the ACPI table oem_id, oem_table_id and
# revision fields
KERNELFLINGER_ALLOW_UNSUPPORTED_ACPI_TABLE := true
# Allow Kernelflinger to start watchdog prior to boot the kernel
KERNELFLINGER_USE_WATCHDOG := true
# Tell Kernelflinger to ignore ACPI RSCI table
KERNELFLINGER_IGNORE_RSCI := true
KERNELFLINGER_SSL_LIBRARY := boringssl
# Specify system verity partition
#PRODUCT_SYSTEM_VERITY_PARTITION := /dev/block/by-name/system

BOARD_GPU_DRIVERS ?= i915 i965 swrast
ifneq ($(strip $(BOARD_GPU_DRIVERS)),)
TARGET_HARDWARE_3D := true
endif

BOARD_SEPOLICY_DIRS += device/android_ia/sepolicy

BOARD_KERNEL_CMDLINE := root=/dev/ram0 androidboot.hardware=$(TARGET_PRODUCT) i915.nuclear_pageflip=1 androidboot.selinux=permissive

# Use the non-open-source parts, if they're present
-include vendor/BoardConfigVendor.mk
