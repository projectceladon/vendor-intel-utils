#enable Widevine drm
PRODUCT_PROPERTY_OVERRIDES += drm.service.enabled=true

#only enable default drm service
PRODUCT_PACKAGES += android.hardware.drm@1.0-service \
                    android.hardware.drm@1.0-impl \
                    android.hardware.drm@1.1-service.clearkey

