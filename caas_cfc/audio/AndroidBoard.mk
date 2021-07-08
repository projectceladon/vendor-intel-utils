LOCAL_PATH := $(call my-dir)

# audio hardware is not discoverable, select hardware or use basic default
ifeq ($(INTEL_AUDIO_HAL), stub)
AUDIO_HARDWARE := stub
else
AUDIO_HARDWARE := default
endif

ifeq ($(INTEL_AUDIO_HAL), stub)
LOCAL_REQUIRED_MODULES += audio.stub.default
else
LOCAL_REQUIRED_MODULES += audio.primary.$(TARGET_BOARD_PLATFORM)
endif

ifeq ($(INTEL_AUDIO_HAL), stub)
LOCAL_REQUIRED_MODULES += stub_audio_policy_configuration.xml
endif

include $(CLEAR_VARS)
LOCAL_MODULE := stub_audio_policy_configuration.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := default/policy/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(USE_CONFIGURABLE_AUDIO_POLICY), 1)
include $(TARGET_DEVICE_DIR)/audio/reference_configurable_audio_policy/AndroidBoard.mk
endif
