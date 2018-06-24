#!/bin/bash
# -*- coding: utf-8; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-

# autopatch.py: script to manage patches on top of repo
# Copyright (c) 2015, Intel Corporation.
# Author: Falempe Jocelyn <jocelyn.falempe@intel.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

top_dir=`pwd`
utils_dir="$top_dir/vendor/intel/utils"
patch_dir="$utils_dir/android_o/google_diff/$TARGET_PRODUCT"

echo "TOP DIR : $top_dir"
echo "Utils : $utils_dir"
echo "PATCH DIR : $patch_dir"

#STEP 1: Generate Patch list and dir list
cd $patch_dir
patch_list=`find * -iname "*.patch" | sort -u`

current_project=""
previous_project=""

for i in $patch_list
do
  current_project=`dirname $i`
  if [[ $current_project != $previous_project ]]; then
    echo ""
    echo ""
    echo "Project $current_project"
  fi
  previous_project=$current_project

  cd $top_dir/$current_project
  a=`grep "Date: " $patch_dir/$i`
  b=`echo ${a#"Date: "}`
  c=`git log --pretty=format:%aD remotes/m/master..HEAD | grep "$b"`

  if [[ "$c" == "" ]] ; then
    git am $patch_dir/$i >& /dev/null
    if [[ $? == 0 ]]; then
      echo "        Applying          $i"
    else
      echo "        Conflict          $i"
      git am --abort >& /dev/null
    fi
  else
    echo "        Already applied         $i"
    #echo "$i        Not Applied [Resolve Conflicts]"
  fi
done
