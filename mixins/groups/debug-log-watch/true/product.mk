ifneq ($(TARGET_BUILD_VARIANT),user)
ifeq ($(MIXIN_DEBUG_LOGS),true)
    PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/{{_extra_dir}}/log-watch-kmsg.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/log-watch-kmsg.cfg \
{{#treble}}
        $(LOCAL_PATH)/{{_extra_dir}}/init.log-watch.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.log-watch.rc
{{/treble}}
{{^treble}}
        $(LOCAL_PATH)/{{_extra_dir}}/init.log-watch.rc:root/init.log-watch.rc
{{/treble}}

    PRODUCT_PACKAGES += log-watch
endif
endif

