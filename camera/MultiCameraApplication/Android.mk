LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_SDK_VERSION := current
LOCAL_MIN_SDK_VERSION := 27

LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/res
LOCAL_SRC_FILES := \
	$(call all-java-files-under, java)
LOCAL_DEX_PREOPT := false
LOCAL_PACKAGE_NAME := MultiCameraApp

include $(BUILD_PACKAGE)

include $(call all-makefiles-under, $(LOCAL_PATH))
