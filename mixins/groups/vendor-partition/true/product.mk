{{^avb}}
PRODUCT_VENDOR_VERITY_PARTITION := /dev/block/by-name/{{partition_name}}
{{/avb}}

PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/sh_recovery:recovery/root/vendor/bin/sh
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/mkshrc_recovery:recovery/root/vendor/etc/mkshrc

PRODUCT_PACKAGES += \
     toybox_static \
     toybox_vendor \
