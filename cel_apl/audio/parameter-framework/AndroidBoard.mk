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

AUDIO_PATTERNS := @VIBRATOR_DEVICE@=/sys/devices/platform/INT34E1:00

# The file included below allows to set or not the tuning flags
# according to the type of build.
include $(INTEL_PATH_COMMON)/audio/parameter-framework/AndroidBoard.mk

include $(PLATFORM_PFW_CONFIG_PATH)/AndroidBoard.wm8281.mk
include $(PLATFORM_PFW_CONFIG_PATH)/AndroidBoard.wm8998.mk
include $(PLATFORM_PFW_CONFIG_PATH)/AndroidBoard.tdf8532.common.mk
include $(PLATFORM_PFW_CONFIG_PATH)/AndroidBoard.tdf8532.eavb.master.mk
include $(PLATFORM_PFW_CONFIG_PATH)/AndroidBoard.tdf8532.eavb.master.raw.mk
include $(PLATFORM_PFW_CONFIG_PATH)/AndroidBoard.tdf8532.eavb.slave.mk
include $(PLATFORM_PFW_CONFIG_PATH)/AndroidBoard.tdf8532.no.eavb.mk

LOCAL_PATH := $(PLATFORM_PFW_CONFIG_PATH)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.broxton
LOCAL_MODULE_TAGS := optional
# LOCAL_ADDITIONAL_DEPENDENCIES is used to ensure that the domains will be
# rebuilt if needed (LOCAL_REQUIRED_MODULES doesn't ensure that).
LOCAL_ADDITIONAL_DEPENDENCIES := \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Audio/AudioConfigurableDomains-wm8281.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Audio/AudioConfigurableDomains-wm8998.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Audio/AudioConfigurableDomains-tdf8532-eavb-master.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Audio/AudioConfigurableDomains-tdf8532-eavb-master-raw.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Audio/AudioConfigurableDomains-tdf8532-eavb-slave.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Audio/AudioConfigurableDomains-tdf8532-no-eavb.xml
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.route.broxton
LOCAL_MODULE_TAGS := optional
# LOCAL_ADDITIONAL_DEPENDENCIES is used to ensure that the domains will be
# rebuilt if needed (LOCAL_REQUIRED_MODULES doesn't ensure that).
LOCAL_ADDITIONAL_DEPENDENCIES := \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Route/RouteConfigurableDomains-wm8281.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Route/RouteConfigurableDomains-wm8998.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Settings/Route/RouteConfigurableDomains-tdf8532.xml
include $(BUILD_PHONY_PACKAGE)

######### Audio Structures #########

include $(CLEAR_VARS)
LOCAL_MODULE := SstSubsystem-common.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := Structure/Audio/$(LOCAL_MODULE)
LOCAL_REQUIRED_MODULES :=  \
    CommonAlgoTypes.xml \
    cavs-Gain.xml \
    cavs-Dcr.xml \
    cavs-Fir.xml \
    cavs-Iir.xml \
    cavs-Mdrc.xml \
    cavs-Equalizers.xml \
    cavs-Ghwd.xml \
    cavs-Lpro.xml

include $(BUILD_PREBUILT)

######### Policy PFW top level file #########

include $(CLEAR_VARS)
LOCAL_MODULE := ParameterFrameworkConfigurationPolicy$(TUNING_SUFFIX).xml
LOCAL_MODULE_STEM := ParameterFrameworkConfigurationPolicy.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM).in
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

############ Vibrator ##########

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.vibrator.broxton
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
        parameter-framework.vibrator.common \
        VibratorConfigurableDomains.xml
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := VibratorConfigurableDomains.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Settings/Vibrator
LOCAL_SRC_FILES := Settings/Vibrator/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################
## DBGA PFW top-level configuration file

include $(CLEAR_VARS)
LOCAL_MODULE := fdk-modules-structure
LOCAL_MODULE_STEM := DbgaParameterFramework.xml
LOCAL_SRC_FILES := DbgaParameterFramework.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework
LOCAL_REQUIRED_MODULES := \
        cAVSSubsystem.xml \
        SystemClass.xml \
        ampli2_013CF859-6F0D-4EC1-B211-B302D416617A_1.0.0.xml
include $(BUILD_PREBUILT)
##################################################
include $(CLEAR_VARS)
LOCAL_MODULE := SystemClass.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/DBGA
LOCAL_SRC_FILES := Structure/DBGA/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################
include $(CLEAR_VARS)
LOCAL_MODULE := cAVSSubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/DBGA
LOCAL_SRC_FILES := Structure/DBGA/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################
include $(CLEAR_VARS)
LOCAL_MODULE := ampli2_013CF859-6F0D-4EC1-B211-B302D416617A_1.0.0.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/DBGA/modules
LOCAL_SRC_FILES := Structure/DBGA/modules/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
