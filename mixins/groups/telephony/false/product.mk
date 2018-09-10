# product.mk common to Telephony disabled platforms
PRODUCT_COPY_FILES += \
    $(INTEL_PATH_VENDOR)/featsetclass_tel/telephony/all/apns-conf.xml:system/etc/old-apns-conf.xml

# Inherit from common Open Source Telephony product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base.mk)

DEVICE_PACKAGE_OVERLAYS += $(INTEL_PATH_VENDOR)/featsetclass_tel/telephony/overlay_none
PRODUCT_PROPERTY_OVERRIDES += \
    ro.radio.noril=true
