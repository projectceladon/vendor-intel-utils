#!/bin/bash

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel
git clone https://github.com/projectceladon/vendor-intel-utils
git clone https://github.com/intel/linux-intel-lts.git -b 5.10/yocto
cd linux-intel-lts
git checkout refs/tags/lts-v5.10.145-civ-android-221027T031053Z
cp ../vendor-intel-utils/host/kernel/lts2020-yocto/x86_64_defconfig .config

patch_list=`find ../vendor-intel-utils/host/kernel/lts2020-yocto -iname "*.patch" | sort -u`
for i in $patch_list
do
  a=`grep "Date: " $i`
  b=`echo ${a#"Date: "}`
  c=`git log --pretty=format:%aD | grep "$b"`
  if [[ "$c" == "" ]] ; then
    git am -3 --keep-cr --whitespace=nowarn $i >& /dev/null
    if [[ $? == 0 ]]; then
      echo -e "Patch\t"`basename $i` "\tApplied"
    else
      git am --abort
      echo "Not able to apply\t"`basename $i`
      break
    fi
  else
    echo -e "\tAlready applied\t\t"`basename $i`
  fi
done
make ARCH=x86_64 clean
make ARCH=x86_64 olddefconfig
make ARCH=x86_64 -j64 LOCALVERSION=-yvhb bindeb-pkg
