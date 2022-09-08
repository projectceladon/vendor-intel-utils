#!/bin/bash

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel
git clone https://github.com/projectceladon/vendor-intel-utils.git
cd vendor-intel-utils
git checkout 7507c2fa8b6b548e3889b4c5fc32d5511a8af07e
cd ../
git clone https://github.com/projectceladon/linux-intel-lts2021-chromium.git
cd linux-intel-lts2021-chromium
git checkout 94ca6268827aa5389b6e560dbed0b79f537b8f59
cp ../vendor-intel-utils/host/kernel/lts2021-chromium/x86_64_defconfig .config
patch_list=`find  ../vendor-intel-utils/host/kernel/lts2021-chromium -iname "*.patch" | sort -u`
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
      exit
    fi
  else
    echo -e "\tAlready applied\t\t"`basename $i`
  fi
done
make ARCH=x86_64 clean
make ARCH=x86_64 olddefconfig
make ARCH=x86_64 -j64 LOCALVERSION=-cvhb bindeb-pkg
