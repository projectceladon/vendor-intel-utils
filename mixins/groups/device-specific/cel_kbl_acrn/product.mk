TARGET_BOARD_PLATFORM := broxton

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.rc:root/init.$(TARGET_DEVICE).rc \
    $(LOCAL_PATH)/init.recovery.rc:root/init.recovery.$(TARGET_DEVICE).rc \
    $(LOCAL_PATH)/ueventd.rc:root/ueventd.$(TARGET_DEVICE).rc \


# Camera: Device-specific configuration files.
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.xml:vendor/etc/permissions/android.hardware.camera.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:vendor/etc/permissions/android.hardware.camera.front.xml \


PRODUCT_PACKAGES += android.hardware.keymaster@3.0-impl \
                    android.hardware.keymaster@3.0-service \
                    android.hardware.wifi@1.0-service \
                    android.hardware.bluetooth@1.0-service.vbt \
                    android.hardware.usb@1.0-impl \
                    android.hardware.usb@1.0-service \
                    android.hardware.dumpstate@1.0-impl \
                    android.hardware.dumpstate@1.0-service \
                    camera.device@1.0-impl \
                    android.hardware.camera.provider@2.4-impl \
                    android.hardware.camera.provider@2.4-service \
                    android.hardware.graphics.mapper@2.0-impl \
                    android.hardware.graphics.allocator@2.0-impl \
                    android.hardware.graphics.allocator@2.0-service \
                    android.hardware.renderscript@1.0-impl \
                    android.hardware.graphics.composer@2.1-impl \
                    android.hardware.graphics.composer@2.1-service \
                    libbt-vendor \

PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/manifest.xml:vendor/manifest.xml

PRODUCT_PACKAGES += \
                   android.hardware.audio@2.0-impl \
                   android.hardware.audio@2.0-service \
                   android.hardware.audio.effect@2.0-impl \
                   android.hardware.soundtrigger@2.0-impl \
                   android.hardware.broadcastradio@1.0-impl
