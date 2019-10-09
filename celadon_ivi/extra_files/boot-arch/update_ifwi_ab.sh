#!/system/bin/sh

# userdata checkpoint cleanup at postinstall step
USERDATA_CHECKPOINT_GC=vendor/bin/checkpoint_gc
if [ -f ${USERDATA_CHECKPOINT_GC} ]; then
    source ${USERDATA_CHECKPOINT_GC}
fi

KFLD_UPDATE_FILE="/postinstall/firmware/kfld.efi"
KF_UPDATE_FILE="/postinstall/firmware/kernelflinger.efi"
BIOS_UPDATE_FILE="/postinstall/firmware/BIOSUPDATE.fv"
DEV_PATH_KFLD="/dev/block/by-name/esp"
DEV_PATH="/dev/block/by-name/bootloader"
MOUNT_POINT="/bootloader"
KFLD_DESTINATION_FILE="/bootloader/EFI/INTEL/KFLD_NEW.EFI"
KF_DESTINATION_FILE="/bootloader/EFI/BOOT/kernelflinger_new.efi"
BIOS_UPDATE_DESTINATION_FILE="/bootloader/BIOSUPDATE.fv"
is_exist_efi=0
is_exist_bios=0

EFI_UPDATE_FILE=${KF_UPDATE_FILE}
EFI_DESTINATION_FILE=${KF_DESTINATION_FILE}
# -f is check for regular files, not for symlinks
# so use -e which is more permissive to check if path exist
if [ -e ${DEV_PATH_KFLD} ]; then
    EFI_UPDATE_FILE=${KFLD_UPDATE_FILE}
    EFI_DESTINATION_FILE=${KFLD_DESTINATION_FILE}
    DEV_PATH=${DEV_PATH_KFLD}
fi

if [ -f ${EFI_UPDATE_FILE} ]; then
    is_exist_efi=1
fi
if [ -f ${BIOS_UPDATE_FILE} ]; then
   is_exist_bios=1
fi

if [[ $is_exist_efi -eq 0 && $is_exist_efi -eq 0 ]]; then
    echo "No file to be copied."
    exit -1
fi

mount -t vfat $DEV_PATH $MOUNT_POINT
if [ $? != 0 ]; then
    echo "Cannot mount $MOUNT_POINT"
    exit -2;
fi

if [ $is_exist_efi -eq 1 ]; then
   cp $EFI_UPDATE_FILE $EFI_DESTINATION_FILE
   if [ $? != 0 ]; then
       echo "Cannot copy $EFI_UPDATE_FILE"
       exit -3;
   fi
fi

if [ $is_exist_bios -eq 1 ]; then
   cp $BIOS_UPDATE_FILE $BIOS_UPDATE_DESTINATION_FILE
   if [ $? != 0 ]; then
       echo "Cannot copy $BIOS_UPDATE_FILE"
       exit -4;
   fi
fi

umount $MOUNT_POINT

echo "EFI copy OK"
exit 0
