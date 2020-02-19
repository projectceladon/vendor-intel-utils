#!/bin/bash

reboot_required=0
QEMU_REL=qemu-4.2.0
a=`grep -rn CIV_WORK_DIR /etc/environment`

function ubu_changes_require(){
	echo "Please make sure your apt is working"
	echo "If you run the installation first time, reboot is required"
	read -p "QEMU version will be replaced (it could be recovered by 'apt purge qemu*, apt install qemu'), do you want to continue? [Y/n]" res
	if [ x$res = xn ]; then
		exit 0
	fi
	apt install -y wget mtools ovmf
}

function ubu_install_qemu(){
	apt purge -y "qemu*"
	apt autoremove -y
	apt install -y git libfdt-dev libpixman-1-dev libssl-dev vim socat libsdl2-dev libspice-server-dev autoconf libtool xtightvncviewer tightvncserver x11vnc uuid-runtime uuid uml-utilities bridge-utils python-dev liblzma-dev libc6-dev libegl1-mesa-dev libepoxy-dev libdrm-dev libgbm-dev libaio-dev libusb-1.0.0-dev libgtk-3-dev bison

	wget https://download.qemu.org/$QEMU_REL.tar.xz
	tar -xf $QEMU_REL.tar.xz
	cd $QEMU_REL/
	./configure --prefix=/usr \
	    --enable-kvm \
	    --disable-xen \
	    --enable-libusb \
	    --enable-debug-info \
	    --enable-debug \
	    --enable-sdl \
	    --enable-vhost-net \
	    --enable-spice \
	    --disable-debug-tcg \
	    --enable-opengl \
	    --enable-gtk \
	    --target-list=x86_64-softmmu \
	    --audio-drv-list=pa
	make -j24
	make install
	cd ../
}

function ubu_build_ovmf(){
	sudo apt install -y uuid-dev nasm acpidump iasl
	cd $QEMU_REL/roms/edk2
	source ./edksetup.sh
	make -C BaseTools/
	build -b DEBUG -t GCC5 -a X64 -p OvmfPkg/OvmfPkgX64.dsc -D NETWORK_IP4_ENABLE -D NETWORK_ENABLE  -D SECURE_BOOT_ENABLE
	cp Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd ../../../OVMF.fd
	cd ../../../
}

function ubu_enable_host_gvtg(){
	if [[ ! `cat /etc/default/grub` =~ "i915.enable_gvt=1 intel_iommu=on" ]]; then
		read -p "The grub entry in '/etc/default/grub' will be updated for enabling GVT-g, do you want to continue? [Y/n]" res
		if [ x$res = xn ]; then
			exit 0
		fi
		sed -i "s/GRUB_CMDLINE_LINUX=\"/GRUB_CMDLINE_LINUX=\"i915.enable_gvt=1 intel_iommu=on /g" /etc/default/grub
		update-grub
		reboot_required=1
	fi
}

function check_network(){
	wget --timeout=3 --tries=1 https://github.com/projectceladon/ -q -O /dev/null
	if [ $? -ne 0 ]; then
		echo "access https://github.com/projectceladon/ failed!"
		echo "please make sure network is working"
		exit -1
	fi
}

function ask_reboot(){
	if [ $reboot_required -eq 1 ];then
		read -p "Reboot is required, do you want to reboot it NOW? [y/N]" res
		if [ x$res = xy ]; then
			reboot
		else
			echo "Please reboot system later to take effect"
		fi
	fi
}

function check_kernel(){
	version=$(cat /proc/version | \
		awk '{
			for(i=0;i<NF;i++) { if ($i == "Linux" && $(i+1) == "version") { print $(i+2); next; } }
		}'
	)
	if [[ "$version" > "5.0.0" ]]; then 
		echo "hardware rendering is supported in current kernel"
	else
		echo "W: Detected linux version $version"
		echo "W: Please upgrade kernel version newer than 5.0.0!"
	fi
}

function clr_changes_require(){
	echo "Please make sure your bundle is working"
	echo "If you run the installation first time, reboot is required"
	swupd bundle-add wget storage-utils kvm-host
}

function clr_enable_host_gvtg(){
	if [[ ! -f /etc/kernel/cmdline.d/gvtg.conf ]]; then
		read -p "File '/etc/kernel/cmdline.d/gvtg.conf' will be created for enabling GVT-g, do you want to continue? [Y/n]" res
		if [ x$res = xn ]; then
			exit 0
		fi
		mkdir -p /etc/kernel/cmdline.d/
		echo "i915.enable_gvt=1 kvm.ignore_msrs=1 intel_iommu=on" > /etc/kernel/cmdline.d/gvtg.conf
		clr-boot-manager update
		reboot_required=1
	fi
}

function get_required_scripts(){
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/start_flash_usb.sh
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/start_android_qcow2.sh
	chmod +x start_flash_usb.sh
	chmod +x start_android_qcow2.sh
}

function save_env(){
	if [ -z "$a" ]; then
		echo "export CIV_WORK_DIR=$(pwd)" | tee -a /etc/environment
	else
		sed -i "s|export CIV_WORK_DIR.*||g" /etc/environment
		echo "export CIV_WORK_DIR=$(pwd)" | tee -a /etc/environment
	fi
}

version=`cat /proc/version`

if [[ $version =~ "Ubuntu" ]]; then
	check_network
	ubu_changes_require
	ubu_install_qemu
	ubu_build_ovmf
	ubu_enable_host_gvtg
	get_required_scripts
	check_kernel
	save_env
	ask_reboot
elif [[ $version =~ "Clear Linux OS" ]]; then
	check_network
	clr_changes_require
	clr_enable_host_gvtg
	get_required_scripts
	check_kernel
	save_env
	ask_reboot
else
	echo "only clearlinux or Ubuntu is supported"
fi
