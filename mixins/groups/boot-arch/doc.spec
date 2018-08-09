=== Overview

boot-arch is used to select bootloader and specify system main fstab.

--- deps

    - variants
    - slot-ab
    - avb
    - firststage-mount
    - disk-bus

=== Options

--- efi
this option will select UEFI as bootloader.

    --- parameters
        - uefi_arch: set for TARGET_UEFI_ARCH
        - acpi_permissive: set for KERNELFLINGER_ALLOW_UNSUPPORTED_ACPI_TABLE
        - use_power_button: set for KERNELFLINGER_USE_POWER_BUTTON
        - disable_watchdog: set for KERNELFLINGER_USE_WATCHDOG
        - watchdog_parameters: set watchdog parameters
        - use_charging_applet: set for KERNELFLINGER_USE_CHARGING_APPLET
        - ignore_rsci: set for KERNELFLINGER_IGNORE_RSCI
        - magic_key_timeout: Maximum timeout to check for magic key at boot; loader GUID
        - bios_variant: set for BIOS_VARIANT
        - data_encryption: forceencrypt
        - bootloader_policy: It activates the Bootloader policy and RMA refurbishing features.
                             TARGET_BOOTLOADER_POLICY is the desired bitmask for this device
        - blpolicy_use_efi_var: set for TARGET_BOOTLOADER_POLICY_USE_EFI_VAR
        - ignore_not_applicable_reset: Allow Kernelflinger to ignore the RSCI reset source "not_applicable"
        - verity_warning: verity feature
        - txe_bind_root_of_trust: It makes kernelflinger bind the device state to the root of trust
                                  using the TXE.
        - run_tco_on_shutdown: set for iTCO_wdt.stop_on_shutdown=0 in kernel command line
        - bootloader_block_size: set for BOARD_BOOTLOADER_BLOCK_SIZE
        - hung_task_timeout_secs: set for hung_task_timeout_secs configure in sysfs
        - vendor-partition: set for support vendor partition
        - os_secure_boot: Kernelfligner will set the global variable OsSecureBoot
                          The BIOS must support this variable to enable this feature
        - verity_mode: verity feature
        - ifwi_debug: Allow to add debug ifwi file only on userdebug and eng flashfiles
        - bootloader_len: set for bootloader size
        - flash_block_size: set for BOARD_FLASH_BLOCK_SIZE
        - loader_efi_to_flash: set for fastboot boot command

    --- extra files
        - oemvars.txt:  "magic key detection timeout"

    --- code dir
        - hardware/intel/bootctrl/boot/overlay
        - device/intel/sepolicy/boot-arch/efi

