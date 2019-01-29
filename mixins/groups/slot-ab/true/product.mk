PRODUCT_PACKAGES += \
    update_engine \
    update_engine_client \
    update_verifier \
    libavb \
    bootctrl.avb \
    bootctrl.intel \
    bootctrl.intel.static \
    update_engine_sideload \
    avbctl \
    android.hardware.boot@1.0-impl \
    android.hardware.boot@1.0-service

PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.bootctrl=intel

ifeq ({{ota-upgrade}},False)
ifeq ({{post_fw_update}},true)

PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/update_ifwi_ab.sh:vendor/bin/update_ifwi_ab
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/update_ifwi_ab.sh:recovery/root/vendor/bin/update_ifwi_ab

ifeq ({{boot-arch}},abl)
PRODUCT_PACKAGES += \
    abl-user-cmd_vendor \
    abl-user-cmd_static
endif

ifeq ({{boot-arch}},sbl)
PRODUCT_PACKAGES += \
    sbl-user-cmd_vendor \
    sbl-user-cmd_static
endif

endif


ifeq ({{boot_fw_update}},true)

#only support abl
ifeq ({{boot-arch}},abl)
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/update_ifwi_boot.sh:vendor/bin/fw_update.sh
endif

ifeq ({{post_fw_update}},False)
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/update_ifwi_ab.sh:vendor/bin/update_ifwi_ab
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/update_ifwi_ab.sh:recovery/root/vendor/bin/update_ifwi_ab

ifeq ({{boot-arch}},abl)
PRODUCT_PACKAGES += \
    abl-user-cmd_vendor \
    abl-user-cmd_static
endif
endif

endif
#ota-upgrade=true
else
ifeq ({{boot-arch}},abl)

ifeq ({{upgrade_version}},o)
PRODUCT_PACKAGES += \
    abl-user-cmd_vendor \
    abl-user-cmd_static

PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/upgrade_o/updater_ab.sh:vendor/bin/updater_ab
PRODUCT_COPY_FILES += $(LOCAL_PATH)/{{_extra_dir}}/upgrade_o/fw_update.sh:vendor/bin/fw_update.sh
endif

endif
endif
