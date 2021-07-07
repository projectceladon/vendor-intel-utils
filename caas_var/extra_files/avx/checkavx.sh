#!/vendor/bin/sh

IS_AVX2=`cat /proc/cpuinfo | grep -w "avx2" | wc -l`
if [ $IS_AVX2 -gt 0 ]
   then	
     setprop vendor.dalvik.vm.isa.x86.variant kabylake
     setprop vendor.dalvik.vm.isa.x86_64.variant kabylake
fi     
