[main]
mixinsdir: device/intel/mixins/groups

[mapping]
product.mk: device.mk

[groups]
android_ia: default
sepolicy: permissive
graphics: android_ia(gen9+=true,hwc2=false,vulkan=false)
media: android_ia
device-type: tablet
ethernet: dhcp
debugfs: default
houdini: true
storage: default
display-density: default
adb_net: true
kernel: android_ia
bluetooth: btusb
