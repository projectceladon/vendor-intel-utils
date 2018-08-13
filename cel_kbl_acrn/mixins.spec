[main]
mixinsdir: device/intel/project-celadon/mixins/groups
mixinsctl: true
mixinsrel: true

[mapping]
product.mk: device.mk

[groups]
kernel: gmin64(useprebuilt=false, src_path=kernel/4.14, loglevel=4, interactive_governor=false, relative_sleepstates=true, modules_in_bootimg=false, external_modules=marvell/wifi, use_bcmdhd=false, extmod_platform=bxt, firmware_path = /vendor/firmware, disable_cpuidle_on_boot=true, cfg_path=kernel/config-lts/v4.14/bxt/android)
disk-bus: pci
boot-arch: vsbl(target=gordon_peak_acrn, rpmb=false, x64=true, use_ec_uart=true, mountfstab-flag=true, watchdog_parameters=10 30,target=gordon_peak_acrn)
sepolicy: permissive
vendor-partition: true(partition_size=800,partition_name=vendor)
config-partition: true
display-density: medium
dalvik-heap: tablet-7in-400dpi-4096
cpu-arch: slm
dexpreopt: enabled
pstore: ram_dummy(address=0x50000000,size=0x400000,record_size=0x4000,console_size=0x200000,ftrace_size=0x2000,dump_oops=1)
bugreport: default
media: mesa
graphics: mesa(gen9+=true,hwc2=true,vulkan=false,drmhwc=false,minigbm=true,gralloc1=true,cursor_wa=true)
storage: sdcard-mmc0-usb-sd(adoptablesd=true,adoptableusb=false)
ethernet: dhcp(eth-driver=igb_avb.ko,eth-arg1=tx_size=512)
rfkill: true(force_disable=)
wlan: mwifiex
codecs: configurable(hw_ve_h265=true, hw_vd_vp9=true, platform=bxt)
usb: host
usb-gadget: configfs(mtp_adb_pid=0x0a5f,ptp_adb_pid=0x0a61,rndis_pid=0x0a62,rndis_adb_pid=0x0a63,bcdDevice=0x0,bcdUSB=0x200,controller=dwc3.0.auto,f_acm=false,f_dvc_trace=false)
usb-init: true
usb-otg-switch: true
midi: true
touch: galax7200
navigationbar: true
device-type: car
gms: true(car=true)
debug-tools: true
fota: true
security: cse
net: common
debug-logs: true
debug-npk: true(default_cfg=none, console_master_range="256 259", console_channel_range="7 10", user_master_range="260 1023", user_channel_range="0 127", platform=bxt)
debug-crashlogd: true(binder=true, ssram_crashlog=broxton, ramdump=broxton)
debug-coredump: true
debug-phonedoctor: true
lights: true
power: true(wakelock_trace=true)
mediaserver-radio: true
debug-usb-config: true(source_dev=dvcith-0-msc0)
debug-dvc_desc: npk
intel_prop: true
debug-lct: true
debug-log-watch: true
readahead: optimized
eavb: true
variants: true(target=bxt)
memtrack: true
platformxml: true
dbga: true(func_test=true, mock_dev=true)
cpuset: 2cores
power: true(power_throttle=true)
audio: gordon_peak_acrn
device-specific: cel_kbl_acrn(gk_force_passthrough=true)
hdcpd: true
acrn-guest: true(size=9840M,data_size=10320M)
trusty: true(enable_hw_sec=false, enable_storage_proxyd=false, keymaster_version=1, ref_target=gordon_peak_acrn)
hyper-dmabuf-sharing: true
avb: true
slot-ab: true
allow-missing-dependencies: true
art-config: default
