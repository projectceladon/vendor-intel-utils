#!/bin/bash

[ $# -lt 1 ] && echo "Usage: $0 caas-flashfiles-eng-<user>.zip" && exit -1

if [ -f android.qcow2 ]
then
	echo -n "android.qcow2 already exsited, Do you want to flash new one(y/N):"
	read option
	if [ "$option" == 'y' ]
	then
		rm android.qcow2
	else
		exit 1
	fi
fi

qemu-img create -f qcow2 android.qcow2 16G

[ -d "./flashfiles_decompress" ] && rm -rf "./flashfiles_decompress"
mkdir ./flashfiles_decompress
unzip $1 -d ./flashfiles_decompress
dd if=/dev/zero of=./flash.vfat bs=63M count=160
mkfs.vfat ./flash.vfat
mcopy -i flash.vfat flashfiles_decompress/* ::

ovmf_file="./OVMF.fd"
[ ! -f $ovmf_file ] && ovmf_file="/usr/share/qemu/OVMF.fd"

if [[ $2 == "--display-off" ]]
then
	display_type="none"
else
	display_type="gtk,gl=on"
fi

qemu-system-x86_64 \
  -m 2048 -smp 2 -M q35 \
  -name caas-vm \
  -enable-kvm \
  -k en-us \
  -vga qxl \
  -machine kernel_irqchip=on \
  -chardev socket,id=charserial0,path=./kernel-console,server,nowait \
  -device isa-serial,chardev=charserial0,id=serial0 \
  -device qemu-xhci,id=xhci,addr=0x5 \
  -drive file=./flash.vfat,id=udisk1,format=raw,if=none \
  -device usb-storage,drive=udisk1,bus=xhci.0 \
  -device virtio-scsi-pci,id=scsi0,addr=0x8 \
  -drive file=./android.qcow2,if=none,format=qcow2,id=scsidisk1 \
  -device scsi-hd,drive=scsidisk1,bus=scsi0.0 \
  -drive file=$ovmf_file,format=raw,if=pflash \
  -no-reboot \
  -display $display_type \
  -boot menu=on,splash-time=5000,strict=on \

echo "Flashing is completed"
