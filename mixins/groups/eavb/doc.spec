=== Overview

eavb mixin is used to enable/disable eAVB feature intel solution and related components.

=== Supported

--- d6 : change to d6_1722a compatibility, can be done using "persist.d6.mode" android property

--- Master/Slave : switch between master/slave using "persist.eavb.mode" android property

--- Profile Name : change avb profile using "persist.avb.profile.name" android property

=== Options

--- true
use this to enable eAVB

	--- parameters
		- automotive_profile: y or n - setup gptp clock synchronization for automotive or non-automotive

	--- code dir
		- vendor/intel/ias/media_transport/avb_streamhandler: AVB Stream Handler
		- vendor/intel/ias/ias-audio-smartx : User-space library that allows
		 to define and control all aspects of user-space audio routing and processing.
		- vendor/intel/external/open-avb : GPTP daemon and igb module.
		- device/intel/sepolicy/gptp : enable SELinux rules for gptp daemon.
		- device/intel/sepolicy/avbstreamhandler : enable SELinux rules for avbstreamhandler daemon.
		- hardware/intel/audio/coe/primary/parameter_framework_plugin/smartx : parameter_framework smartx plugin.


