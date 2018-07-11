LOCAL_PATH := $(call my-dir)

# Copy the BXP-P LPE FW Binary(for 4.1 kernel)

include $(CLEAR_VARS)
LOCAL_MODULE := dsp_fw_bxtp_release.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_MODULE_STEM := dsp_fw_release.bin
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := dsp_fw_bxtp_release.bin
include $(BUILD_PREBUILT)

# Copy the BXP-P LPE FW Binary(for 4.4 kernel)

include $(CLEAR_VARS)
LOCAL_MODULE := dsp_fw_bxtp_release_k44.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware/intel
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_MODULE_STEM := dsp_fw_bxtn.bin
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := dsp_fw_bxtp_release.bin
include $(BUILD_PREBUILT)

# Copy the BXP-P DFW Binary(for 4.1 kernel)

include $(CLEAR_VARS)
LOCAL_MODULE := dfw_sst_bxtp.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_MODULE_STEM := dfw_sst.bin
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := dfw_sst_bxtp.bin
include $(BUILD_PREBUILT)

# Copy the BXP-P DFW Binary (for kernel 4.4)
#
include $(CLEAR_VARS)
LOCAL_MODULE := 5a98-INTEL-NHLT-GPA-3-tplg.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_MODULE_STEM := 5a98-INTEL-NHLT-GPA-3-tplg.bin
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := 5a98-INTEL-NHLT-GPA-3-tplg.bin
include $(BUILD_PREBUILT)

##################################################

# Copy the BXP-M NHLT blob Binary

include $(CLEAR_VARS)
LOCAL_MODULE := nhlt_blob.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_MODULE_STEM := nhlt_blob.bin
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := nhlt_blob.bin
include $(BUILD_PREBUILT)

##################################################

# Copy the BXP-P NHLT blob Binary for playback

include $(CLEAR_VARS)
LOCAL_MODULE := sspGpMrbAmp4ch.blob
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := sspGpMrbAmp4ch.blob
include $(BUILD_PREBUILT)

# Copy the BXP-P amplifier Binary

include $(CLEAR_VARS)
LOCAL_MODULE := dsp_lib_fdk_amp.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_MODULE_STEM := dsp_lib_fdk_amp.bin
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := libamp_noextmft.bin
include $(BUILD_PREBUILT)

# Copy the BXP-P ASRC Binary

include $(CLEAR_VARS)
LOCAL_MODULE := LIBASRC.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_MODULE_STEM := LIBASRC.bin
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := lib_asrc.bin
include $(BUILD_PREBUILT)

# Copy the BXP-P NHLT blob Binary for capture

include $(CLEAR_VARS)
LOCAL_MODULE := sspGpMrbTuner.blob
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := sspGpMrbTuner.blob
include $(BUILD_PREBUILT)

# Copy the BXP-P NHLT blob Binary for BT HFP

include $(CLEAR_VARS)
LOCAL_MODULE := sspGpMrbBtHfp.blob
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := sspGpMrbBtHfp.blob
include $(BUILD_PREBUILT)

# Copy the BXP-P NHLT blob Binary for Modem

include $(CLEAR_VARS)
LOCAL_MODULE := sspGpMrbModem.blob
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_RELATIVE_PATH := firmware
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := sspGpMrbModem.blob
include $(BUILD_PREBUILT)
