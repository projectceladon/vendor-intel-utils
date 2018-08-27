=== Overview

flashfiles is used to define the flash procedure

--- deps

    - variants
    - slot-ab

=== Options

--- ini
this option is used to use ini configure for flash

    --- parameters
        - installer: for support flashing installer.efi
        - oemvars: Set OEM variables
        - timeout: for flash timeout
        - capsule: Delete firmware capsule
        - reboot: reboot command
        - version: version number
        - bts: btsdata.ini
        - fastboot_min_battery_level: set fastboot_min_battery_level
        - retry: rety times
        - firmware_info: firmware-info.txt
        - flash:for flash
        - clearrpmb_emmc:for clearrpmb_emmc
        - clearrpmb_ufs:for clearrpmb_ufs
        - startover:for startover
        - bootloader: configure bootloader
        - fast_flashfiles: configure FAST_FLASHFILES used in compile
        - flash_dnx_os: dnx mode flash

    --- code dir

    --- extra files
        - flashfiles.ini: "define the flash procedure for phoneflashtool and installer.efi"


--- default
when not explicitly selected in mixin spec file, the default option will be used.
