#!/bin/bash

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel
git clone https://github.com/intel/linux-intel-lts.git -b 5.10/yocto
git clone https://github.com/intel-innersource/os.linux.kernel.kernel-config.git -b 5.10/civ_host
cd linux-intel-lts
git checkout refs/tags/lts-v5.10.100-civ-android-220303T165800Z
cp ../os.linux.kernel.kernel-config/x86_64_defconfig .config

make ARCH=x86_64 clean
make ARCH=x86_64 olddefconfig
make ARCH=x86_64 -j64 LOCALVERSION=-yvhb bindeb-pkg
