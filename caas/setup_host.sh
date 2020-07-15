#!/bin/bash

set -eE

function error() {
	local line=$1
	local cmd=$2
	echo "$BASH_SOURCE Failed at line($line): $cmd"
}
trap 'error ${LINENO} "$BASH_COMMAND"' ERR

reboot_required=0
QEMU_REL=qemu-4.2.0
CIV_WORK_DIR=$(pwd)
CIV_GOP_DIR=$CIV_WORK_DIR/GOP_PKG

function ubu_changes_require(){
	echo "Please make sure your apt is working"
	echo "If you run the installation first time, reboot is required"
	read -p "QEMU version will be replaced (it could be recovered by 'apt purge ^qemu, apt install qemu'), do you want to continue? [Y/n]" res
	if [ x$res = xn ]; then
		exit 0
	fi
	apt install -y wget mtools ovmf dmidecode python3-usb python3-pyudev pulseaudio jq
}

function ubu_install_qemu_gvt(){
	apt purge -y "^qemu"
	apt autoremove -y
	apt install -y git libfdt-dev libpixman-1-dev libssl-dev vim socat libsdl2-dev libspice-server-dev autoconf libtool xtightvncviewer tightvncserver x11vnc uuid-runtime uuid uml-utilities bridge-utils python-dev liblzma-dev libc6-dev libegl1-mesa-dev libepoxy-dev libdrm-dev libgbm-dev libaio-dev libusb-1.0-0-dev libgtk-3-dev bison libcap-dev libattr1-dev flex libvirglrenderer-dev build-essential gettext libegl-mesa0 libegl-dev libglvnd-dev libgl1-mesa-dev libgl1-mesa-dev libgles2-mesa-dev libegl1 gcc g++ pkg-config libpulse-dev libgl1-mesa-dri

	[ ! -f $CIV_WORK_DIR/$QEMU_REL.tar.xz ] && wget https://download.qemu.org/$QEMU_REL.tar.xz -P $CIV_WORK_DIR
	[ -d $CIV_WORK_DIR/$QEMU_REL ] && rm -rf $CIV_WORK_DIR/$QEMU_REL
	tar -xf $CIV_WORK_DIR/$QEMU_REL.tar.xz

	cd $CIV_WORK_DIR/$QEMU_REL/
	patch -p1 < $CIV_WORK_DIR/patches/qemu/Disable-EDID-auto-generation-in-QEMU.patch
	patch -p1 < $CIV_WORK_DIR/patches/qemu/0001-Revert-Revert-vfio-pci-quirks.c-Disable-stolen-memor.patch
	if [ -d $CIV_GOP_DIR ]; then
		for i in $CIV_GOP_DIR/qemu/*.patch; do patch -p1 < $i; done
	fi

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
		--enable-virtfs \
		--target-list=x86_64-softmmu \
		--audio-drv-list=pa
	make -j24
	make install
	cd -
}

function install_9p_module(){
	echo "installing 9p kernel modules for file-sharing"
	sudo modprobe 9pnet
	sudo modprobe 9pnet_virtio
	sudo modprobe 9p
	mkdir -p $CIV_WORK_DIR/share_folder
}

function ubu_build_ovmf_gvt(){
	sudo apt install -y uuid-dev nasm acpidump iasl
	cd $CIV_WORK_DIR/$QEMU_REL/roms/edk2

	patch -p4 < $CIV_WORK_DIR/patches/ovmf/OvmfPkg-add-IgdAssgingmentDxe-for-qemu-4_2_0.patch
	if [ -d $CIV_GOP_DIR ]; then
		for i in $CIV_GOP_DIR/ovmf/*.patch; do patch -p1 < $i; done
		cp $CIV_GOP_DIR/ovmf/Vbt.bin OvmfPkg/Vbt/Vbt.bin
	fi

	source ./edksetup.sh
	make -C BaseTools/
	build -b DEBUG -t GCC5 -a X64 -p OvmfPkg/OvmfPkgX64.dsc -D NETWORK_IP4_ENABLE -D NETWORK_ENABLE  -D SECURE_BOOT_ENABLE -DTPM2_ENABLE=TRUE
	cp Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd ../../../OVMF.fd

	if [ -d $CIV_GOP_DIR ]; then
		local gpu_device_id=$(cat /sys/bus/pci/devices/0000:00:02.0/device)
		./BaseTools/Source/C/bin/EfiRom -f 0x8086 -i $gpu_device_id -e $CIV_GOP_DIR/IntelGopDriver.efi -o $CIV_GOP_DIR/GOP.rom
	fi

	cd -
}

function ubu_enable_host_gvt(){
	if [[ ! `cat /etc/default/grub` =~ "i915.enable_gvt=1 intel_iommu=on i915.force_probe=*" ]]; then
		read -p "The grub entry in '/etc/default/grub' will be updated for enabling GVT-g and GVT-d, do you want to continue? [Y/n]" res
		if [ x$res = xn ]; then
			exit 0
		fi
		sed -i "s/GRUB_CMDLINE_LINUX=\"/GRUB_CMDLINE_LINUX=\"i915.enable_gvt=1 intel_iommu=on i915.force_probe=*/g" /etc/default/grub
		update-grub

		echo -e "\nkvmgt\nvfio-iommu-type1\nvfio-mdev\n" >> /etc/initramfs-tools/modules
		update-initramfs -u -k all

		reboot_required=1
	fi
}

function check_network(){
	wget --timeout=3 --tries=1 https://download.qemu.org/ -q -O /dev/null
	if [ $? -ne 0 ]; then
		echo "access https://download.qemu.org failed!"
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

function prepare_required_scripts(){
	chmod +x $CIV_WORK_DIR/scripts/*.sh
	cp -R $CIV_WORK_DIR/scripts/sof_audio/ $CIV_WORK_DIR/
	chmod +x $CIV_WORK_DIR/scripts/guest_pm_control
	chmod +x $CIV_WORK_DIR/scripts/findall.py
	chmod +x $CIV_WORK_DIR/scripts/thermsys
	chmod +x $CIV_WORK_DIR/scripts/batsys
}

function install_auto_start_service(){
	service_file=/etc/systemd/system/civ.service
	touch $service_file
	cat /dev/null > $service_file

	echo "[Unit]" > $service_file
	echo -e "Description=CiV Auto Start\n" >> $service_file

	echo "[Service]" >> $service_file
	echo -e "WorkingDirectory=$CIV_WORK_DIR\n" >> $service_file
	echo -e "ExecStart=/bin/bash -E $CIV_WORK_DIR/scripts/start_android.sh\n" >> $service_file

	echo "[Install]" >> $service_file
	echo -e "WantedBy=multi-user.target\n" >> $service_file
}

function ubu_thermal_conf (){
	systemctl stop thermald.service
	cp $CIV_WORK_DIR/scripts/intel-thermal-conf.xml /etc/thermald
	cp $CIV_WORK_DIR/scripts/thermald.service  /lib/systemd/system
	systemctl daemon-reload
	systemctl start thermald.service
}

version=`cat /proc/version`

if [[ $version =~ "Ubuntu" ]]; then
	check_network
	ubu_changes_require
	check_kernel
	#Auto start service for audio will be enabled in future
	#install_auto_start_service
	ubu_install_qemu_gvt
	ubu_build_ovmf_gvt
	ubu_enable_host_gvt
	if [[ $1 == "--gvtd" ]]; then
		systemctl set-default multi-user.target
	fi
	if [[ ! -d $CIV_WORK_DIR/sof_audio ]]; then
		reboot_required=1
	fi
	prepare_required_scripts
	$CIV_WORK_DIR/sof_audio/configure_sof.sh "install" $CIV_WORK_DIR
	$CIV_WORK_DIR/scripts/setup_audio_host.sh
	#starting Intel Thermal Deamon, currently supporting CML/EHL only.
	ubu_thermal_conf
	install_9p_module
	ask_reboot

elif [[ $version =~ "Clear Linux OS" ]]; then
	check_network
	clr_changes_require
	clr_enable_host_gvtg
	prepare_required_scripts
	check_kernel
	save_env
	install_9p_module
	ask_reboot
else
	echo "only clearlinux or Ubuntu is supported"
fi
