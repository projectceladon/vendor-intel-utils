mixin_update := $(wildcard device/intel/mixins/mixin-update)

ifeq ($(mixin_update),)
mixin_update := $(wildcard $(TARGET_DEVICE_DIR)/mixins/mixin-update)
endif

ifneq ($(mixin_update),)

.PHONY: check-mixins
check-mixins:
	$(mixin_update) --dry-run -s $(TARGET_DEVICE_DIR)/mixins.spec

droidcore: check-mixins
flashfiles: check-mixins

endif

