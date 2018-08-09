#enable Widevine drm
PRODUCT_PROPERTY_OVERRIDES += drm.service.enabled=true


# There is an additional dependency on hdcpd that should be controlled
# through the content-protection mixin

PRODUCT_PACKAGES += com.google.widevine.software.drm.xml \
    com.google.widevine.software.drm \
    libdrmwvmplugin \
    libwvm \
    libdrmdecrypt \
    libwvdrmengine \
    libWVStreamControlAPI_L3 \
    libwvdrm_L3

PRODUCT_PACKAGES += android.hardware.drm@1.1-service.widevine \
                    android.hardware.drm@1.0-service \
                    android.hardware.drm@1.0-impl \
                    libwvhidl \
                    android.hardware.drm@1.1-service.clearkey

PRODUCT_PACKAGES_ENG += ExoPlayerDemo

BOARD_WIDEVINE_OEMCRYPTO_LEVEL := 3
