ifeq ($(MIXIN_DEBUG_LOGS),true)
BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/crashlogd
BOARD_SEPOLICY_M4DEFS += module_debug_crashlogd=true
endif
