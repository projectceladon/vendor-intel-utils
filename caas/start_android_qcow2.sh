#!/bin/bash

work_dir=$PWD
caas_image=$work_dir/android.qcow2
usb_switch=$work_dir/auto_switch_pt_usb_vms.sh
find_port=$work_dir/findall.py
qmp_handler=$work_dir/qmp_events_handler.sh
qmp_log=$work_dir/qmp_event.log

ovmf_file="./OVMF.fd"
[ ! -f $ovmf_file ] && ovmf_file="/usr/share/qemu/OVMF.fd"

GVTg_DEV_PATH="/sys/bus/pci/devices/0000:00:02.0"
GVTg_VGPU_UUID="4ec1ff92-81d7-11e9-aed4-5bf6a9a2bb0a"

function network_setup(){
	#setup unprivileged ICMP on the host for ping to work on guest side.
	sysctl -w net.ipv4.ping_group_range='0 2147483647'
}

function setup_vgpu(){
	res=0
	if [ ! -d $GVTg_DEV_PATH/$GVTg_VGPU_UUID ]; then
		echo "Creating VGPU..."
		sudo sh -c "echo $GVTg_VGPU_UUID > $GVTg_DEV_PATH/mdev_supported_types/i915-GVTg_V5_8/create"
		res=$?
	fi
	return $res
}

function setup_gvtd(){
	res=0
	if [ ! -d $GVTg_DEV_PATH/$GVTg_VGPU_UUID ]; then
		echo "Unbinding GPU to the host and binding dedicated GPU to the guest..."
		sudo sh -c "modprobe vfio"
		sudo sh -c "modprobe vfio_pci"
		intelVGA_id=`sudo sh -c "lspci -D -nn |grep '02.0 VGA' |grep -o 8086:[0-9a-f][0-9a-f][0-9a-f][0-9a-f]"`
		sudo sh -c "echo 0000:00:02.0 > /sys/bus/pci/devices/0000:00:02.0/driver/unbind"
		sudo sh -c "echo ${intelVGA_id/:/ } > /sys/bus/pci/drivers/vfio-pci/new_id"
		res=$?
	fi
	return $res
}

function create_snd_dummy(){
	modprobe snd-dummy
}
if [[ $1 == "--display-off" ]]
then
        display_type="none"
        ramfb_state="off"
        display_state="off"
else
        display_type="gtk,gl=on"
        ramfb_state="on"
        display_state="on"
fi

WIFI_PT="false"

for each in $@
        do
        if [[ $each == "--wifi-passthrough" ]]
                then
                WIFI_PT="true"
                echo WIFI_PT: $WIFI_PT
                break
	fi
done
BT_PT="true"

for each in $@
	do
	if [[ $each == "--bt-host" ]]
	then
		BT_PT="false"
		echo BT_PT: $BT_PT
		break
	fi
done

if [ $BT_PT = "true" ]
then
       bt_passthrough=`sed -n '/Cls=e0(wlcon)/{n;p}' /sys/kernel/debug/usb/devices \
		| grep 'Vendor=8087 ProdID=' | awk -F "=" '{print $3}' | \
		awk '{printf "-device usb-host,vendorid=0x8087,productid=0x%s\n", $1;}'`
fi

GUEST_PM="false"

for each in $@
	do
	if [[ $each == "--guest-pm" ]]
	then
		GUEST_PM="true"
		echo GUEST_PM: $GUEST_PM
		break
	fi
done

smbios_serialno=$(dmidecode -t 2 | grep -i serial | awk '{print $3}')

common_usb_device_passthrough="\
 -device qemu-xhci,id=xhci,addr=0x8 \
 `/bin/bash $usb_switch` \
 -device usb-host,vendorid=0x03eb,productid=0x8a6e \
 -device usb-host,vendorid=0x0eef,productid=0x7200 \
 -device usb-host,vendorid=0x222a,productid=0x0141 \
 -device usb-host,vendorid=0x222a,productid=0x0088 \
 $bt_passthrough \
 -device usb-mouse \
 -device usb-kbd \
"
common_guest_pm_control="\
 -qmp unix:qmp-sock,server,nowait \
 -nodefaults -no-reboot \
"

common_options="\
 -m 2048 -smp 2 -M q35 \
 -name caas-vm \
 -enable-kvm \
 -vga none \
 -display $display_type \
 -k en-us \
 -machine kernel_irqchip=off \
 -global PIIX4_PM.disable_s3=1 -global PIIX4_PM.disable_s4=1 \
 -cpu host \
 -qmp stdio \
 -drive file=$ovmf_file,format=raw,if=pflash \
 -chardev socket,id=charserial0,path=./kernel-console,server,nowait \
 -device isa-serial,chardev=charserial0,id=serial0 \
 -device intel-hda -device hda-duplex \
 -audiodev id=android_spk,timer-period=5000,server=$XDG_RUNTIME_DIR/pulse/native,driver=pa \
 -drive file=$caas_image,if=none,id=disk1 \
 -device virtio-blk-pci,drive=disk1,bootindex=1 \
 -device vhost-vsock-pci,id=vhost-vsock-pci0,guest-cid=3 \
 -device e1000,netdev=net0 \
 -netdev user,id=net0,hostfwd=tcp::5555-:5555,hostfwd=tcp::5554-:5554 \
 -device intel-iommu,device-iotlb=off,caching-mode=on \
 -full-screen \
 -fsdev local,security_model=none,id=fsdev0,path=./share_folder \
 -device virtio-9p-pci,fsdev=fsdev0,mount_tag=hostshare \
 -smbios "type=2,serial=$smbios_serialno" \
 -nodefaults
"

usb_vfio_passthrough="false"
for arg in $*
do
        if [ $arg == "--usb-host-passthrough" ]; then
               usb_vfio_passthrough="true"
               echo usb_vfio_passthrough: $usb_vfio_passthrough
               break;
        fi
done

function setup_usb_vfio_passthrough(){
        if [[ $usb_vfio_passthrough == "false" ]]; then
                common_options=${common_usb_device_passthrough}${common_options}
        else
                modprobe vfio-pci

                USBController=`sudo sh -c "lspci -D -nn | grep -i 'USB controller' | grep -i '14.0'"`
                USBControllerID=${USBController:0:13}
                USBControllerID=`echo $USBControllerID | awk '{gsub(/^\s+|\s+$/, "");print}'`
                iommu_group_devices=`sudo sh -c "ls /sys/bus/pci/devices/${USBControllerID}/iommu_group/devices/"`
                str=`echo $iommu_group_devices | sed -e 's/\s\+/,/g'`
                str=","${str}

                address=14
                g_usb_ctrl_id=0

                if [[ $1 == "setup" ]]; then
                    echo "setup USB vfio passthrough"
                    sudo sh -c "echo device > /sys/class/usb_role/intel_xhci_usb_sw-role-switch/role"
                else
                    echo "remove USB vfio passthrough"
                    sudo sh -c "echo host > /sys/class/usb_role/intel_xhci_usb_sw-role-switch/role"
                fi

                while [ -n "$str" ];do

                    substr=${str##*,}

                    usb_device=`sudo sh -c "lspci -D -nn |grep ${substr} |grep -o 8086:[0-9a-f][0-9a-f][0-9a-f][0-9a-f]"`

                    if [[ $1 == "setup" ]]; then
                        sudo sh -c "echo ${substr} > /sys/bus/pci/devices/${substr}/driver/unbind"
                        sudo sh -c "echo ${usb_device/:/ } > /sys/bus/pci/drivers/vfio-pci/new_id"
                        commandstr="-device vfio-pci,host=${substr#*:},id=gusbctrl${g_usb_ctrl_id},addr=0x${address},x-no-kvm-intx=on "
                        common_options=${commandstr}${common_options}

                        let address=address+1
                        let g_usb_ctrl_id=g_usb_ctrl_id+1
                    else
                        sudo sh -c "echo ${substr} > /sys/bus/pci/drivers/vfio-pci/unbind"
                        sudo sh -c "echo ${substr} > /sys/bus/pci/drivers_probe"
                    fi

                    str=${str%,*}
                done

                echo "enumerate done"
        fi
}

function launch_hwrender(){
	if [ $WIFI_PT = "true" ]
	then
		rfkill unblock all
		PCI_ID=`lspci -D -nn  | grep -oP '([\w:[\w.]+) Network controller' | awk '{print $1}'`
		echo $PCI_ID > /sys/bus/pci/devices/`echo $PCI_ID | sed 's/:/\\:/g'`/driver/unbind
		modprobe vfio-pci
		DEVICE_ID=`lspci -nn  | grep -oP 'Network controller.*\[\K[\w:]+'`
		echo $DEVICE_ID | sed 's/:/\ /g' > /sys/bus/pci/drivers/vfio-pci/new_id
		WIFI_VFIO_OPTIONS="-device vfio-pci,host=`lspci -nn  | grep -oP '([\w:[\w.]+) Network controller' | awk '{print $1}'`"
	fi

	setup_usb_vfio_passthrough setup

	if [ $GUEST_PM = "true" ]
	then
		 common_options=${common_guest_pm_control}${common_options}
		./guest_pm_control qmp-sock &
	fi

	if [[ $1 == "--display-off" ]]
	then
		qemu-system-x86_64 \
		-device vfio-pci-nohotplug,ramfb=$ramfb_state,sysfsdev=$GVTg_DEV_PATH/$GVTg_VGPU_UUID,display=$display_state,x-igd-opregion=on \
		-pidfile android_vm.pid \
		$WIFI_VFIO_OPTIONS \
		$common_options 1>$qmp_log 2>qemu_start_android.log <<< "{ \"execute\": \"qmp_capabilities\" }" &
		sleep 5
		cat qemu_start_android.log
		echo -n "Android started successfully and is running in background, pid of the process is:"
		cat android_vm.pid
		echo -ne '\n'
	elif [[ ($1 == "--display-off") && ($2 == "--usb-adb") ]]
	then
		echo device > /sys/class/usb_role/intel_xhci_usb_sw-role-switch/role
		echo 0000:00:14.1 > /sys/bus/pci/devices/0000\:00\:14.1/driver/unbind
		modprobe vfio-pci
		echo 8086 9d30 > /sys/bus/pci/drivers/vfio-pci/new_id
		qemu-system-x86_64 \
		-device vfio-pci-nohotplug,ramfb=$ramfb_state,sysfsdev=$GVTg_DEV_PATH/$GVTg_VGPU_UUID,display=$display_state,x-igd-opregion=on \
		-device vfio-pci,host=00:14.1,id=dwc3,addr=0x14,x-no-kvm-intx=on \
		$WIFI_VFIO_OPTIONS \
		$common_options 1>$qmp_log 2>qemu_start_android.log <<< "{ \"execute\": \"qmp_capabilities\" }" &
		sleep 5
		cat qemu_start_android.log
		echo -n "Android started successfully and is running in background, pid of the process is:"
		cat android_vm.pid
	elif [[ $1 == "--usb-adb" ]]
	then
		echo device > /sys/class/usb_role/intel_xhci_usb_sw-role-switch/role
		echo 0000:00:14.1 > /sys/bus/pci/devices/0000\:00\:14.1/driver/unbind
		modprobe vfio-pci
		echo 8086 9d30 > /sys/bus/pci/drivers/vfio-pci/new_id
		qemu-system-x86_64 \
		-device vfio-pci-nohotplug,ramfb=$ramfb_state,sysfsdev=$GVTg_DEV_PATH/$GVTg_VGPU_UUID,display=$display_state,x-igd-opregion=on \
		-device vfio-pci,host=00:14.1,id=dwc3,addr=0x14,x-no-kvm-intx=on \
		$WIFI_VFIO_OPTIONS \
		$common_options > $qmp_log <<< "{ \"execute\": \"qmp_capabilities\" }"
	else
		qemu-system-x86_64 \
		-device vfio-pci-nohotplug,ramfb=$ramfb_state,sysfsdev=$GVTg_DEV_PATH/$GVTg_VGPU_UUID,display=$display_state,x-igd-opregion=on \
		$WIFI_VFIO_OPTIONS \
		$common_options > $qmp_log <<< "{ \"execute\": \"qmp_capabilities\" }"
	fi

	setup_usb_vfio_passthrough remove
}

function launch_hwrender_gvtd(){
	setup_usb_vfio_passthrough setup
	common_options=${common_options/-display $display_type /}
	common_options=${common_options/-vga none /-vga none -nographic}
	qemu-system-x86_64 \
	-device vfio-pci,host=00:02.0,x-igd-gms=2,id=hostdev0,bus=pcie.0,addr=0x2,x-igd-opregion=on \
	${common_options/-device virtio-9p-pci,fsdev=fsdev0,mount_tag=hostshare /} > $qmp_log <<< "{ \"execute\": \"qmp_capabilities\" }"
	setup_usb_vfio_passthrough remove
}

function launch_swrender(){
	if [[ $1 == "--display-off" ]]
        then
		qemu-system-x86_64 \
		-device qxl-vga,xres=1280,yres=720 \
		-pidfile android_vm.pid \
		$common_options > $qmp_log <<< "{ \"execute\": \"qmp_capabilities\" }" &
		sleep 5
		echo -n "Android started successfully and is running in background, pid of the process is:"
		cat android_vm.pid
		echo -ne '\n'
	else
		qemu-system-x86_64 \
		-device qxl-vga,xres=1280,yres=720 \
		$common_options > $qmp_log <<< "{ \"execute\": \"qmp_capabilities\" }"
	fi
}

function check_nested_vt(){
	nested=$(cat /sys/module/kvm_intel/parameters/nested)
	if [[ $nested != 1 && $nested != 'Y' ]]; then
		echo "E: Nested VT is not enabled!"
		exit -1
	fi
}

version=`cat /proc/version`

vno=$(echo $version | \
	awk '{
		for(i=0;i<NF;i++) { if ($i == "Linux" && $(i+1) == "version") { print $(i+2); next; } }
	}'
)
if [[ "$vno" > "5.0.0" ]]; then
	check_nested_vt
	create_snd_dummy
	if [[ $1 == "--gvtd" ]]; then
		setup_gvtd
	else
		setup_vgpu
	fi
	network_setup

	rm -rf $qmp_log
	$qmp_handler &
	if [[ $? == 0 ]]; then
		if [[ $1 == "--gvtd" ]]; then
			launch_hwrender_gvtd
		else
			launch_hwrender $1
		fi
	else
		echo "W: Failed to create vgpu or bind dedicated GPU to the guest, fall to software rendering"
		launch_swrender $1
	fi
else
	echo "E: Detected linux version $vno"
	echo "E: Please upgrade kernel version newer than 5.0.0!"
	exit -1
fi

