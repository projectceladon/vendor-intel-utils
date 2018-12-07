ifeq ($(MIXIN_DEBUG_LOGS),true)
{{#treble}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/init.coredump.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.coredump.rc
{{/treble}}
{{^treble}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/init.coredump.rc:root/init.coredump.rc
{{/treble}}
endif

ifeq ($(MIXIN_DEBUG_LOGS),true)
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/coredump
{{#data_dump}}
BOARD_SEPOLICY_M4DEFS += module_debug_coredump=true
{{/data_dump}}
# Enable core dump for eng builds
ifneq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.vendor.core.enabled=1
else
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.vendor.core.enabled=0
endif
CRASHLOGD_COREDUMP := true
endif
