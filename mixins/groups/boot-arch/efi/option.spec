[mixinfo]
# fstab is using shortcut /dev/block/by-name that is created by
# disk-bus mixin.
deps = disk-bus

[defaults]
acpi_permissive = false
bios_variant = release
blpolicy_use_efi_var = true
bootloader_block_size = 512
bootloader_len = 33
bootloader_policy = false
data_encryption = true
disable_watchdog = false
flash_block_size = 512
hung_task_timeout_secs = 120
ifwi_debug = false
ignore_not_applicable_reset = false
ignore_rsci = false
os_secure_boot = false
rpmb = false
rpmb_simulate = false
run_tco_on_shutdown = false
txe_bind_root_of_trust = false
uefi_arch = x86_64
use_charging_applet = false
use_power_button = false
vendor-partition = false
verity_mode = true
verity_warning = true
watchdog_parameters = false
target =
self_usb_device_mode_protocol = false
