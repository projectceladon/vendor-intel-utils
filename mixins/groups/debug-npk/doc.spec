=== Overview

npk is used for output logs through npk.

--- deps

    - sepolicy
    - debug-logs


=== Options

--- true
this option will enable npk driver log output

    --- parameters
        - default_cfg: it used to set input logs and output type

    --- code dir
	- drivers/hwtracing/intel_th

    --- extra files
        - init.npk.rc: "Debug specific init scripts"


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option will disable npk log output.
