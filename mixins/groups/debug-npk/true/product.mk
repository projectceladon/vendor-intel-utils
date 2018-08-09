# There is a strong dependency on debug-logs; disable debug-npk if not set
ifeq ($(MIXIN_DEBUG_LOGS),true)

PRODUCT_COPY_FILES += \
{{#treble}}
    $(LOCAL_PATH)/{{_extra_dir}}/init.npk.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.npk.rc \
{{/treble}}
{{^treble}}
    $(LOCAL_PATH)/{{_extra_dir}}/init.npk.rc:root/init.npk.rc \
{{/treble}}
    $(LOCAL_PATH)/{{_extra_dir}}/npk_{{platform}}.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/npk.cfg
PRODUCT_PACKAGES += \
    npk_init \
    logd2sven

endif #MIXIN_DEBUG_LOGS

# There is a strong dependency on debug-logs; disable debug-npk if not set
ifeq ($(MIXIN_DEBUG_LOGS),true)

# Enable redirection of android logs to SVENTX
LOGCATEXT_USES_SVENTX := true
BOARD_SEPOLICY_DIRS += \
    $(INTEL_PATH_SEPOLICY)/debug-npk

ifeq ($(PSTORE_CONFIG),PRAM)

# Default configuration for dumps to pstore
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.npk.cfg={{default_cfg}}

# Increase pstore dump size to fit MSC buffers
BOARD_KERNEL_CMDLINE += \
    intel_pstore_pram.record_size=524288 \
    pstore.extra_size=524288

endif # PSTORE_CONFIG == PRAM

endif #MIXIN_DEBUG_LOGS
