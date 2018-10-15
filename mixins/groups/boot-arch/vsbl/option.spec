[mixinfo]
# fstab is using shortcut /dev/block/by-name that is created by
# disk-bus mixin.
deps = disk-bus

[defaults]
disk_encryption = false
file_encryption = false
verity_warning = true
verity_mode = true
watchdog_parameters = 10 30
run_tco_on_shutdown = false
hung_task_timeout_secs = 120
vendor-partition = false
ifwi_debug = false
config = gr_mrb
prebuilts_dir = gordon_peak_acrn
timeout = 60000
bootloader_block_size = 1024
bootloader_len = 30
fastboot_in_ifwi = true
xen_partition = false
rpmb = true
use_ipp_sha256 = false
large-cache-partition = false
mountfstab-flag = true
use_ec_uart = false
ifwi_variant = release
x64 = false
mountfstab-flag = true
acrn-fstab-flag = false
target = gordon_peak_acrn
data_use_f2fs = false
