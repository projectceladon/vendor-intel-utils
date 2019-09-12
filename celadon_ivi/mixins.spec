[main]
mixinsdir: ./mixins/groups
mixinsctl: true
mixinsrel: false

[mapping]
product.mk: device.mk

[groups]
kernel: gmin64(useprebuilt=false,src_path=kernel/lts2018, loglevel=7, interactive_governor=false, relative_sleepstates=false, modules_in_bootimg=false, external_modules=,debug_modules=, use_bcmdhd=false, use_iwlwifi=false, extmod_platform=bxt, iwl_defconfig=, cfg_path=config-lts/lts2018/bxt/android/non-embargoed)
disk-bus: auto
boot-arch: project-celadon(uefi_arch=x86_64,fastboot=efi,ignore_rsci=true,disable_watchdog=true,watchdog_parameters=10 30,verity_warning=false,txe_bind_root_of_trust=false,bootloader_block_size=4096,verity_mode=false,disk_encryption=false,file_encryption=true,target=celadon_ivi,rpmb_simulate=true,ignore_not_applicable_reset=true,self_usb_device_mode_protocol=true,userdata_checkpoint=true,grub_installer=true)
sepolicy: enforcing
bluetooth: btusb(ivi=true)
audio: project-celadon
vendor-partition: true(partition_size=600,partition_name=vendor)
acpio-partition: true(partition_size=2)
config-partition: true
display-density: medium
dalvik-heap: tablet-10in-xhdpi-2048
cpu-arch: slm
allow-missing-dependencies: true
dexpreopt: true
pstore: ram_dummy(address=0x50000000,size=0x400000,record_size=0x4000,console_size=0x200000,ftrace_size=0x2000,dump_oops=1)
bugreport: true
media: mesa(add_sw_msdk=false, opensource_msdk=true)
graphics: mesa(gen9+=true,hwc2=true,vulkan=true,drmhwc=false,minigbm=true,gralloc1=true,enable_guc=false)
storage: sdcard-mmc0-usb-sd(adoptablesd=true,adoptableusb=true)
ethernet: dhcp
camera-ext: ext-camera-only
rfkill: true(force_disable=)
wlan: iwlwifi(libwifi-hal=true)
codecs: configurable(hw_ve_h265=true, hw_vd_vp9=true, hw_vd_mp2=true, hw_vd_vc1=false, platform=bxt,profile_file=media_profiles_1080p.xml)
codec2: true
usb: host
usb-gadget: configfs(usb_config=adb,mtp_adb_pid=0x0a5f,ptp_adb_pid=0x0a61,rndis_pid=0x0a62,rndis_adb_pid=0x0a63,bcdDevice=0x0,bcdUSB=0x200,controller=dwc3.0.auto,f_acm=false,f_dvc_trace=true,dvctrace_source_dev=dvcith-0-msc0)
midi: true
touch: galax7200
navigationbar: true
device-type: car(vhal-proto-type=google-emulator,aosp_hal=true)
debug-tools: true
fota: true
thermal: thermal-daemon
serialport: ttyUSB0
flashfiles: ini(fast_flashfiles=false, oemvars=false,installer=true,flash_dnx_os=false,blank_no_fw=true,version=3.0)
net: common
debug-crashlogd: true
debug-coredump: false
debug-phonedoctor: true
lights: false
power: true(power_throttle=true)
debug-usb-config: true(source_dev=dvcith-0-msc0)
intel_prop: true
trusty: true(ref_target=celadon_64)
memtrack: true
avb: true
health: true
slot-ab: true
abota-fw: true
firststage-mount: true
cpuset: autocores
usb-init: true
usb-audio-init: true
usb-otg-switch: true
vndk: true
public-libraries: true
device-specific: celadon_ivi
hdcpd: true
neuralnetworks: true
treble: true
swap: zram_auto(size=1073741824,swappiness=true,hardware=gordon_peak)
art-config: true
psdapp: true
debugfs: true
disk-encryption: true
factory-scripts: true
filesystem_config: common
load_modules: true
gptbuild: true(size=16G,generate_craff=false,compress_gptimage=true)
dynamic-partitions: true(super_img_in_flashzip=true)
dbc: true
atrace: true
firmware: true
evs: true
