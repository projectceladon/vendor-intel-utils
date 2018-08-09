LOCAL_PATH := $(call my-dir)
PFW_CORE := external/parameter-framework
BUILD_PFW_SETTINGS := $(PFW_CORE)/support/android/build_pfw_settings.mk
PFW_DEFAULT_SCHEMAS_DIR := $(PFW_CORE)/upstream/schemas
PFW_SCHEMAS_DIR := $(PFW_DEFAULT_SCHEMAS_DIR)

###########################################
# Audio stack Package
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := audio_parameter_framework
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    libremote-processor \
    remote-process \
    AudioConfigurableDomains.xml \
    topology.audio.$(TARGET_DEVICE)

include $(BUILD_PHONY_PACKAGE)


###########################################
# Audio Tuning Tool Configuration file
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := topology.audio.$(TARGET_DEVICE)
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    layout_types.xml \
    layout_topology.xml \
    layout_ui.xml \
    layout_probes.xml

include $(BUILD_PHONY_PACKAGE)


include $(CLEAR_VARS)
LOCAL_MODULE := layout_types.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := tuning/audio
LOCAL_SRC_FILES := topology/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := layout_topology.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := tuning/audio
LOCAL_SRC_FILES := topology/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := layout_ui.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := tuning/audio
LOCAL_SRC_FILES := topology/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := layout_probes.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := tuning/audio
LOCAL_SRC_FILES := topology/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
###########################################

###########################################
# Audio PFW Package
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := AudioConfigurableDomains.xml
LOCAL_MODULES_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := \
    AudioParameterFramework.xml \
    AudioClass.xml \
    RealtekAlc283Subsystem.xml \
    HdmiSubsystem.xml

LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Settings/Audio
LOCAL_REQUIRED_MODULES := \
    libtinyalsa-subsystem

PFW_TOPLEVEL_FILE := $(TARGET_OUT_ETC)/parameter-framework/AudioParameterFramework.xml
PFW_CRITERIA_FILE := $(LOCAL_PATH)/parameter-framework/AudioCriteria.txt
PFW_TUNING_FILE   := $(LOCAL_PATH)/$(LOCAL_MODULE_RELATIVE_PATH)/AudioConfigurableDomains-Tuning.xml

PFW_EDD_FILES := \
    $(LOCAL_PATH)/$(LOCAL_MODULE_RELATIVE_PATH)/routing_alc283.pfw \
    $(LOCAL_PATH)/$(LOCAL_MODULE_RELATIVE_PATH)/routing_hdmi.pfw

include $(BUILD_PFW_SETTINGS)

###########################################
# Audio PFW top file
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := AudioParameterFramework.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := parameter-framework
LOCAL_SRC_FILES := parameter-framework/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
###########################################

###########################################
# Audio PFW Structure files
###########################################
include $(CLEAR_VARS)
LOCAL_MODULE := AudioClass.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := $(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := RealtekAlc283Subsystem.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := $(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := HdmiSubsystem.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := $(LOCAL_MODULE_RELATIVE_PATH)/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
###########################################
