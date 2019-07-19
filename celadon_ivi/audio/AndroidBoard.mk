LOCAL_PATH := $(call my-dir)

# audio hardware is not discoverable, select hardware or use basic default
ifeq ($(INTEL_AUDIO_HAL), stub)

# this option only enables USB, A2DP and remote submix audio.
# the audio.primary.default HAL is part of the build to pass
# CTS tests but only acts as a NULL sink/source

AUDIO_HARDWARE := stub

else
AUDIO_HARDWARE := default
#AUDIO_HARDWARE := PCH-CX20724
# Next configuration is used for Intel NUC6i5SYH
#AUDIO_HARDWARE := PCH-ALC283
#AUDIO_HARDWARE := nuc-skull-canyon
endif

###########################################
# Audio stack Packages
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := audio_configuration_files
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    audio_policy_configuration_files \
    audio_settings_configuration_files

ifeq ($(INTEL_AUDIO_HAL), stub)
LOCAL_REQUIRED_MODULES += audio.stub.default
else
LOCAL_REQUIRED_MODULES += audio.primary.$(TARGET_BOARD_PLATFORM)

ifeq ($(INTEL_AUDIO_HAL),audio_pfw)
LOCAL_REQUIRED_MODULES += audio_hal_configuration_files
endif

endif

include $(BUILD_PHONY_PACKAGE)

###########################################
# Audio Policy Configuration files
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := audio_policy_configuration_files
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    a2dp_audio_policy_configuration.xml \
    r_submix_audio_policy_configuration.xml \
    usb_audio_policy_configuration.xml \
    audio_policy_volumes.xml \
    default_volume_tables.xml \
    audio_policy_configuration.xml

ifeq ($(INTEL_AUDIO_HAL), stub)
LOCAL_REQUIRED_MODULES += stub_audio_policy_configuration.xml
endif

include $(BUILD_PHONY_PACKAGE)

#include $(CLEAR_VARS)
#LOCAL_MODULE := a2dp_audio_policy_configuration.xml
#LOCAL_MODULE_OWNER := intel
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
#LOCAL_SRC_FILES := default/policy/$(LOCAL_MODULE)
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE := audio_policy_volumes.xml
#LOCAL_MODULE_OWNER := intel
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
#LOCAL_SRC_FILES := default/policy/$(LOCAL_MODULE)
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE := default_volume_tables.xml
#LOCAL_MODULE_OWNER := intel
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
#LOCAL_SRC_FILES := default/policy/$(LOCAL_MODULE)
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE := r_submix_audio_policy_configuration.xml
#LOCAL_MODULE_OWNER := intel
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
#LOCAL_SRC_FILES := default/policy/$(LOCAL_MODULE)
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE := usb_audio_policy_configuration.xml
#LOCAL_MODULE_OWNER := intel
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
#LOCAL_SRC_FILES := default/policy/$(LOCAL_MODULE)
#include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := stub_audio_policy_configuration.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := default/policy/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := audio_policy_configuration.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(AUDIO_HARDWARE)/policy/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
###########################################

###########################################
# Audio HAL configuration file
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := audio_hal_configuration_files
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    vendor_audio_criteria.xml \
    vendor_audio_criterion_types.xml

include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := vendor_audio_criteria.xml
LOCAL_MODULE_STEM := audio_criteria.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := default/route/$(LOCAL_MODULE_STEM)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=  vendor_audio_criterion_types.xml
LOCAL_MODULE_STEM :=  audio_criterion_types.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := default/route/$(LOCAL_MODULE_STEM)
include $(BUILD_PREBUILT)
###########################################

###########################################
# Audio HAL Custom configuration files
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := audio_settings_configuration_files
LOCAL_MODULE_TAGS := optional
ifeq ($(INTEL_AUDIO_HAL),audio_pfw)
LOCAL_REQUIRED_MODULES := audio_parameter_framework
else
LOCAL_REQUIRED_MODULES := mixer_paths_0.xml
endif
include $(BUILD_PHONY_PACKAGE)

ifeq ($(INTEL_AUDIO_HAL),audio_pfw)
include audio/$(AUDIO_HARDWARE)/AndroidBoard.mk
else
include $(CLEAR_VARS)
LOCAL_MODULE := mixer_paths_0.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(AUDIO_HARDWARE)/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
endif
###########################################

ifeq ($(USE_CONFIGURABLE_AUDIO_POLICY), 1)
include $(TARGET_DEVICE_DIR)/audio/reference_configurable_audio_policy/AndroidBoard.mk
endif
