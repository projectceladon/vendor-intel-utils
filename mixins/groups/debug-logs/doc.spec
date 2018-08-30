=== Overview

debug logs is used to  collect logs from Android continuously.

--- deps

    - sepolicy


=== Options

--- true
this option will enable android build logs

    --- parameters
        - eng_only: this is for enable logs on eng build only or for other debug as well.
        - logs_dir: this set android logs store directory
        - logger_pack: this is for zip logs tool.
        - logger: this is for which log source is used for output
        - logger_rot_cnt: this is for logger buffer number.
        - logger_rot_size: this is for each buffer size
        - klogd_raw_message: this is for output raw message or not.

    --- extra files
        - init.logs.rc: "Debug specific init scripts"


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option will disable all android platform logs to specific folder.
