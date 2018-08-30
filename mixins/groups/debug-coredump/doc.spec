=== Overview

Coredump is used to enable/disable AOSP core dump feature when ANR, Tombstone happened.

--- deps

    - sepolicy


=== Options

--- true
this option will enable core dump.

    --- extra files
        - init.coredump.rc: "Debug specific init scripts"


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option will disable core dump.
