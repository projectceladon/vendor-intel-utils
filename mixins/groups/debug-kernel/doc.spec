=== Overview

debug kernel is used to detect some specific panic and hungs happened in kernel.

--- deps

    - n/a


=== Options

--- default
when not explicitly selected in mixin spec file, the default option will be used.


--- true

    --- parameters
        - hung_detect: this will enable/disable hung detect in kernel
        - hung_task_timeout_secs: this set hung task timeout in second.

    --- extra files
        - init.kernel.rc: "Debug specific init scripts"
