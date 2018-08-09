ifneq ($(TARGET_BUILD_VARIANT),user)
{{#treble}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/init.kernel.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.kernel.rc
{{/treble}}
{{^treble}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/init.kernel.rc:root/init.kernel.rc
{{/treble}}
endif
