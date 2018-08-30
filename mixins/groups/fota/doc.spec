=== Overview

fota is used to configure fota support


=== Options

--- true
this option is used to add fota support

    --- parameters
        - eula: end user license agreement
        - log_level: define log level
        - update_stream:

    --- code dir
        - vendor/intel/apps/fota/


--- default
when not explicitly selected in mixin spec file, the default option will be used.

--- false
this option is used to not add fota support

    --- parameters

    --- code dir
