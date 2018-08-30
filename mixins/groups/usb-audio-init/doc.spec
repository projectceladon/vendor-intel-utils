=== Overview

usb-audio-init is used to enable/disable loading of USB audio driver modules.

--- deps

    - boot-arch


=== Options

--- true
this option will enable loading of USB audio driver modules.

    --- code dir
        - device/intel/mixins/groups/usb-audio-init
        - kernel/icl/drivers/gadget/function


--- default
when not explicitly selected in mixin spec file, the default option will be used.


--- false
this option will result in not enabling USB audio (but it still can be done manually).
