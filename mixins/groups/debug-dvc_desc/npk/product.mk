ifneq ($(TARGET_BUILD_VARIANT),user)
PRODUCT_PACKAGES += dvc_desc
PRODUCT_COPY_FILES += \
{{#treble}}
        $(LOCAL_PATH)/{{_extra_dir}}/init.dvc_desc.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.dvc_desc.rc \
{{/treble}}
{{^treble}}
        $(LOCAL_PATH)/{{_extra_dir}}/init.dvc_desc.rc:root/init.dvc_desc.rc \
{{/treble}}
        $(LOCAL_PATH)/{{_extra_dir}}/dvc_descriptors.cfg:$(TARGET_COPY_OUT_VENDOR)/etc/dvc_descriptors.cfg
endif

