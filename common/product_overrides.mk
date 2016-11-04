# set SELinux property value
#PRODUCT_PROPERTY_OVERRIDES += \
#   ro.build.selinux=1

PRODUCT_PROPERTY_OVERRIDES := \
    ro.com.android.dateformat=MM-dd-yyyy \

# please modify to appropriate value based on tuning
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hwui.texture_cache_size=24.0f \
    ro.hwui.text_large_cache_width=2048 \
    ro.hwui.text_large_cache_height=512

PRODUCT_PROPERTY_OVERRIDES += \
    ro.ril.hsxpa=1 \
    ro.ril.gprsclass=10 \
    keyguard.no_require_sim=true \
    ro.com.android.dataroaming=true

PRODUCT_DEFAULT_PROPERTY_OVERRIDES := \
    ro.arch=x86 \
    persist.rtc_local_time=1 \

# Enable MultiWindow
PRODUCT_PROPERTY_OVERRIDES += \
    persist.sys.debug.multi_window=true

# ADBoverWIFI
PRODUCT_PROPERTY_OVERRIDES += \
    service.adb.tcp.port=5555

# DRM service
PRODUCT_PROPERTY_OVERRIDES += \
    drm.service.enabled=true

# Property to choose between virtual/external wfd display
PRODUCT_PROPERTY_OVERRIDES += \
    persist.sys.wfd.virtual=0

# Camera
PRODUCT_PROPERTY_OVERRIDES += \
    camera.disable_zsl_mode=0 \
    persist.camera.HAL3.enabled=0 \
    persist.camera.ois.disable=0

# Camera: Format set up for graphics
PRODUCT_PROPERTY_OVERRIDES += ro.camera.pixel_format = 0x10F
PRODUCT_PROPERTY_OVERRIDES += ro.camera.rec.pixel_format = 0x100
PRODUCT_PROPERTY_OVERRIDES += ro.ycbcr.pixel_format = 0x10F

# Input resampling configuration
PRODUCT_PROPERTY_OVERRIDES += \
    ro.input.noresample=1

# set default USB configuration
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

# Audio Configuration
PRODUCT_PROPERTY_OVERRIDES += \
    persist.audio.handset.mic.type=digital \
    persist.audio.dualmic.config=endfire \
    persist.audio.fluence.voicecall=false \
    persist.audio.fluence.voicecomm=true \
    persist.audio.fluence.voicerec=true \
    persist.audio.fluence.speaker=true

# Audio logging property for audio driver
ifeq ($(TARGET_BUILD_VARIANT),eng)
PRODUCT_PROPERTY_OVERRIDES += persist.audio.log=2
else ifeq ($(TARGET_BUILD_VARIANT),userdebug)
PRODUCT_PROPERTY_OVERRIDES += persist.audio.log=1
else
PRODUCT_PROPERTY_OVERRIDES += persist.audio.log=0
endif

#Enable low latency stream
PRODUCT_PROPERTY_OVERRIDES += persist.audio.low_latency=1

#Enable deep buffer for video playback
PRODUCT_PROPERTY_OVERRIDES += media.stagefright.audio.deep=true

# Enable AAC 5.1 output
PRODUCT_PROPERTY_OVERRIDES += \
    media.aac_51_output_enabled=true

# Gralloc
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.gralloc android_ia \
    ro.setupwizard.mode ENABLED

# HW Composer
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.hwcomposer android_ia

# Houdini
PRODUCT_PROPERTY_OVERRIDES += \
   ro.dalvik.vm.isa.arm=x86 \
   ro.enable.native.bridge.exec=1

ifeq ($(ENABLE_NATIVEBRIDGE_64BIT),true)
  PRODUCT_PROPERTY_OVERRIDES += \
  ro.dalvik.vm.isa.arm64=x86_64 \
  ro.enable.native.bridge.exec64=1
endif

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
   ro.dalvik.vm.native.bridge=libhoudini.so

# Ethernet
PRODUCT_PROPERTY_OVERRIDES += \
   net.eth0.startonboot=1

# GLES version
PRODUCT_PROPERTY_OVERRIDES += \
   ro.opengles.version=196609 \
   ro.sf.lcd_density=213 \

# HWC
PRODUCT_PROPERTY_OVERRIDES += \
   hwc.drm.use_overlay_planes=1
