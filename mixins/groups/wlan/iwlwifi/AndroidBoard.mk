{{#iwl_upstream_drv}}
LOAD_MODULES_IN += $(TARGET_DEVICE_DIR)/{{_extra_dir}}/load_legacy_iwlwifi.in
{{/iwl_upstream_drv}}
{{^iwl_upstream_drv}}
LOCAL_KERNEL_PATH := $(abspath $(PRODUCT_OUT)/obj/kernel) is not defined yet
$(abspath $(PRODUCT_OUT)/obj/kernel)/copy_modules: iwlwifi
LOAD_MODULES_IN += $(TARGET_DEVICE_DIR)/{{_extra_dir}}/load_iwlwifi.in
{{/iwl_upstream_drv}}
