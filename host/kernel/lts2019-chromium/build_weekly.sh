#!/bin/bash

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel
#git clone https://github.com/projectceladon/vendor-intel-utils
git clone https://chromium.googlesource.com/chromiumos/third_party/kernel
cd kernel
git checkout 7ce9d039b0b4636826b89a39ecdbeaa7fd4e2ac3
cp ../../x86_64_defconfig .config
patch_list=`find ../../ -iname "*.patch" | sort -u`
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
make ARCH=x86_64 -j64 LOCALVERSION=-cvhb bindeb-pkg
