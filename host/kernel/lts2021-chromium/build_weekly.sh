#!/bin/bash

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel

git clone https://github.com/projectceladon/vendor-intel-utils.git
cd vendor-intel-utils
git checkout 1dbe7c9f53dd09e89318a566b32897091d8285da
cd ../

tag="lts-v5.15.74-20221108-r1"
git clone -b $tag https://github.com/projectceladon/linux-intel-lts2021-chromium.git
cd linux-intel-lts2021-chromium

cp ../vendor-intel-utils/host/kernel/lts2021-chromium/x86_64_defconfig .config
patch_list=`find  ../vendor-intel-utils/host/kernel/lts2021-chromium -iname "*.patch" | sort -u`
for i in $patch_list
do
  a=`grep "Date: " $i`
  b=`echo ${a#"Date: "}`
  c=`git log --pretty=format:%aD -100 | grep "$b"`
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
