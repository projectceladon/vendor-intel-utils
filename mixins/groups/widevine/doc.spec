=== Overview

widevine is used to enable/disable Android DRM widevine feature, and set relatedsecure level.

--- deps

    - sepolicy


=== Options

--- L3_Gen
this option will enable widevine level 3 for Gen based device.

    --- code dir
        - device/intel/mixins/groups/widevine
        - device/intel/sepolicy/widevine
        - vendor/widevine


--- default
this option will only enable default drm, when not explicitly selected in mixin spec file, the default option will be used.
    --- code dir
        - device/intel/mixins/groups/widevine
        - device/intel/sepolicy/drm-default
        - hardware/interfaces/drm/1.0/default
