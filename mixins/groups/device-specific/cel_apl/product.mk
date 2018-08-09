TARGET_BOARD_PLATFORM := project-celadon

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.rc:root/init.$(TARGET_DEVICE).rc \
    $(LOCAL_PATH)/init.recovery.rc:root/init.recovery.$(TARGET_DEVICE).rc \
    $(LOCAL_PATH)/ueventd.rc:root/ueventd.$(TARGET_DEVICE).rc \
    $(LOCAL_PATH)/fstab:root/fstab

PRODUCT_PACKAGES += android.hardware.keymaster@3.0-impl \
                    android.hardware.keymaster@3.0-service \
                    android.hardware.wifi@1.0-service \
                    android.hardware.audio@2.0-impl \
	            android.hardware.audio@2.0-service \
                    android.hardware.soundtrigger@2.0-impl \
                    android.hardware.audio.effect@2.0-impl \
                    android.hardware.broadcastradio@1.0-impl \
                    android.hardware.graphics.mapper@2.0-impl \
                    android.hardware.graphics.allocator@2.0-impl \
                    android.hardware.graphics.allocator@2.0-service

PRODUCT_PACKAGES += \
	   android.hardware.graphics.composer@2.1-impl \
	   android.hardware.graphics.composer@2.1-service \
	   android.hardware.usb@1.0-impl \
	   android.hardware.usb@1.0-service \
	   android.hardware.dumpstate@1.0-impl \
	   android.hardware.dumpstate@1.0-service

PRODUCT_COPY_FILES += $(LOCAL_PATH)/manifest.xml:vendor/manifest.xml

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += service.adb.root=1
