# Tinyalsa
PRODUCT_PACKAGES_DEBUG += \
    tinymix \
    tinyplay \
    tinycap \
    tinypcminfo \
    tinyprobe

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/libfwdt/reef-apl.ri:vendor/firmware/intel/reef-apl.ri \
    $(LOCAL_PATH)/audio/libfwdt/reef-apl.tplg:vendor/firmware/intel/reef-apl.tplg
