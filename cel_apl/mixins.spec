[main]
mixinsdir: device/intel/mixins/groups
mixinsctl: true

[mapping]
product.mk: device.mk


[groups]
kernel: gmin64(useprebuilt=false,src_path=kernel/4.14, loglevel=7, interactive_governor=false, relative_sleepstates=false, modules_in_bootimg=false, external_modules=,debug_modules=, use_bcmdhd=false, use_iwlwifi=false, extmod_platform=bxt, iwl_defconfig=, cfg_path=kernel/config-lts/v4.14/kbl/android)
boot-arch: efi(uefi_arch=x86_64,fastboot=efi,ignore_rsci=true,disable_watchdog=true,watchdog_parameters=10 30,verity_warning=false,txe_bind_root_of_trust=false,bootloader_block_size=4096,verity_mode=false,data_encryption=true,target=cel_apl)
graphics: mesa(gralloc1=true,gen9+=false,hwc2=true,vulkan=false,drmhwc=false,minigbm=true)
cpu-arch: slm
#thermal: dptf_configurable(intel_modem=true,thermal_lite=true,platform=kbl)
thermal: none
serialport: ttyS1
gptbuild: false
#gptbuild: true(size=7G)
flashfiles: ini(fast_flashfiles=false, oemvars=false,installer=true,flash_dnx_os=false,blank_no_fw=true)
storage: sdcard-mmcblk1-4xUSB-sda-emulated(adoptablesd=true,adoptableusb=false)
slot-ab: false
avb: false
trusty: false
sepolicy: permissive
widevine: L3_Gen
touch: galax7200
display-density: high
media: mesa(add_sw_msdk=false)
#media: none
public-libraries: true
device-specific: cel_apl
hdcpd: true
codecs: configurable(hw_vd_vp9=true, hw_vd_mp2=true, hw_vd_vc1=true, platform=icl)
usb-audio-init: true
debug-npk: true(default_cfg=none, console_master_range="57 60", console_channel_range="1 4", user_master_range="72 126", user_channel_range="1 127", platform=icl)
debug-dvc_desc: npk
wlan: iwlwifi(iwl_sub_folder=dev,firmware=iwl-fw-kbl,iwl_defconfig=kbl,iwl_platform=kbl)
bluetooth: btusb(firmware=bt_fw_wsp)
variants: false
disk-bus: auto
vendor-partition: true(partition_name=vendor)
config-partition: true
dexpreopt: enabled
dalvik-heap: tablet-7in-hdpi-1024
houdini: false
bugreport: default
ethernet: dhcp
rfkill: true(force_disable=)
gps: none
#audio: none
audio: bxtp-mrb(log=true, broadcast_radio=true)
media-audio: none
camera: none
sensors: none
usb: host+acc
usb-gadget: configfs(mtp_adb_pid=0x0a5f,ptp_adb_pid=0x0a61,rndis_pid=0x0a62,rndis_adb_pid=0x0a63,bcdDevice=0x0,bcdUSB=0x200,controller=dwc3.0.auto,f_acm=false,f_dvc_trace=false)
navigationbar: true
device-type: car
gms: default
debug-tools: true
debug-logs: true
debug-crashlogd: true(binder=true, ssram_crashlog=broxton, ramdump=icelake)
debug-coredump: true
debug-phonedoctor: true
pstore: ram_dummy(record_size=0x4000,console_size=0x200000,ftrace_size=0x2000,dump_oops=1)
fota: true
factory-scripts: true
charger: false
telephony: none
memtrack: true
security: cse
