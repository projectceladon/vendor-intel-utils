# AOSP Packages
PRODUCT_PACKAGES += \
    Launcher3 \
    Terminal

PRODUCT_PACKAGES += \
    libion \
    libxml2

PRODUCT_PACKAGES += \
    libemoji

PRODUCT_PACKAGES += \
    LiveWallpapers \
    LiveWallpapersPicker \
    NotePad \
    Provision \
    camera.android_ia \
    drmserver \
    gps.default \
    hwcomposer.drm \
    libGLES_android \
    lights.default \
    power.android_ia \
    scp \
    sensors.hsb \
    sftp \
    ssh \
    sshd \
    gralloc.android_ia \
    libGLES_mesa

PRODUCT_PACKAGES += \
    libwpa_client \
    hostapd \
    wificond \
    wpa_supplicant \
    wpa_supplicant.conf

# Sensors
PRODUCT_PACKAGES += \
    sensorhubd      \
    libsensorhub    \
    AndroidCalibrationTool \

# USB
PRODUCT_PACKAGES += \
    com.android.future.usb.accessory

# Audio
PRODUCT_PACKAGES += \
    audio.r_submix.default \
    libauddriver \
    tinyplay \
    tinymix \
    tinycap \
    audio.primary.android_ia \
    audio.hdmi.android_ia \
    audio.a2dp.default \
    audio.usb.default \
    audio_policy.default \
    audio.r_submix.default \
    libasound

# Houdini
PRODUCT_PACKAGES += \
    libhoudini        \
    houdini

ifeq ($(ENABLE_NATIVEBRIDGE_64BIT),true)
PRODUCT_PACKAGES += \
    houdini64
endif
