=== Overview

The IOC component on broxton IVI platform is responsible of
halting/rebooting the device based on two input signals. Only one
signal (SUS STAT) can be used because of a hardware bug.
Hence in order to reboot the platform we should send a can frame to
inform the IOC to reset the platform instead of going into the defaut
power state.


=== Options

--- default
when not explicitly selected in mixin spec file, the default option will be used.



