#!/vendor/bin/sh

# 2G size in kB
SIZE_2G=2097152

mem_size=`cat /proc/meminfo | grep MemTotal | tr -s ' ' | cut -d ' ' -f 2`

if [ "$mem_size" -le "$SIZE_2G" ]
then
    setprop vendor.low_ram 1
else
    setprop vendor.low_ram 0
fi
