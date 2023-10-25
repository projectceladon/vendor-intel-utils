#!/bin/bash

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel

git clone https://github.com/projectceladon/vendor-intel-utils.git
cd vendor-intel-utils
git checkout 0babae8b1d9ad42dde9580f3ef91b640ccd86b1c
cd ../

branch_name="main"
git clone -b $branch_name https://github.com/projectceladon/linux-intel-lts2022-chromium.git

cd linux-intel-lts2022-chromium
git checkout ae3fc1db4d1ebf32cbe8ebda9e47653a9a149b71

cp ../vendor-intel-utils/host/kernel/lts2022-chromium/x86_64_defconfig .config
patch_list=`find  ../vendor-intel-utils/host/kernel/lts2022-chromium -iname "*.patch" | sort -u`
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
