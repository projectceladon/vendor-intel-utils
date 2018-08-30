=== Overview

debug-dvc_desc is for sofia based usb dvc debug.

--- deps

    - sepolicy


=== Options

--- npk
this option will enable usb dvc debug.

    --- parameters
        - source_dev: this defines which device is used for logs transfer.

    --- extra files
        - init.dvc_desc.rc: "DvC desc init file"
        - dvc_descriptors.cfg: "DvC Trace configuration file"


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option will disable usb dvc debug.
