include $(CLEAR_VARS)
LOCAL_MODULE := vendor-partition
LOCAL_REQUIRED_MODULES := toybox_static
include $(BUILD_PHONY_PACKAGE)

RECOVERY_VENDOR_LINK_PAIRS := \
	$(PRODUCT_OUT)/recovery/root/vendor/bin/getprop:toolbox_static \

RECOVERY_VENDOR_LINKS := \
	$(foreach item, $(RECOVERY_VENDOR_LINK_PAIRS), $(call word-colon, 1, $(item)))

$(RECOVERY_VENDOR_LINKS):
	$(hide) echo "Creating symbolic link on $(notdir $@)"
	$(eval PRV_TARGET := $(call word-colon, 2, $(filter $@:%, $(RECOVERY_VENDOR_LINK_PAIRS))))
	$(hide) mkdir -p $(dir $@)
	$(hide) mkdir -p $(dir $(dir $@)$(PRV_TARGET))
	$(hide) touch $(dir $@)$(PRV_TARGET)
	$(hide) ln -sf $(PRV_TARGET) $@

ALL_DEFAULT_INSTALLED_MODULES += $(RECOVERY_VENDOR_LINKS)
