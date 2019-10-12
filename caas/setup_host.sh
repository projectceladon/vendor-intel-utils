#!/bin/bash

reboot_required=0

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
	apt install -y git libfdt-dev libpixman-1-dev libssl-dev vim socat libsdl1.2-dev libspice-server-dev autoconf libtool xtightvncviewer tightvncserver x11vnc libsdl1.2-dev uuid-runtime uuid uml-utilities bridge-utils python-dev liblzma-dev libc6-dev libegl1-mesa-dev libepoxy-dev libdrm-dev libgbm-dev libaio-dev libusb-1.0.0-dev libgtk-3-dev bison

	wget https://download.qemu.org/qemu-3.0.0.tar.xz
	tar -xf qemu-3.0.0.tar.xz
	cd qemu-3.0.0/
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
	    --audio-drv-list=alsa
	make -j24
	make install
	cd ../
}

function ubu_enable_host_gvtg(){
	[[ ! `cat /etc/default/grub` =~ "i915.enable_gvt=1 intel_iommu=on" ]] \
	&& sed -i "s/GRUB_CMDLINE_LINUX=\"/GRUB_CMDLINE_LINUX=\"i915.enable_gvt=1 intel_iommu=on /g" /etc/default/grub \
	&& update-grub \
	&& reboot_required=1
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
		echo "W: Detected linux version $version, software rendering will be used"
		echo "W: Please upgrade kernel version newer than 5.0.0 for smoother experience!"
	fi
}

function clr_changes_require(){
	echo "Please make sure your bundle is working"
	echo "If you run the installation first time, reboot is required"
	swupd bundle-add wget storage-utils kvm-host
}

function clr_enable_host_gvtg(){
	[[ ! -f /etc/kernel/cmdline.d/gvtg.conf ]] \
	&& mkdir -p /etc/kernel/cmdline.d/ \
	&& echo "i915.enable_gvt=1 kvm.ignore_msrs=1 intel_iommu=on" > /etc/kernel/cmdline.d/gvtg.conf \
	&& clr-boot-manager update \
	&& reboot_required=1
}

function get_required_scripts(){
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/start_flash_usb.sh
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/start_android_qcow2.sh
	chmod +x start_flash_usb.sh
	chmod +x start_android_qcow2.sh
}

version=`cat /proc/version`

if [[ $version =~ "Ubuntu" ]]; then
	check_network
	ubu_changes_require
	ubu_install_qemu
	ubu_enable_host_gvtg
	get_required_scripts
	check_kernel
	ask_reboot
elif [[ $version =~ "Clear Linux OS" ]]; then
	check_network
	clr_changes_require
	clr_enable_host_gvtg
	get_required_scripts
	check_kernel
	ask_reboot
else
	echo "only clearlinux or Ubuntu is supported"
fi
