# Enable FOTA for non user builds
PRODUCT_PACKAGES_DEBUG += AFotaApp

ifneq ($(TARGET_BUILD_VARIANT),user)
{{#update_stream}}
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.vendor.fota.ota_stream={{update_stream}}
{{/update_stream}}

AFOTAAPP_EULA_PATH := {{{eula}}}
AFOTAAPP_LOG_LEVEL := {{{log_level}}}
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/fota
endif
