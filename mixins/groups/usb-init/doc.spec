=== Overview

usb-init is used to enable/disable loading of USB driver modules.

--- deps

    - boot-arch


=== Options

--- true
this option will enable loading of USB driver modules.

    --- code dir
        - device/intel/mixins/groups/usb-init
        - kernel/bxt/drivers/usb


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option will result in not enabling USB (but it still can be done manually).
