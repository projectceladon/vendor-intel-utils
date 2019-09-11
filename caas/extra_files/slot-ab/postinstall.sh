#!/system/bin/sh

UPDATE_IFWI_AB=/postinstall/bin/update_ifwi_ab
if [ -f ${UPDATE_IFWI_AB} ]; then
    source ${UPDATE_IFWI_AB}
fi
exit 0
