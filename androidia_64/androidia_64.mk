ENABLE_NATIVEBRIDGE_64BIT := true

$(call inherit-product,$(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product,device/android_ia/common/device.mk)

# Overrides
PRODUCT_NAME := androidia_64
PRODUCT_BRAND := AndroidIA
PRODUCT_DEVICE := androidia_64
PRODUCT_MODEL := Generic androidia_64
