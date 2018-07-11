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
AUDIO_PATTERNS_2 := @SOUND_CARD_NAME@=broxtontdf8532

LOCAL_PATH := $(PLATFORM_PFW_CONFIG_PATH)

##################################################
## Route PFW top-level configuration file

include $(CLEAR_VARS)
LOCAL_MODULE := RouteParameterFramework-tdf8532$(TUNING_SUFFIX).xml
LOCAL_MODULE_STEM := RouteParameterFramework-tdf8532.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM).in
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

######### Route Structures #########

include $(CLEAR_VARS)
LOCAL_MODULE := RouteClass-tdf8532.xml
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Route
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := Structure/Route/RouteClass-tdf8532.xml.in
LOCAL_REQUIRED_MODULES := \
        RouteSubsystem-CommonCriteria.xml \
        RouteSubsystem-RoutesTypes.xml \
        DebugFsSubsystem.xml \
        libroute-subsystem
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

######### Route Settings #########

include $(CLEAR_VARS)
LOCAL_MODULE := RouteConfigurableDomains-tdf8532.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Settings/Route

# Refresh domain file
LOCAL_ADDITIONAL_DEPENDENCIES := \
        $(TARGET_OUT_VENDOR_ETC)/parameter-framework/RouteParameterFramework-tdf8532$(TUNING_SUFFIX).xml \
        $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Route/RouteClass-tdf8532.xml

PFW_TOPLEVEL_FILE := $(TARGET_OUT_VENDOR_ETC)/parameter-framework/RouteParameterFramework-tdf8532.xml
PFW_CRITERIA_FILE := $(COMMON_PFW_CONFIG_PATH)/RouteCriteria.txt
PFW_EDD_FILES := \
    $(COMMON_PFW_CONFIG_PATH)/Settings/Route/parameters.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Route/routes-applicability-bxtp.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Route/routes-configuration-bxtp.pfw
include $(BUILD_PFW_SETTINGS)

#################################

include $(CLEAR_VARS)
LOCAL_MODULE := SstSubsystem-tdf8532.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := Structure/Audio/SstSubsystem-tdf8532.xml.in
LOCAL_REQUIRED_MODULES := \
    libtinyalsa_custom-subsystem
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

include $(CLEAR_VARS)
LOCAL_MODULE := PipelineLibrary-amp.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio/eAVB/SmartX
LOCAL_SRC_FILES := Structure/Audio/eAVB/SmartX/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
