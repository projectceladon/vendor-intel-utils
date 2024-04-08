#!/bin/bash
#
# Copyright (C) 2023-2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#

rm -rf host_kernel
mkdir -p host_kernel
cd host_kernel

git clone https://github.com/projectceladon/vendor-intel-utils.git
cd vendor-intel-utils
git checkout bcac42cad4f5f39218044fd7e02751660befa323
cd ../

git clone https://github.com/projectceladon/linux-intel-lts2022-chromium.git
cd linux-intel-lts2022-chromium
git checkout 3ac5aa048639a40e29d2c773c5451afdc661c47f

cp ../vendor-intel-utils/host/kernel/lts2022-chromium/x86_64_defconfig .config
patch_list=`find ../vendor-intel-utils/host/kernel/lts2022-chromium -iname "*.patch" | sort -u`
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
