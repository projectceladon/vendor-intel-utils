#!/vendor/bin/sh

SLOT_SUFFIX=$(getprop ro.boot.slot_suffix)

IFWIFAIL=0
CMDIFWIFAIL=$(cat /proc/cmdline | grep "^.*ABL.ifwifail=" | sed 's/^.*ABL.ifwifail=//' | cut -d " " -f1 | cut -d "," -f2)
CMDIFWIVER1=$(cat /proc/cmdline | grep "^.*ABL.bpdt1=" | sed 's/^.*ABL.bpdt1=//' | cut -d " " -f1 | cut -d "," -f2)
CMDIFWIVER2=$(cat /proc/cmdline | grep "^.*ABL.bpdt2=" | sed 's/^.*ABL.bpdt2=//' | cut -d " " -f1 | cut -d "," -f2)

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

RAWIFWIVER=$(getprop ro.vendor.ifwi_version)
if [ "$CMDIFWIVER1" != "" ] && [ "$CMDIFWIVER2" != "" ]; then
    if [ "$CMDIFWIVER1" = "$CMDIFWIVER2" ]; then
        if [ "$SLOT_SUFFIX" = "_a" ]; then
            if [ "$RAWIFWIVER" != "$CMDIFWIVER1" ] && [ "$RAWIFWIVER" != "" ]; then
                setprop vendor.ota.update.fw a
            fi
        elif [ "$SLOT_SUFFIX" = "_b" ]; then
            if [ "$RAWIFWIVER" != "$CMDIFWIVER1" ] && [ "$RAWIFWIVER" != "" ]; then
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
