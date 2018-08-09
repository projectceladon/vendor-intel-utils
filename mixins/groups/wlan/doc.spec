=== Overview

Loading wifi driver.

--- deps

    - sepolicy


=== Options

--- iwlwifi


--- mwifiex

mwifiex is used to load the marvellW8897 PCIe-wifi driver.

    --- parameters
        - NL80211: Netlink interface between kernel driver and userspace.

    --- code dir
        - device/intel/mixins/groups/wlan/mwifiex
        - device/intel/sepolicy/wlan/load_mwifiex
        - system/connectivity/wificond
        - hardware/interfaces/wifi
        - external/wpa_supplicant_8/
        - kernel/modules/marvell/wifi
        - vendor/intel/fw/marvell
