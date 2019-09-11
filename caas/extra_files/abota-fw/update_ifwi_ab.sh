#!/system/bin/sh

# userdata checkpoint cleanup at postinstall step
USERDATA_CHECKPOINT_GC=vendor/bin/checkpoint_gc
if [ -f ${USERDATA_CHECKPOINT_GC} ]; then
    source ${USERDATA_CHECKPOINT_GC}
fi

SLOT_SUFFIX=$(getprop ro.boot.slot_suffix)

if [ "$SLOT_SUFFIX" = "_a" ]; then
    setprop vendor.ota.update.fw b
elif [ "$SLOT_SUFFIX" = "_b" ]; then
    setprop vendor.ota.update.fw a
fi

IOC_FLASH_TOOL=/vendor/bin/ioc_flash_server

if [ -f ${IOC_FLASH_TOOL} ]; then
    cp -rp /postinstall/firmware/ioc/* /mnt/ioc/
    setprop vendor.ioc.update run
    sleep 30
fi

exit 0
