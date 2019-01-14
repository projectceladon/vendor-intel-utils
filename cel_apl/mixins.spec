[main]
mixinsdir: device/intel/project-celadon/mixins/groups
mixinsctl: true
mixinsrel: true

[mapping]
product.mk: device.mk


[groups]
device-specific: cel_apl
kernel: gmin64(useprebuilt=false,src_path=kernel/4.14, loglevel=7, interactive_governor=false, relative_sleepstates=false, modules_in_bootimg=false, external_modules=,debug_modules=, use_bcmdhd=false, use_iwlwifi=false, extmod_platform=bxt, iwl_defconfig=, cfg_path=kernel/config-lts/v4.14/kbl/android)
boot-arch: efi(uefi_arch=x86_64,fastboot=efi,ignore_rsci=true,disable_watchdog=true,watchdog_parameters=10 30,verity_warning=false,txe_bind_root_of_trust=false,bootloader_block_size=4096,verity_mode=false,data_encryption=true,target=cel_apl,rpmb_simulate=true)
graphics: mesa(gralloc1=true,gen9+=true,hwc2=true,vulkan=false,drmhwc=false,minigbm=true)
cpu-arch: slm
#thermal: dptf_configurable(intel_modem=true,thermal_lite=true,platform=kbl)
serialport: ttyS1
#gptbuild: true(size=7G)
flashfiles: ini(fast_flashfiles=false, oemvars=false,installer=true,flash_dnx_os=false,blank_no_fw=true)
storage: sdcard-mmc0-usb-sd(adoptablesd=true,adoptableusb=false)
sepolicy: permissive
widevine: L3_Gen
touch: galax7200
display-density: medium
media: mesa(add_sw_msdk=false)
public-libraries: true
hdcpd: true
codecs: configurable(hw_vd_vp9=true, hw_vd_mp2=true, hw_vd_vc1=true, platform=icl)
usb-audio-init: true
debug-npk: true(default_cfg=none, console_master_range="57 60", console_channel_range="1 4", user_master_range="72 126", user_channel_range="1 127", platform=icl)
debug-dvc_desc: npk
wlan: iwlwifi(iwl_sub_folder=dev,firmware=iwl-fw-celadon,iwl_defconfig=kbl,iwl_platform=celadon)
bluetooth: btusb(firmware=bt_fw_cel,ivi=true)
disk-bus: auto
vendor-partition: true
config-partition: true
dexpreopt: true
dalvik-heap: tablet-7in-hdpi-1024
bugreport: true
ethernet: dhcp
rfkill: true(force_disable=)
audio: project-celadon
camera-ext: ext-camera-only
usb: host+acc
usb-gadget: configfs(mtp_adb_pid=0x0a5f,ptp_adb_pid=0x0a61,rndis_pid=0x0a62,rndis_adb_pid=0x0a63,bcdDevice=0x0,bcdUSB=0x200,controller=dwc3.0.auto,f_acm=false,f_dvc_trace=false)
navigationbar: true
device-type: car
gms: default
debug-tools: true
debug-logs: true
debug-crashlogd: true
debug-coredump: true
debug-phonedoctor: true
pstore: ram_dummy(address=0x50000000,size=0x400000,record_size=0x4000,console_size=0x200000,ftrace_size=0x2000,dump_oops=1)
fota: true
trusty: true(enable_hw_sec=true, enable_storage_proxyd=true, ref_target=project-celadon_64)
factory-scripts: true
memtrack: true
security: cse
debugfs: true
lights: true
factory-partition: true
midi: true
art-config: true
cpuset: autocores
disk-encryption: true
filesystem_config: common
load_modules: true
mixin-check: true
debug-kernel: true
