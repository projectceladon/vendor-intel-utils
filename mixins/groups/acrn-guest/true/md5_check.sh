#!/bin/bash

md5_file=md5sum.txt
check_sum_imgs=(sos_boot.img sos_rootfs.img.xz partition_desc.bin)

if [ -a "$1/$md5_file" ];then
	cat $1/$md5_file | while read line
	do
		var_name=`echo $line | awk '{print $2}'`
		var_name=${var_name##*/}

		for img in ${check_sum_imgs[@]}; do
			if [ "$img" = "$var_name" ]; then
				md5_val=`echo $line | awk '{print $1}'`
				md5_img=`md5sum $1/$img | awk '{print $1}'`

				echo "$line in $md5_file"
				echo "md5sum $img $md5_img"
				count=1
				while [ $count -le 3 ];
				do
					if [ "$md5_img" != "$md5_val" ]; then
						echo "file $img checksum is failed, try again"
						sleep 60
						count=$((count + 1))
						md5_img=`md5sum $1/$img | awk '{print $1}'`
						echo "md5sum $img $md5_img"
					else
						break
					fi
				done

				if [ $count -gt 2 ];then
					echo "network exception, download $img failed!"
					exit 1
				else
					echo "$img: md5 check successfully!"
				fi
			fi
		done
	done
else
	echo "ERROR: $md5_file File NOT FOUND!"
fi
