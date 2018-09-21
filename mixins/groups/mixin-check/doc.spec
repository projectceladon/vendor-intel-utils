=== Overview

mixin-check is used to fail build if any of the targets is out-of-date.
It forces devs to understand the impact of their change on all targets.
Also avoids random master breakges cause of mixins being out-of-date.

=== Options

--- default
when not explicitly selected in mixin spec file, the default option will be used.




--- false
empty dir.

--- true
enable mixin-check.
