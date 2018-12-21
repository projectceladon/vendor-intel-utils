PLATFORM_PFW_CONFIG_PATH := $(call my-dir)
#
# Defines all variables required to :
#    - configure mapping field of PFW plugings automatically at compile time
#    - use the specific device/codec structure/settings files.
#
AUDIO_PATTERNS_2 := @SOUND_CARD_NAME@=wm8998audio

LOCAL_PATH := $(PLATFORM_PFW_CONFIG_PATH)

##################################################
## Audio PFW top-level configuration file

include $(CLEAR_VARS)
LOCAL_MODULE := AudioParameterFramework-wm8998$(TUNING_SUFFIX).xml
LOCAL_MODULE_STEM := AudioParameterFramework-wm8998.xml
ifeq ($(BOARD_HAVE_MODEM),true)
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM).in
else
LOCAL_SRC_FILES := AudioParameterFramework-wm8998-nomodem.xml.in
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

##################################################
## Route PFW top-level configuration file

include $(CLEAR_VARS)
LOCAL_MODULE := RouteParameterFramework-wm8998$(TUNING_SUFFIX).xml
LOCAL_MODULE_STEM := RouteParameterFramework-wm8998.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE_STEM).in
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

##################################################
## Audio Tuning + Routing

include $(CLEAR_VARS)
LOCAL_MODULE := AudioConfigurableDomains-wm8998.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Settings/Audio

LOCAL_REQUIRED_MODULES := \
    parameter-framework.audio.common
# Generate tunning + routing domain file for florida-audio-default
ifeq ($(BOARD_HAVE_MODEM),true)
LOCAL_ADDITIONAL_DEPENDENCIES := \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/AudioParameterFramework-wm8998$(TUNING_SUFFIX).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/AudioClass-wm8998.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/SstSubsystem-wm8998.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/WM8998Subsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/IMCSubsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/ConfigurationSubsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/PowerSubsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/BluedroidCommSubsystem.xml
else
LOCAL_ADDITIONAL_DEPENDENCIES := \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/AudioParameterFramework-wm8998$(TUNING_SUFFIX).xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/AudioClass-wm8998.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/SstSubsystem-wm8998.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/WM8998Subsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/ConfigurationSubsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/PowerSubsystem.xml \
    $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Audio/BluedroidCommSubsystem.xml
endif
PFW_TOPLEVEL_FILE := $(TARGET_OUT_VENDOR_ETC)/parameter-framework/AudioParameterFramework-wm8998.xml
PFW_CRITERIA_FILE := $(COMMON_PFW_CONFIG_PATH)/AudioCriteria.txt
ifeq ($(BOARD_HAVE_MODEM),true)
PFW_TUNING_FILE := $(LOCAL_PATH)/Settings/Audio/AudioConfigurableDomains-wm8998.Tuning.xml
PFW_EDD_FILES := \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_hwcodec_common_wm8998.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_sst_common.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_imc_common.pfw \
    $(COMMON_PFW_CONFIG_PATH)/Settings/Audio/routing_power.pfw \
    $(COMMON_PFW_CONFIG_PATH)/Settings/Audio/routing_a2dp_offload.pfw
else
PFW_TUNING_FILE := $(LOCAL_PATH)/Settings/Audio/AudioConfigurableDomains-wm8998-nomodem.Tuning.xml
PFW_EDD_FILES := \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_hwcodec_common_wm8998.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Audio/routing_sst_common.pfw \
    $(COMMON_PFW_CONFIG_PATH)/Settings/Audio/routing_power.pfw \
    $(COMMON_PFW_CONFIG_PATH)/Settings/Audio/routing_a2dp_offload.pfw
endif
PFW_DOMAIN_FILES := \
    $(LOCAL_PATH)/Settings/Audio/cavs/MediaEffectLoop0.Equalizer.xml \
    $(LOCAL_PATH)/Settings/Audio/cavs/MediaEffectLoop0.Mdrc.xml \
    $(LOCAL_PATH)/Settings/Audio/cavs/GhwdSettings.xml \
    $(LOCAL_PATH)/Settings/Audio/cavs/MediaEffectLoop3.Lpro.xml

include $(BUILD_PFW_SETTINGS)

######### Route Structures #########

include $(CLEAR_VARS)
LOCAL_MODULE := RouteClass-wm8998.xml
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Route
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := Structure/Route/RouteClass.xml.in
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
LOCAL_MODULE := RouteConfigurableDomains-wm8998.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Settings/Route

# Refresh domain file
LOCAL_ADDITIONAL_DEPENDENCIES := \
        $(TARGET_OUT_VENDOR_ETC)/parameter-framework/RouteParameterFramework-wm8998$(TUNING_SUFFIX).xml \
        $(TARGET_OUT_VENDOR_ETC)/parameter-framework/Structure/Route/RouteClass-wm8998.xml

PFW_TOPLEVEL_FILE := $(TARGET_OUT_VENDOR_ETC)/parameter-framework/RouteParameterFramework-wm8998.xml
PFW_CRITERIA_FILE := $(COMMON_PFW_CONFIG_PATH)/RouteCriteria.txt
PFW_EDD_FILES := \
    $(COMMON_PFW_CONFIG_PATH)/Settings/Route/parameters.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Route/routes-applicability.pfw \
    $(PLATFORM_PFW_CONFIG_PATH)/Settings/Route/routes-configuration.pfw
include $(BUILD_PFW_SETTINGS)

#################################

include $(CLEAR_VARS)
LOCAL_MODULE := AudioClass-wm8998.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
ifeq ($(BOARD_HAVE_MODEM),true)
LOCAL_SRC_FILES := Structure/Audio/AudioClass-wm8998.xml
else
LOCAL_SRC_FILES := Structure/Audio/AudioClass-wm8998-nomodem.xml
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := WM8998Subsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := Structure/Audio/$(LOCAL_MODULE).in
LOCAL_REQUIRED_MODULES := \
    WM8998Subsystem-common.xml \
    libtinyalsa-subsystem
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)

include $(CLEAR_VARS)
LOCAL_MODULE := SstSubsystem-wm8998.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := parameter-framework/Structure/Audio
LOCAL_SRC_FILES := Structure/Audio/SstSubsystem.xml.in
LOCAL_REQUIRED_MODULES := \
    SstSubsystem-common.xml \
    libtinyalsa_custom-subsystem
REPLACE_PATTERNS := $(AUDIO_PATTERNS)
REPLACE_PATTERNS += $(AUDIO_PATTERNS_2)
include $(BUILD_REPLACE_PATTERNS_IN_FILE)
