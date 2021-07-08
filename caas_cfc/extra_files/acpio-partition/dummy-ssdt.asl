//ACPI module device to config First-Stage Mount
DefinitionBlock ("dummy.aml", "SSDT", 1, "INTEL ", "android", 0x00001000)
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
                }
            })
        }
}
}
}
