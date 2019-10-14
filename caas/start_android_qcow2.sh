#!/bin/bash

g_file="/sys/bus/pci/devices/0000:00:02.0/4ec1ff92-81d7-11e9-aed4-5bf6a9a2bb0a"
ovmf_file="./OVMF.fd"
[ ! -f $ovmf_file ] && ovmf_file="/usr/share/qemu/OVMF.fd"

function setup_vgpu(){
	res=0
	if [ ! -d $g_file ]; then
		echo "Creating VGPU..."
		sudo sh -c "echo 4ec1ff92-81d7-11e9-aed4-5bf6a9a2bb0a > /sys/bus/pci/devices/0000:00:02.0/mdev_supported_types/i915-GVTg_V5_8/create"
		res=$?
	fi
	return $res
}

function launch_hwrender(){
	qemu-system-x86_64 \
	  -m 2048 -smp 2 -M q35 \
	  -name caas-vm \
	  -enable-kvm \
	  -vga none \
	  -display gtk,gl=on \
	  -device vfio-pci,sysfsdev=/sys/bus/pci/devices/0000:00:02.0/4ec1ff92-81d7-11e9-aed4-5bf6a9a2bb0a,display=on,x-igd-opregion=on \
	  -k en-us \
	  -machine kernel_irqchip=off \
	  -global PIIX4_PM.disable_s3=1 -global PIIX4_PM.disable_s4=1 \
	  -cpu host \
	  -device qemu-xhci,id=xhci,addr=0x8 \
	  -device usb-mouse \
	  -device usb-kbd \
	  -drive file=$ovmf_file,format=raw,if=pflash \
	  -chardev socket,id=charserial0,path=./kernel-console,server,nowait \
	  -device isa-serial,chardev=charserial0,id=serial0 \
	  -device intel-hda -device hda-duplex \
	  -drive file=./android.qcow2,if=none,id=disk1 \
	  -device virtio-blk-pci,drive=disk1,bootindex=1 \
	  -device e1000,netdev=net0 \
	  -netdev user,id=net0,hostfwd=tcp::5555-:5555 \
	  -nodefaults
}

function launch_swrender(){
	qemu-system-x86_64 \
	  -m 2048 -smp 2 -M q35 \
	  -name caas-vm \
	  -enable-kvm \
	  -k en-us \
	  -vga none \
	  -display gtk,gl=on \
	  -device qxl-vga,xres=1280,yres=720 \
	  -machine kernel_irqchip=off \
	  -global PIIX4_PM.disable_s3=1 -global PIIX4_PM.disable_s4=1 \
	  -cpu host \
	  -device qemu-xhci,id=xhci,addr=0x8 \
	  -device usb-mouse \
	  -device usb-kbd \
	  -drive file=$ovmf_file,format=raw,if=pflash \
	  -chardev socket,id=charserial0,path=./kernel-console,server,nowait \
	  -device isa-serial,chardev=charserial0,id=serial0 \
	  -device intel-hda -device hda-duplex \
	  -drive file=./android.qcow2,if=none,id=disk1 \
	  -device virtio-blk-pci,drive=disk1,bootindex=1 \
	  -device e1000,netdev=net0 \
	  -netdev user,id=net0,hostfwd=tcp::5555-:5555 \
	  -nodefaults
}

version=`cat /proc/version`

vno=$(echo $version | \
	awk '{
		for(i=0;i<NF;i++) { if ($i == "Linux" && $(i+1) == "version") { print $(i+2); next; } }
	}'
)
if [[ "$vno" > "5.0.0" ]]; then 
	setup_vgpu
	if [[ $? == 0 ]]; then
		launch_hwrender
	else
		echo "W: Failed to create vgpu, fall to software rendering"
		launch_swrender
	fi
else
	echo "E: Detected linux version $vno"
	echo "E: Please upgrade kernel version newer than 5.0.0!"
	exit -1
fi

