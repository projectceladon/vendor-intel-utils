#!/vendor/bin/sh

SLOT_SUFFIX=$(getprop ro.boot.slot_suffix)

IFWIFAIL=0
CMDIFWIFAIL=$(cat /proc/cmdline | grep "^.*ABL.ifwifail=" | sed 's/^.*ABL.ifwifail=//' | cut -d " " -f1 | cut -d "," -f2)
CMDIFWIVER1=$(cat /proc/cmdline | grep "^.*ABL.bpdt1=" | sed 's/^.*ABL.bpdt1=//' | cut -d " " -f1 | cut -d "," -f2)
CMDIFWIVER2=$(cat /proc/cmdline | grep "^.*ABL.bpdt2=" | sed 's/^.*ABL.bpdt2=//' | cut -d " " -f1 | cut -d "," -f2)
FILEIFWIVER=/mnt/ifwi.version

if [ "$CMDIFWIFAIL" == "07" ]; then
    echo "IFWI ERR: IFWI_UPDATE_ERR_CSE"
    IFWIFAIL=1
elif [ "$CMDIFWIFAIL" == "08" ]; then
    echo "IFWI ERR: IFWI_UPDATE_ERR_BIOS_REG_READ"
    IFWIFAIL=1
elif [ "$CMDIFWIFAIL" == "09" ]; then
    echo "IFWI ERR: IFWI_UPDATE_ERR_SPI_DRV_INIT"
    IFWIFAIL=1
elif [ "$CMDIFWIFAIL" == "0A" ]; then
    echo "IFWI ERR: IFWI_UPDATE_ERR_SPI_WRITE_ERROR"
    IFWIFAIL=1
elif [ "$CMDIFWIFAIL" == "0B" ]; then
    echo "IFWI ERR: IFWI_UPDATE_ERR_SPI_READ_ERROR"
    IFWIFAIL=1
elif [ "$CMDIFWIFAIL" == "0C" ]; then
    echo "IFWI ERR: IFWI_UPDATE_ERR_SPI_ERASE_ERROR"
    IFWIFAIL=1
elif [ "$CMDIFWIFAIL" == "0D" ]; then
    echo "IFWI ERR: IFWI_UPDATE_ERR_SPI_COMPARE_ERROR"
    IFWIFAIL=1
fi

if [ "$IFWIFAIL" != "0" ]; then
    if [ "$SLOT_SUFFIX" = "_a" ]; then
        setprop vendor.ota.update.fw a
    elif [ "$SLOT_SUFFIX" = "_b" ]; then
        setprop vendor.ota.update.fw b
    fi
fi

if [ "$SLOT_SUFFIX" = "_a" ]; then
    /vendor/bin/toybox_vendor dd if=/dev/block/by-name/bootloader_a of=$FILEIFWIVER bs=1024 skip=7990 count=1
    RAWIFWIVER_A=$(cat $FILEIFWIVER | head -n 10)
    rm $FILEIFWIVER
elif [ "$SLOT_SUFFIX" = "_b" ]; then
    /vendor/bin/toybox_vendor dd if=/dev/block/by-name/bootloader_b of=$FILEIFWIVER bs=1024 skip=7990 count=1
    RAWIFWIVER_B=$(cat $FILEIFWIVER | head -n 10)
    rm $FILEIFWIVER
fi
if [ "$CMDIFWIVER1" != "" ] && [ "$CMDIFWIVER2" != "" ]; then
    if [ "$CMDIFWIVER1" = "$CMDIFWIVER2" ]; then
        if [ "$SLOT_SUFFIX" = "_a" ]; then
            if [ "$RAWIFWIVER_A" != "$CMDIFWIVER1" ] && [ "$RAWIFWIVER_A" != "" ]; then
                setprop vendor.ota.update.fw a
            fi
        elif [ "$SLOT_SUFFIX" = "_b" ]; then
            if [ "$RAWIFWIVER_B" != "$CMDIFWIVER1" ] && [ "$RAWIFWIVER_B" != "" ]; then
                setprop vendor.ota.update.fw b
            fi
        fi
    else
        if [ "$SLOT_SUFFIX" = "_a" ]; then
            setprop vendor.ota.update.fw a
        elif [ "$SLOT_SUFFIX" = "_b" ]; then
            setprop vendor.ota.update.fw b
        fi
    fi
fi
exit 0
