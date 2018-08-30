=== Overview

intel_prop is used to export the necessary properties based on a configuration file (intel_prop.cfg).

--- deps
    - sepolicy

=== Options

--- default
when not explicitly selected in mixin spec file, the default option will be used.




--- true
Add rule to build intel_prop tool and related service.

    --- extra files
        - intel_prop.cfg

    --- code dir
        - vendor/intel/tools/log_capture/intel_prop

--- false
Disable intel_prop tool.
