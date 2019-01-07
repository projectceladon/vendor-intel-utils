#-------------------------------------------------------------------------
# INTEL CONFIDENTIAL
#
# Copyright 2017 Intel Corporation All Rights Reserved.
#
# This source code and all documentation related to the source code
# ("Material") contains trade secrets and proprietary and confidential
# information of Intel and its suppliers and licensors. The Material is
# deemed highly confidential, and is protected by worldwide copyright and
# trade secret laws and treaty provisions. No part of the Material may be
# used, copied, reproduced, modified, published, uploaded, posted,
# transmitted, distributed, or disclosed in any way without Intel's prior
# express written permission.
#
# No license under any patent, copyright, trade secret or other
# intellectual property right is granted to or conferred upon you by
# disclosure or delivery of the Materials, either expressly, by
# implication, inducement, estoppel or otherwise. Any license under such
# intellectual property rights must be express and approved by Intel in
# writing.
#-------------------------------------------------------------------------
#
#

PLATFORM_PFW_CONFIG_PATH := $(call my-dir)
#
# Defines all variables required to :
#    - configure mapping field of PFW plugings automatically at compile time
#    - use the specific device/codec structure/settings files.
#
AUDIO_PATTERNS_2 := @SOUND_CARD_NAME@=sklvirtiocard
AVB_PROFILE := eavb-master
AUDIO_AVB_PATTERNS := @AVB_PROFILE@=$(AVB_PROFILE)

LOCAL_PATH := $(PLATFORM_PFW_CONFIG_PATH)

##################################################
## Audio PFW top-level configuration file

include $(CLEAR_VARS)
LOCAL_MODULE := AudioParameterFramework-tdf8532-$(AVB_PROFILE)$(TUNING_SUFFIX).xml
LOCAL_MODULE_STEM := AudioParameterFramework-tdf8532-$(AVB_PROFILE).xml
LOCAL_SRC_FILES := AudioParameterFramework-tdf8532-avb.xml.in
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
REPLACE_PATTERNS += $(AUDIO_AVB_PATTERNS)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

##################################################
## Audio Tuning + Routing

include $(CLEAR_VARS)
LOCAL_MODULE := AudioConfigurableDomains-tdf8532-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Settings/Audio

LOCAL_REQUIRED_MODULES := \
    parameter-framework.audio.common
# Generate tunning + routing domain file for florida-audio-default
LOCAL_ADDITIONAL_DEPENDENCIES := \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/AudioParameterFramework-tdf8532-$(AVB_PROFILE)$(TUNING_SUFFIX).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/AudioClass-tdf8532-$(AVB_PROFILE).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/SstSubsystem-tdf8532.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/SmartXSubsystem-$(AVB_PROFILE).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/eAVB/SmartX/RoutingZonesLibrary-$(AVB_PROFILE).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/eAVB/SmartX/AudioSourceDevicesLibrary-$(AVB_PROFILE).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/eAVB/SmartX/AudioSinkDevicesLibrary-$(AVB_PROFILE).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/ConfigurationSubsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/eAVB/SmartX/ModulesLibrary.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/eAVB/SmartX/PipelineLibrary-amp.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/AvbControlSubsystem-$(AVB_PROFILE).xml

PFW_TOPLEVEL_FILE := $(TARGET_OUT_VENDOR_ETC)/parameter-framework/AudioParameterFramework-tdf8532-$(AVB_PROFILE).xml
PFW_CRITERIA_FILE := $(COMMON_PFW_CONFIG_PATH)/AudioCriteria.txt
PFW_EDD_FILES := \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_sst_tdf8532.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_smartx_$(AVB_PROFILE).pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/calibration_smartx.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_avb_$(AVB_PROFILE).pfw \

include $(BUILD_PFW_SETTINGS)

#################################

include $(CLEAR_VARS)
LOCAL_MODULE := AudioClass-tdf8532-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := Structure/Audio/AudioClass-tdf8532-eavb.xml.in
REPLACE_PATTERNS := $(AUDIO_AVB_PATTERNS)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

include $(CLEAR_VARS)
LOCAL_MODULE := SmartXSubsystem-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := Structure/Audio/SmartXSubsystem.xml.in
LOCAL_REQUIRED_MODULES := \
    SmartXSubsystem-Definitions.xml \
    LocalAudioStreams-$(AVB_PROFILE).xml
REPLACE_PATTERNS := $(AUDIO_AVB_PATTERNS)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

include $(CLEAR_VARS)
LOCAL_MODULE := AudioSourceDevicesLibrary-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio/eAVB/SmartX
LOCAL_SRC_FILES := Structure/Audio/eAVB/SmartX/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := AudioSinkDevicesLibrary-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio/eAVB/SmartX
LOCAL_SRC_FILES := Structure/Audio/eAVB/SmartX/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := RoutingZonesLibrary-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio/eAVB/SmartX
LOCAL_SRC_FILES := Structure/Audio/eAVB/SmartX/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := AvbControlSubsystem-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := Structure/Audio/AvbControlSubsystem.xml.in
LOCAL_REQUIRED_MODULES := \
    AvbControlSubsystem-Definitions.xml \
    LocalAudioStreams-$(AVB_PROFILE).xml \
    NetworkAudioStreams-$(AVB_PROFILE).xml
REPLACE_PATTERNS := $(AUDIO_AVB_PATTERNS)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

include $(CLEAR_VARS)
LOCAL_MODULE := LocalAudioStreams-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio/eAVB/Common
LOCAL_SRC_FILES := Structure/Audio/eAVB/Common/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := NetworkAudioStreams-$(AVB_PROFILE).xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio/eAVB/AVB
LOCAL_SRC_FILES := Structure/Audio/eAVB/AVB/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
