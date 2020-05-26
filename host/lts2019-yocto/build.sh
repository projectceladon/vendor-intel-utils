#!/bin/bash

rm -rf cic_kernel
mkdir -p cic_kernel
cd cic_kernel
git clone https://github.com/projectceladon/vendor-intel-utils -b celadon/p/mr0/cic
git clone https://github.com/projectceladon/linux-intel-lts2019-yocto.git
cd linux-intel-lts2019-yocto
cp ../vendor-intel-utils/host/kernel/lts2019-yocto/x86_64_defconfig .config
patch_list=`find ../vendor-intel-utils/host/kernel/lts2019-yocto -iname "*.patch" | sort -u`
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
make ARCH=x86_64 -j64 LOCALVERSION=-ychb bindeb-pkg
