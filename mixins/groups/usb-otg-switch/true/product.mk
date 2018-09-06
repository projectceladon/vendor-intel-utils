{{#ioc}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/usb_otg_switch_{{ioc}}.sh:vendor/bin/usb_otg_switch.sh
{{/ioc}}

{{^ioc}}
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/usb_otg_switch.sh:vendor/bin/usb_otg_switch.sh
{{/ioc}}
