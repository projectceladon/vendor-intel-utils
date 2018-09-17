//ACPI module device to config First-Stage Mount
DefinitionBlock ("ssdt.aml", "SSDT", 1, "INTEL ", "general", 0x00001000)
{
Scope (\)
{
External (\_SB.CFG0, DeviceObj)
Scope(_SB)
{
        Device (ANDT)
        {
            Name (_HID, "ANDR0001")
            Name (_STR, Unicode("android device tree"))  // Optional

            Name (_DSD, Package () {
                ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
                Package () {
                    Package () {"android.compatible", "android,firmware"},
                    Package () {"android.vbmeta.compatible","android,vbmeta"},
                    Package () {"android.vbmeta.parts","vbmeta,boot,system,vendor{{#avb}}{{^slot-ab}},recovery{{/slot-ab}}{{/avb}}{{#trusty}},tos{{/trusty}}"},
                    Package () {"android.fstab.compatible", "android,fstab"},
                    Package () {"android.fstab.vendor.compatible", "android,vendor"},
                    Package () {"android.fstab.vendor.dev", "/dev/block/pci/pci0000:00/0000:00:03.0/by-name/vendor"},  // Varies with platform
                    Package () {"android.fstab.vendor.type", "ext4"},  // May vary with platform
                    Package () {"android.fstab.vendor.mnt_flags", "ro"},  // May vary with platform
                    Package () {"android.fstab.vendor.fsmgr_flags", "wait{{#slot-ab}},slotselect{{/slot-ab}}{{#avb}},avb{{/avb}}"},  // May vary with platform
{{^avb}}
                    Package () {"android.fstab.system.compatible", "android,system"},
                    Package () {"android.fstab.system.dev", "/dev/block/pci/pci0000:00/0000:00:03.0/by-name/system"},  // Varies with platform
                    Package () {"android.fstab.system.type", "ext4"},  // May vary with platform
                    Package () {"android.fstab.system.mnt_flags", "ro"},  // May vary with platform
                    Package () {"android.fstab.system.fsmgr_flags", "wait"},  // May vary with platform
{{/avb}}
{{#avb}}
{{^slot-ab}}
                    Package () {"android.fstab.system.compatible", "android,system"},
                    Package () {"android.fstab.system.dev", "/dev/block/pci/pci0000:00/0000:00:03.0/by-name/system"},  // Varies with platform
                    Package () {"android.fstab.system.type", "ext4"},  // May vary with platform
                    Package () {"android.fstab.system.mnt_flags", "ro"},  // May vary with platform
                    Package () {"android.fstab.system.fsmgr_flags", "wait,avb"},  // May vary with platform
{{/slot-ab}}
{{/avb}}
                }
            })
        }
}
}
}
