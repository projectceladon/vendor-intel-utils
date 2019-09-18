LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := MultiCameraApp
LOCAL_MODULE_TAGS  := optional
LOCAL_DEX_PREOPT   := false
LOCAL_CERTIFICATE  := platform
LOCAL_SDK_VERSION  := current
LOCAL_MIN_SDK_VERSION := 27

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res \

LOCAL_SRC_FILES := $(call all-java-files-under, java)


LOCAL_AIDL_INCLUDES := \
    frameworks/native/aidl/gui

#LOCAL_PROGUARD_FLAG_FILES := ../../../frameworks/support/design/proguard-rules.pro

LOCAL_USE_AAPT2 := true

LOCAL_STATIC_JAVA_LIBRARIES = \
    androidx-constraintlayout_constraintlayout-solver

LOCAL_PROPRIETARY_MODULE := true

LOCAL_STATIC_ANDROID_LIBRARIES := \
    androidx-constraintlayout_constraintlayout \
    androidx.preference_preference \
    androidx.cardview_cardview \
    com.google.android.material_material \
    androidx.legacy_legacy-support-v13 \
    androidx.legacy_legacy-support-v4 \
    androidx.appcompat_appcompat

include $(BUILD_PACKAGE)
