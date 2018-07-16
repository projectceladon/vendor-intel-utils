LOCAL_PATH := $(call my-dir)

##################################################
# Audio policy files

include $(CLEAR_VARS)
LOCAL_MODULE := audio_policy_configuration.xml
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_OWNER := intel
LOCAL_REQUIRED_MODULES := \
    audio_policy_volumes.xml \
    default_volume_tables.xml \
    r_submix_audio_policy_configuration.xml \
    usb_audio_policy_configuration.xml \
    a2dp_audio_policy_configuration.xml
include $(BUILD_PREBUILT)

##################################################
# The audio meta package

include $(CLEAR_VARS)
LOCAL_MODULE := meta.package.audio
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES := \
    audio_hal_configurable \
    audio_policy_configuration.xml \
    parameter-framework.audio.broxton \
    parameter-framework.route.broxton \
    audio.r_submix.default \
    audio.usb.default \
    dsp_fw_bxtp_release.bin \
    dsp_fw_bxtp_release_k44.bin \
    dsp_lib_fdk_amp.bin \
    dfw_sst_bxtp.bin \
    sspGpMrbAmp4ch.blob \
    sspGpMrbTuner.blob \
    sspGpMrbModem.blob \
    dirana_config.sh \
    sspGpMrbBtHfp.blob \
    5a98-INTEL-NHLT-GPA-3-tplg.bin

include $(BUILD_PHONY_PACKAGE)
