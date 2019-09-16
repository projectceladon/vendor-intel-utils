[main]
mixinsdir: device/intel/mixins
mixinsctl: true
mixinsrel: false

[mapping]
product.mk: device.mk

[groups]
allow-missing-dependencies: true
audio: aic
cpu-arch: x86_64
debug-unresponsive: false
dexpreopt: true
device-specific: cic
graphics: aic_mdc
usb: acc
wlan: mac80211_hwsim
