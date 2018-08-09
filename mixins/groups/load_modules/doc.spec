=== Overview

This mixin group provides a service to load driver modules.
when load driver modules by a service, a service is in parallel with android main init process,
thus save total boot up time.

service script is built from source listed in LOAD_MODULES_IN and LOAD_MODULES_H_IN varible.
each component want to use this service for their modules load, need only to create
a modules_load.in and append it to LOAD_MODULES_IN in its own mixin group/configure, then it will be built to service script.

modules for one component need to write in a function, then it can be loaded in parallel with other components.

when a component is critical to a cold boot KPI, then its module_load.in should be in LOAD_MODULES_H_IN for higher priority.
this service will load high priority modules, and wait it finish, then load normal priority modules.

=== Options

--- default
use load modules service for modules load
load it at ealry-init when firststage-mount is true, otherwise at fs.
use /vendor/bin/sh as interpreter when treble is true, otherwise use /system/bin/sh

