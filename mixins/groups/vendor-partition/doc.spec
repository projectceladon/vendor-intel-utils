=== Overview

vendor-partition is used to add new partition named vendor.

    --- dpes
        - boot-arch
        - slot-ab
        - avb
        - firststage-mount

=== Options

--- true
add vendor partition.

    --- parameters
        - partition_size: specify partition size
        - partition_name: specify partition name


--- default
when not explicitly selected in mixin spec file, the default option will be used.



--- false
empty dir.
