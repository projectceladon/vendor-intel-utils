.PHONY: lk evmm tosimage multiboot

LOCAL_MAKE := make

lk:
	@echo "making lk.elf.."
	$(hide) (cd $(TOPDIR)trusty && $(TRUSTY_ENV_VAR) $(LOCAL_MAKE) {{{lk_project}}})

evmm: yoctotoolchain
	@echo "making evmm.."
	$(hide) (cd $(TOPDIR)$(INTEL_PATH_VENDOR)/fw/evmm && $(TRUSTY_ENV_VAR) $(LOCAL_MAKE))

# include sub-makefile according to boot_arch
include $(TARGET_DEVICE_DIR)/{{_extra_dir}}/trusty_{{boot-arch}}.mk

LOAD_MODULES_H_IN += $(TARGET_DEVICE_DIR)/{{_extra_dir}}/load_trusty_modules.in
