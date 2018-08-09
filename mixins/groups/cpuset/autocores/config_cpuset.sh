#!/vendor/bin/sh
 
 cp /sys/devices/system/cpu/online /dev/cpuset/foreground/cpus
 cp /sys/devices/system/cpu/online /dev/cpuset/background/cpus
 cp /sys/devices/system/cpu/online /dev/cpuset/system-background/cpus
 cp /sys/devices/system/cpu/online /dev/cpuset/top-app/cpus
 cp /sys/devices/system/cpu/online /dev/cpuset/foreground/boost/cpus

