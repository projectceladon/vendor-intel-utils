#!/bin/sh

FLASHING_LIST="multiboot bootloader boot system vendor recovery"
ERASE_LIST="data"
FORMAT_LIST="config cache"
MMC="/dev/mmcblk0"
SIMG2IMG="/opt/simg2img/simg2img"
PART_LIST=`mktemp`

CMDLINE=`cat /proc/cmdline`
for i in $CMDLINE
do
   echo $i | grep "ABL.hwver"
   if [ "$?" == 0 ];then
      hwver=`echo $i | cut -d ',' -f 4 | tr tr '[A-Z]''[a-z]'`
      #echo $hwver
      if [ "$hwver" == "908f" ];then
         echo "a0 soc..."
         cp bootloader_gr_mrb bootloader.img
      elif [ "$hwver" == "b08f" ];then
         echo "b1 soc..."
         cp bootloader_gr_mrb_b1 bootloader.img
      fi
   fi
done


echo "Flashing gpt.img"
dd if=gpt.img of=${MMC}

echo "Reloading partition table"
hdparm -z ${MMC}

fdisk -l > $PART_LIST
cat $PART_LIST

for p in $FLASHING_LIST
do
   part_num=`grep -w $p $PART_LIST | awk '{print $1}'`
   part_name=`echo $p | cut -d '_' -f 2-`

   if [ ! -b ${MMC}p${part_num} ]
   then
      echo "Could not flash $p : partition not found."
      continue
   fi

   if [ ! -f ${part_name}.img ]
   then
      echo "Could not flash $p : file not found."
      continue
   fi

   echo "Flashing ${part_name} on ${MMC}p${part_num}"
   unsparsed_img=/tmp/${part_name}.img

   cmd="$SIMG2IMG ${part_name}.img ${unsparsed_img}"
   echo $cmd
   $cmd
   if [ "$?" != "0" ]
   then
      echo "Source image is not sparsed. Flashing it"
      cmd="dd if=${part_name}.img of=${MMC}p${part_num} bs=1M"
      echo $cmd
      $cmd
   else
      cmd="dd if=${unsparsed_img} of=${MMC}p${part_num} bs=1M"
      echo $cmd
      $cmd
   fi
   rm ${unsparsed_img}
done

for p in $ERASE_LIST
do
   part_num=`grep -w $p $PART_LIST | awk '{print $1}'`
   part_name=`echo $p | cut -d '_' -f 2`

   echo "Erasing ${part_name} (${MMC}p${part_num})"
   dd if=/dev/zero of=${MMC}p${part_num} bs=4096 count=2
done

for p in $FORMAT_LIST
do
   part_num=`grep -w $p $PART_LIST | awk '{print $1}'`
   part_name=`echo $p | cut -d '_' -f 2`

   echo "Formatting ${part_name} (${MMC}p${part_num})"
   yes | mkfs.ext4 ${MMC}p${part_num}
done

rm $PART_LIST
sync
