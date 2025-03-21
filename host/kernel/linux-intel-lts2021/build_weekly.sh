#!/bin/bash

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel

git clone https://github.com/projectceladon/vendor-intel-utils.git
cd vendor-intel-utils
git checkout 86c5e151362e92cb6c10f702f7be33a89404b916
cd ../

git clone -b lts-v5.15.71-20221130-r1 https://github.com/projectceladon/linux-intel-lts2021.git
cd linux-intel-lts2021

cp ../vendor-intel-utils/host/kernel/linux-intel-lts2021/x86_64_defconfig .config
patch_list=`find  ../vendor-intel-utils/host/kernel/linux-intel-lts2021 -iname "*.patch" | sort -u`
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
make ARCH=x86_64 -j64 LOCALVERSION=-vhb bindeb-pkg
