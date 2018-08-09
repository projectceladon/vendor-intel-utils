PRODUCT_PACKAGES += \
    wificond \
    wifilogd \
    wpa_supplicant \
    wpa_cli \
    iw

PRODUCT_PACKAGES += \
  android.hardware.wifi@1.0-service

# FW and PNVM
PRODUCT_PACKAGES += \
    {{firmware}} \
{{#nvm}}
    iwl-nvm
{{/nvm}}

# iwlwifi USC
PRODUCT_PACKAGES += \
    wifi_intel_usc

#copy iwlwifi wpa config files
PRODUCT_COPY_FILES += \
        $(INTEL_PATH_COMMON)/wlan/wpa_supplicant-common.conf:vendor/etc/wifi/wpa_supplicant.conf \
        $(INTEL_PATH_COMMON)/wlan/iwlwifi/wpa_supplicant_overlay.conf:vendor/etc/wifi/wpa_supplicant_overlay.conf \
        frameworks/native/data/etc/android.hardware.wifi.xml:vendor/etc/permissions/android.hardware.wifi.xml \
        frameworks/native/data/etc/android.hardware.wifi.direct.xml:vendor/etc/permissions/android.hardware.wifi.direct.xml

{{#gpp}}
# Add Manufacturing tool
PRODUCT_PACKAGES += \
    wlan_intel_restore.sh
{{/gpp}}

{{#softap_dualband_allow}}
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.wifi.softap_dualband_allow=true
{{/softap_dualband_allow}}
{{^softap_dualband_allow}}
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.wifi.softap_dualband_allow=false
{{/softap_dualband_allow}}
