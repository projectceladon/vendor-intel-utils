# This enables the wpa wireless driver
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION := VER_2_1_DEVEL
{{#libwifi-hal}}
# required for wifi HAL support
BOARD_WLAN_DEVICE := iwlwifi
{{/libwifi-hal}}

# Enabling iwlwifi
BOARD_USING_INTEL_IWL := true
INTEL_IWL_MODULE_SUB_FOLDER := {{{iwl_sub_folder}}}
INTEL_IWL_PLATFORM := {{{iwl_platform}}}
INTEL_IWL_BOARD_CONFIG := {{{iwl_defconfig}}}
INTEL_IWL_PNVM_HW := {{{iwl_pnvm_hw}}}
INTEL_IWL_USE_COMPAT_INSTALL := y
INTEL_IWL_COMPAT_INSTALL_DIR := updates
INTEL_IWL_COMPAT_INSTALL_PATH ?= .
INTEL_IWL_INSTALL_MOD_STRIP := INSTALL_MOD_STRIP=1
CROSS_COMPILE ?= CROSS_COMPILE=$(YOCTO_CROSSCOMPILE)
INSTALLED_KERNEL_TARGET ?= $(PREVIOUS_KERNEL_MODULE)
KERNEL_OUT_DIR ?= $(PRODUCT_OUT)/obj/kernel

COMBO_CHIP_VENDOR := intel
COMBO_CHIP := lnp

# SoftAp FW reload definitions.
# we don't really need this, it's to avoid error when the framework
# will trigger the fwReloadSoftap function, what will lead to an error
# enabling the SoftAp.
# so we set up this for letting the function execute gracefully.
WIFI_DRIVER_FW_PATH_STA := "/vendor/firmware/iwlwifi-softap-dummy.ucode"
WIFI_DRIVER_FW_PATH_AP  := "/vendor/firmware/iwlwifi-softap-dummy.ucode"
WIFI_DRIVER_FW_PATH_P2P := "/vendor/firmware/iwlwifi-softap-dummy.ucode"
WIFI_DRIVER_FW_PATH_PARAM := "/dev/null"

# config_wifi_background_scan_support=true:
DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/wlan/overlay-pno

DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/wlan/overlay-tcp-buffers

# Add SIM , AKA and AKA' methods in EAP entries of WiFi UI
DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/wlan/overlay-eap-methods

ifneq (lhp,$(INTEL_IWL_MODULE_SUB_FOLDER))
    DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/wlan/overlay-dual-band
endif

# WiDi / Miracast Optimisations
DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/wlan/overlay-miracast-go
DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/wlan/overlay-p2p-connected-stop-scan
DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_COMMON)/wlan/overlay-miracast-force-single-ch

BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/wlan/load_iwlwifi

{{#gpp}}
ifeq ($(findstring cws_manu,$(BOARD_SEPOLICY_DIRS)),)
    BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/cws_manu
endif
{{/gpp}}

BOARD_SEPOLICY_M4DEFS += module_iwlwifi=true
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/wlan/iwlwifi
