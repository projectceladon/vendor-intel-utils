LOCAL_KERNEL_PATH := $(abspath $(PRODUCT_OUT)/obj/kernel) is not defined yet
$(abspath $(PRODUCT_OUT)/obj/kernel)/copy_modules: iwlwifi

LOAD_MODULES_IN += $(TARGET_DEVICE_DIR)/{{_extra_dir}}/load_iwlwifi.in
