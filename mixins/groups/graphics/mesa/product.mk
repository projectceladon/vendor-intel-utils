# Mesa
PRODUCT_PACKAGES += \
    libGLES_mesa \
    libGLES_android

PRODUCT_PACKAGES += \
    libdrm \
    libdrm_intel \
    libsync \
    libmd

PRODUCT_PACKAGES += ufo_prebuilts

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/{{_extra_dir}}/drirc:system/etc/drirc

{{#drmhwc}}
# DRM HWComposer
PRODUCT_PACKAGES += \
    hwcomposer.drm

PRODUCT_PROPERTY_OVERRIDES += \
   ro.hardware.hwcomposer=drm
{{/drmhwc}}

{{^drmhwc}}
# HWComposer IA
PRODUCT_PACKAGES += \
    hwcomposer.$(TARGET_BOARD_PLATFORM)

PRODUCT_PROPERTY_OVERRIDES += \
   ro.hardware.hwcomposer=$(TARGET_BOARD_PLATFORM)

INTEL_HWC_CONFIG := $(INTEL_PATH_VENDOR)/external/hwcomposer-intel

ifeq ($(findstring _acrn,$(TARGET_PRODUCT)),_acrn)
PRODUCT_COPY_FILES += $(INTEL_HWC_CONFIG)/hwc_display_virt.ini:$(TARGET_COPY_OUT_VENDOR)/etc/hwc_display.ini
else
PRODUCT_COPY_FILES += $(INTEL_HWC_CONFIG)/hwc_display.ini:$(TARGET_COPY_OUT_VENDOR)/etc/hwc_display.ini
endif

{{/drmhwc}}

{{#minigbm}}
# Mini gbm
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.gralloc=$(TARGET_BOARD_PLATFORM)

PRODUCT_PACKAGES += \
    gralloc.$(TARGET_BOARD_PLATFORM)
{{/minigbm}}

{{^minigbm}}
#Gralloc
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.gralloc=drm

PRODUCT_PACKAGES += \
    gralloc.drm
{{/minigbm}}

{{^gen9+}}
# GLES version. We cannot enable Android
# 3.2 support for Gen9+ devices.
PRODUCT_PROPERTY_OVERRIDES += \
   ro.opengles.version=196609

{{#vulkan}}
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.vulkan.level-0.xml:vendor/etc/permissions/android.hardware.vulkan.level.xml
{{/vulkan}}

{{/gen9+}}

{{#gen9+}}
# Mesa
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.opengles.aep.xml:vendor/etc/permissions/android.hardware.opengles.aep.xml

# GLES version
PRODUCT_PROPERTY_OVERRIDES += \
   ro.opengles.version=196610

{{#vulkan}}
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.vulkan.level-1.xml:vendor/etc/permissions/android.hardware.vulkan.level.xml

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.vulkan.compute-0.xml:vendor/etc/permissions/android.hardware.vulkan.compute.xml
{{/vulkan}}

{{/gen9+}}

{{#vulkan}}
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.vulkan.version-1_1.xml:vendor/etc/permissions/android.hardware.vulkan.version.xml

PRODUCT_PACKAGES += \
    vulkan.$(TARGET_BOARD_PLATFORM) \
    libvulkan_intel

PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.vulkan=$(TARGET_BOARD_PLATFORM)
{{/vulkan}}
