#!/bin/bash

check_sum_imgs=(flash_AaaG.json sos_boot.img sos_rootfs.img partition_desc.bin)

if [ -a "$1/md5sum.md5" ];then
	cat $1/md5sum.md5 | while read line
	do
		var_name=`echo $line | awk '{print $2}'`

		for img in ${check_sum_imgs[@]}; do
			if [ "$img" = "$var_name" ]; then
				md5_val=`echo $line | awk '{print $1}'`
				md5_img=`md5sum $1/$img | awk '{print $1}'`

				count=1
				while [ $count -le 30 ];
				do
					if [ "$md5_img" != "$md5_val" ]; then
						echo "file $img download is continue ......"
						sleep 60
						count=$((count + 1))
						md5_img=`md5sum $1/$img | awk '{print $1}'`
					else
						break
					fi
				done

				if [ $count -gt 20 ];then
					echo "network exception, download $img failed!"
					exit 1
				fi
			fi
		done
	done
	echo "ACRN SOS Images: md5sum check PASS!"
else
	echo "ERROR: md5sum.md5 file NOT FOUND!"
	echo "$1: $(ls -l $1)"
fi
