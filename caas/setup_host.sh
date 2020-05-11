#!/bin/bash

reboot_required=0
QEMU_REL=qemu-4.2.0
a=`grep -rn CIV_WORK_DIR /etc/environment`
CIV_WORK_DIR=$(pwd)

function ubu_changes_require(){
	echo "Please make sure your apt is working"
	echo "If you run the installation first time, reboot is required"
	read -p "QEMU version will be replaced (it could be recovered by 'apt purge qemu*, apt install qemu'), do you want to continue? [Y/n]" res
	if [ x$res = xn ]; then
		exit 0
	fi
	apt install -y wget mtools ovmf dmidecode python3-usb python3-pyudev pulseaudio jq
}

function ubu_install_qemu(){
	apt purge -y "qemu*"
	apt autoremove -y
	apt install -y git libfdt-dev libpixman-1-dev libssl-dev vim socat libsdl2-dev libspice-server-dev autoconf libtool xtightvncviewer tightvncserver x11vnc uuid-runtime uuid uml-utilities bridge-utils python-dev liblzma-dev libc6-dev libegl1-mesa-dev libepoxy-dev libdrm-dev libgbm-dev libaio-dev libusb-1.0.0-dev libgtk-3-dev bison libcap-dev libattr1-dev flex

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
		--enable-virtfs \
		--target-list=x86_64-softmmu \
		--audio-drv-list=pa
	make -j24
	make install
	cd ../
}

function ubu_install_qemu_gvtd(){
	apt purge -y "qemu*"
	apt autoremove -y
	apt install -y git python-dev libfdt-dev libpixman-1-dev libssl-dev vim socat libsdl2-dev libspice-server-dev autoconf libtool uml-utilities xtightvncviewer tightvncserver x11vnc uuid-runtime uuid uml-utilities bridge-utils python-dev liblzma-dev libc6-dev libegl1-mesa-dev libepoxy-dev libdrm-dev libgbm-dev libaio-dev libusb-1.0.0-dev libgtk-3-dev bison libcap-dev libattr1-dev flex gcc g++ flex pkg-config python-pip libpulse-dev uuid-runtime uuid libgl1-mesa-dri
	wget https://download.qemu.org/$QEMU_REL.tar.xz
	tar -xf $QEMU_REL.tar.xz
	cd $QEMU_REL/
	wget https://raw.githubusercontent.com/projectceladon/vendor-intel-utils/master/host/qemu/0001-Revert-Revert-vfio-pci-quirks.c-Disable-stolen-memor.patch
	patch -p1 < 0001-Revert-Revert-vfio-pci-quirks.c-Disable-stolen-memor.patch
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
	cd ../
}

function install_9p_module(){
	echo "installing 9p kernel modules for file-sharing"
	sudo modprobe 9pnet
	sudo modprobe 9pnet_virtio
	sudo modprobe 9p
	mkdir ./share_folder
}

function ubu_build_ovmf(){
	sudo apt install -y uuid-dev nasm acpidump iasl
	cd $QEMU_REL/roms/edk2
	source ./edksetup.sh
	make -C BaseTools/
	build -b DEBUG -t GCC5 -a X64 -p OvmfPkg/OvmfPkgX64.dsc -D NETWORK_IP4_ENABLE -D NETWORK_ENABLE  -D SECURE_BOOT_ENABLE -DTPM2_ENABLE=TRUE
	cp Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd ../../../OVMF.fd
	cd ../../../
}

function ubu_build_ovmf_gvtd(){
	sudo apt install -y uuid-dev nasm acpidump iasl
	cd $QEMU_REL/roms/edk2
	wget https://raw.githubusercontent.com/projectceladon/vendor-intel-utils/master/host/ovmf/OvmfPkg-add-IgdAssgingmentDxe-for-qemu-4_2_0.patch
	patch -p4 < OvmfPkg-add-IgdAssgingmentDxe-for-qemu-4_2_0.patch
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

		echo -e "\nkvmgt\nvfio-iommu-type1\nvfio-mdev\n" >> /etc/initramfs-tools/modules
		update-initramfs -u -k all

		reboot_required=1
	fi
}

function ubu_enable_host_gvtd(){
	systemctl set-default multi-user.target
	if [[ ! `cat /etc/default/grub` =~ "intel_iommu=on i915.force=probe=* " ]]; then
		read -p "The grub entry in '/etc/default/grub' will be updated for enabling GVT-d, do you want to continue? [Y/n]" res
		if [ x$res = xn ]; then
			exit 0
		fi
		sed -i "s/GRUB_CMDLINE_LINUX=\"/GRUB_CMDLINE_LINUX=\"intel_iommu=on i915.force_probe=* /g" /etc/default/grub
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
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/auto_switch_pt_usb_vms.sh
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/findall.py
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/sof_audio/configure_sof.sh
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/sof_audio/blacklist-dsp.conf
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/guest_pm_control
	chmod +x configure_sof.sh
	mkdir $CIV_WORK_DIR/sof_audio
	mv -t $CIV_WORK_DIR/sof_audio configure_sof.sh blacklist-dsp.conf
	chmod +x auto_switch_pt_usb_vms.sh
	chmod +x findall.py
	chmod +x start_flash_usb.sh
	chmod +x start_android_qcow2.sh
	chmod +x guest_pm_control
}

function save_env(){
	if [ -z "$a" ]; then
		echo "export CIV_WORK_DIR=$(pwd)" | tee -a /etc/environment
	else
		sed -i "s|export CIV_WORK_DIR.*||g" /etc/environment
		echo "export CIV_WORK_DIR=$(pwd)" | tee -a /etc/environment
	fi
}

function install_auto_start_service(){
	service_file=/etc/systemd/system/civ.service
	touch $service_file
	cat /dev/null > $service_file

	echo "[Unit]" > $service_file
	echo -e "Description=CiV Auto Start\n" >> $service_file

	echo "[Service]" >> $service_file
	echo -e "WorkingDirectory=$CIV_WORK_DIR\n" >> $service_file
	echo -e "ExecStart=/bin/bash -E $CIV_WORK_DIR/start_android.sh\n" >> $service_file

	echo "[Install]" >> $service_file
	echo -e "WantedBy=multi-user.target\n" >> $service_file
}

function ubu_thermal_conf (){
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/intel-thermal-conf.xml
	wget https://raw.githubusercontent.com/projectceladon/device-androidia-mixins/master/groups/device-specific/caas/thermald.service
	systemctl stop thermald.service
	cp intel-thermal-conf.xml /etc/thermald
	cp thermald.service  /lib/systemd/system
	systemctl daemon-reload
	systemctl start thermald.service
}

version=`cat /proc/version`

if [[ $version =~ "Ubuntu" ]]; then
	check_network
	ubu_changes_require
	save_env
	check_kernel
	#Auto start service for audio will be enabled in future
	#install_auto_start_service
	if [[ $1 == "--gvtd" ]]; then
		ubu_install_qemu_gvtd
		ubu_build_ovmf_gvtd
		ubu_enable_host_gvtd
	else
		ubu_install_qemu
		ubu_build_ovmf
		ubu_enable_host_gvtg
	fi
	if [[ ! -d $CIV_WORK_DIR/sof_audio ]]; then
		reboot_required=1
	fi
	get_required_scripts
	./sof_audio/configure_sof.sh "install" $CIV_WORK_DIR
	#starting Intel Thermal Deamon, currently supporting CML/EHL only.
	ubu_thermal_conf
	install_9p_module
	ask_reboot

elif [[ $version =~ "Clear Linux OS" ]]; then
	check_network
	clr_changes_require
	clr_enable_host_gvtg
	get_required_scripts
	check_kernel
	save_env
	install_9p_module
	ask_reboot
else
	echo "only clearlinux or Ubuntu is supported"
fi
