include $(CLEAR_VARS)
LOCAL_MODULE := sepolicy-areq-checker
LOCAL_REQUIRED_MODULES := sepolicy

#
# On user builds, enforce that open tickets are considered violations.
#
ifeq ($(TARGET_BUILD_VARIANT),user)
LOCAL_USER_OPTIONS := -i
endif

LOCAL_POST_INSTALL_CMD := $(INTEL_PATH_SEPOLICY)/tools/capchecker $(LOCAL_USER_OPTIONS) -p $(INTEL_PATH_SEPOLICY)/tools/caps.conf $(TARGET_ROOT_OUT)/sepolicy

include $(BUILD_PHONY_PACKAGE)
