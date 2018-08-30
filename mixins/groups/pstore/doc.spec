=== Overview

pstore is used for record kernel panic logs when kernel panic happened, it will move logs to specific folder after reboot

--- deps

    - sepolicy

=== Options

--- ram_dummy
this option use a dummy device where RAM buffer is
passed as cmdline parameters, then store logs in RAm buffer

    --- parameters
        - record_size: this is the buffer size for log buffer
        - console_size: this is the buffer size for record kernel logs.
        - ftrace_size: this is the functional trace buffer size
        - dump_oops: true or false when panic to dump oops.
        - ecc: if non-zero, the parameter enables ECC support and specifies "
                "ECC buffer size in bytes (1 is a special value, means 16 "
                "bytes ECC)"".

    --- code dir
        - fs/pstore/ram.c


--- default
when not explicitly selected in mixin spec file, the default option will be used.
