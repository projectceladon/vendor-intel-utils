#! /system/bin/sh
insmod /vendor/lib/modules/9pnet.ko
insmod /vendor/lib/modules/9pnet_virtio.ko
insmod /vendor/lib/modules/9p.ko
cd /mnt
mkdir /mnt/share
mount -t 9p -o trans=virtio,version=9p2000.L hostshare /mnt/share
