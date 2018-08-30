=== Overview

phone doctor is used to parse and upload crashlog to crashtool server

--- deps

    - debug-crashlogd
    - sepolicy


=== Options

--- true
this option will enable phone doctor into android build.

    --- code dir
        - vendor/intel/tools/log_infra


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option will disable phone doctor in android build.
