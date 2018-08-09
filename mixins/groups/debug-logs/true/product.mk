{{#eng_only}}
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
ifeq ($(BUILD_FOR_CTS_AUTOMATION),true)
MIXIN_DEBUG_LOGS := true
endif
endif
ifeq ($(TARGET_BUILD_VARIANT),eng)
MIXIN_DEBUG_LOGS := true
endif
{{/eng_only}}
{{^eng_only}}
ifneq ($(TARGET_BUILD_VARIANT),user)
MIXIN_DEBUG_LOGS := true
endif
{{/eng_only}}

ifeq ($(MIXIN_DEBUG_LOGS),true)
{{#treble}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/init.logs.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.logs.rc
{{/treble}}
{{^treble}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/init.logs.rc:root/init.logs.rc
{{/treble}}
PRODUCT_PACKAGES += \
{{#logger_pack}}
    {{logger_pack}} \
{{/logger_pack}}
    elogs.sh \
    start_log_srv.sh \
    logcat_ep.sh
endif

ifeq ($(MIXIN_DEBUG_LOGS),true)
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.service.default_logfs=apklogfs
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.intel.logger={{logger}}
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += logd.kernel.raw_message={{klogd_raw_message}}
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.intel.logger.rot_cnt={{logger_rot_cnt}}
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.intel.logger.rot_size={{logger_rot_size}}
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/debug-logs
BOARD_SEPOLICY_M4DEFS += module_debug_logs=true
endif
