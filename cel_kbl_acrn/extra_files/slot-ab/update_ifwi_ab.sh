#!/vendor/bin/sh

SLOT_SUFFIX=$(getprop ro.boot.slot_suffix)

if [ "$SLOT_SUFFIX" = "_a" ]; then
    setprop vendor.ota.update.fw b
elif [ "$SLOT_SUFFIX" = "_b" ]; then
    setprop vendor.ota.update.fw a
fi

exit 0
