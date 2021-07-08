//ACPI module device to config First-Stage Mount
DefinitionBlock ("firststage-mount.aml", "SSDT", 1, "INTEL ", "android", 0x00001000)
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
                    Package () {"android.vbmeta.parts","vbmeta,boot,system,vendor,product"},
                    Package () {"android.fstab.compatible", "android,fstab"},
                    Package () {"android.fstab.vendor.compatible", "android,vendor"},
                    Package () {"android.fstab.vendor.dev", "/dev/block/pci/pci0000:00/0000:00:ff.ff/by-name/vendor"},  // Varies with platform
                    Package () {"android.fstab.vendor.type", "ext4"},  // May vary with platform
                    Package () {"android.fstab.vendor.mnt_flags", "ro"},  // May vary with platform
                    Package () {"android.fstab.vendor.fsmgr_flags", "wait,slotselect,avb"},  // May vary with platform
                    Package () {"android.fstab.product.compatible", "android,product"},
                    Package () {"android.fstab.product.dev", "/dev/block/pci/pci0000:00/0000:00:ff.ff/by-name/product"},  // Varies with platform
                    Package () {"android.fstab.product.type", "ext4"},  // May vary with platform
                    Package () {"android.fstab.product.mnt_flags", "ro"},  // May vary with platform
                    Package () {"android.fstab.product.fsmgr_flags", "wait,slotselect,avb"},  // May vary with platform
                }
            })
        }
}
}
}
