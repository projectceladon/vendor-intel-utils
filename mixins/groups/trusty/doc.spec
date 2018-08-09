=== Overview

trusty is used to enable/disable VMM based TEE solution.

--- deps

    - sepolicy
    - boot-arch


=== Options

--- true
this option will enable VMM based TEE.

    --- parameters
        - lk_core_num: specify the core number which LK is running on.
        - lk_project: specify the LK project name
        - enable_hw_sec: specify whether to use hardware-backed KM/GK/SS.
        - enable_storage_proxyd: specify whether to start storageproxyd service when kernel boot up.
        - ref_target: the refernece target

    --- code dir
        - device/intel/mixins/groups/trusty
        - device/intel/sepolicy/trusty
        - vendor/intel/fw/evmm
        - trusty
        - system/core/trusty
        - system/gatekeeper
        - system/keymaster
        - vendor/intel/hardware/keystore
        - vendor/intel/hardware/storage
        - hardware/interfaces/gatekeeper
        - hardware/interfaces/keymaster
        - hardware/intel/kernelflinger/libqltipc
        - kernel/bxt/drivers/trusty

