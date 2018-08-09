# Set default USB interface
USB_CONFIG := {{{usb_config}}}

ifeq ($(TARGET_BUILD_VARIANT),user)
# Enable Secure Debugging
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.adb.secure=1
ifeq ($(BUILD_FOR_CTS_AUTOMATION),true)
# Build for automated CTS
ifneq ($(USB_CONFIG), adb)
USB_CONFIG := $(USB_CONFIG),adb
endif
PRODUCT_COPY_FILES += $(INTEL_PATH_COMMON)/usb-gadget/adb_keys:root/adb_keys
endif #BUILD_FOR_CTS_AUTOMATION == true
endif #TARGET_BUILD_VARIANT == user
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.usb.config=$(USB_CONFIG)

# Add Intel adb keys for userdebug/eng builds
ifneq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_COPY_FILES += $(INTEL_PATH_COMMON)/usb-gadget/adb_keys:root/adb_keys
endif
