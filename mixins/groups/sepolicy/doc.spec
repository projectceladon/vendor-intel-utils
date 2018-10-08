=== Overview

sepolicy mixin is used to configure device selinux policy and control device
enforcing mode. Note that SE Linux is a CDD required and CTS verified feature
to ship Android.

=== Options

--- permissive
This option will disable SELinux enforcing mode. IE setenforce 0.


--- default
when not explicitly selected in mixin spec file, the default option of
"permissive" will be used.

--- false
This option is nothing to do.
