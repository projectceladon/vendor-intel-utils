=== Overview

Crashlogd is used to colloect aosp, kernel, modem crashlogs into a specific folder

--- deps

    - sepolicy
    - debug-logs


=== Options

--- true
this option will enable crashlogd in android build

    --- parameters
        - arch: this is for different platform and linux version.
        - logs_dir: this is for store logs dir.
        - binder: this is for binder dump enable/disable.
        - btdump: this is for backtrace dump enable/disable
        - ssramcrashlog: this is for enable/disable logs that stored ssram copy/move to logs_dir.
        - ramdump: this is for ramdump cp to logs_dir or not.

    --- code dir
        - device/intel/sepolicy/crashlogd
        - vendor/intel/tools/log_capture/crashlog

    --- extra files
        - init.crashlogd.rc: "Debug specific init scripts"


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option will disable crashlogd in android build
