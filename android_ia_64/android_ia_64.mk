ENABLE_NATIVEBRIDGE_64BIT := true

$(call inherit-product,$(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product,device/android_ia/common/device.mk)

# Overrides
PRODUCT_NAME := android_ia_64
PRODUCT_BRAND := AndroidIA
PRODUCT_DEVICE := android_ia_64
PRODUCT_MODEL := Generic android_ia_64
