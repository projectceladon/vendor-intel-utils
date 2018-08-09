ifneq ($(TARGET_BUILD_VARIANT),user)
ifeq ($(MIXIN_DEBUG_LOGS),true)
    BOARD_SEPOLICY_DIRS += $(INTEL_PATH_SEPOLICY)/log-watch
endif
endif

