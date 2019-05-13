#!/bin/bash
# -*- coding: utf-8; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-

# autopatch.sh: script to manage patches on top of repo
# Copyright (c) 2018, Intel Corporation.
# Author: sgnanase <sundar.gnanasekaran@intel.com>
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
common_patch_dir="$utils_dir/android_p/common"
patch_dir="$utils_dir/android_p/target/$TARGET_PRODUCT"
private_utils_dir="$top_dir/vendor/intel/PRIVATE/utils"
private_patch_dir="$private_utils_dir/android_p/google_diff/$TARGET_PRODUCT"

current_project=""
previous_project=""
conflict=""
conflict_list=""
applied_already=""

apply_patch() {

  pl=$1
  pd=$2

  echo ""
  echo "Applying Patches"

  for i in $pl
  do
    current_project=`dirname $i`
    if [[ $current_project != $previous_project ]]; then
      echo ""
      echo ""
      echo "Project $current_project"
    fi
    previous_project=$current_project

    cd $top_dir/$current_project
    remote=`git remote -v | grep "https://android.googlesource.com/"`
    if [[ -z "$remote" ]]; then
      default_revision="remotes/m/master"
    else
      if [[ -f "$top_dir/.repo/manifest.xml" ]]; then
        default_revision=`grep default $top_dir/.repo/manifest.xml | grep -o 'revision="[^"]\+"' | cut -d'=' -f2 | sed 's/\"//g'`
      else
        echo "Please make sure .repo/manifest.xml"
        exit 1
      fi
    fi

    cd $top_dir/$current_project
    a=`grep "Date: " $pd/$i`
    b=`echo ${a#"Date: "}`
    c=`git log --pretty=format:%aD | grep "$b"`

    if [[ "$c" == "" ]] ; then
      git am $pd/$i >& /dev/null
      if [[ $? == 0 ]]; then
        echo "        Applying          $i"
      else
        echo "        Conflicts          $i"
        git am --abort >& /dev/null
        conflict="y"
        conflict_list="$current_project $conflict_list"
      fi
    else
      echo "        Already applied         $i"
      applied_already="y"
    fi
  done
}

#Apply common patches
cd $common_patch_dir
common_patch_list=`find * -iname "*.patch" | sort -u`
apply_patch "$common_patch_list" "$common_patch_dir"

#Aplly target specific patches
cd $patch_dir
patch_list=`find * -iname "*.patch" | sort -u`
apply_patch "$patch_list" "$patch_dir"

#Apply Embargoed patches if exist
if (( 0 )) ; then
    echo ""
    echo "Embargoed Patches Found"
    cd $private_patch_dir
    private_patch_list=`find * -iname "*.patch" | sort -u`
    apply_patch "$private_patch_list" "$private_patch_dir"
fi

echo ""
if [[ "$conflict" == "y" ]]; then
  echo "==========================================================================="
  echo "           ALERT : Conflicts Observed while patch application !!           "
  echo "==========================================================================="
  for i in $conflict_list ; do echo $i; done | sort -u
  echo "==========================================================================="
  echo -e "Error: Please resolve Conflict(s) and re-run lunch..."
  echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils/Android.mk
else
  echo "==========================================================================="
  echo "           SUCCESS : All patches applied fine !!                           "
  echo "==========================================================================="
  if [[ "$applied_already" == "y" ]]; then
    echo "==========================================================================="
    echo "           INFO : SOME PATCHES ARE APPLIED ALREADY  !!                     "
    echo "==========================================================================="
  fi
  sed -i '/^/d' $top_dir/vendor/intel/utils/Android.mk
fi
