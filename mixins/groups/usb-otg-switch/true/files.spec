[extrafiles]
{{^ioc}}
usb_otg_switch.sh: "Defualt SW OTG port switching script"
{{/ioc}}

{{#ioc}}
usb_otg_switch_{{ioc}}.sh: "SW OTG port switching script for specified protocol"
{{/ioc}}
