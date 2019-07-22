#!/bin/bash
# -*- coding: utf-8; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-

# autopatch.sh: script to manage patches on top of repo
# Copyright (c) 2018, Intel Corporation.
# Author: sgnanase <sundar.gnanasekaran@intel.com>
# Author: Sun, Yi J <yi.j.sun@intel.com>
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
if [ ! "${top_dir}" ]; then
    echo "[autopatch.sh] Couldn't locate the top of the tree.  Try setting TOP." >&2
    return
fi
utils_dir="$top_dir/vendor/intel/utils"
patch_dir_aosp="$utils_dir/aosp_diff"
patch_dir_bsp="$utils_dir/bsp_diff"
private_utils_dir="$top_dir/vendor/intel/utils_priv"
private_patch_dir_aosp="$private_utils_dir/aosp_diff"
private_patch_dir_bsp="$private_utils_dir/bsp_diff"

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
    a=`grep "Date: " ${pd}/$i`
    b=`echo ${a#"Date: "}`
    c=`git log --pretty=format:%aD | grep "$b"`

    if [[ "$c" == "" ]] ; then
      git am -3 --keep-cr --whitespace=nowarn $pd/$i >& /dev/null
      if [[ $? == 0 ]]; then
        echo "        Applying          `basename $i`"
      else
        echo "        Conflicts          `basename $i`"
        git am --abort >& /dev/null
        conflict="y"
        conflict_list="$current_project $conflict_list"
      fi
    else
      echo "        Already applied         `basename $i`"
      applied_already="y"
    fi
  done
}

function fpnat() # find patch files and apply them
{
    local patch_top_dir=$1
    # arg #1 should be the .patch files' top directory,
    # either aosp_diff/common or aosp_diff/${TARGET_PRODUCT}
    # and bsp_diff/common bsp_diff/${TARGET_PRODUCT} directories.
    cd ${patch_top_dir}
    patch_list=`find * -iname "*.patch" | sort -u`
    apply_patch "${patch_list}" "${patch_top_dir}"
    if [[ $? != 0 ]]; then
        echo "Apply ${patch_top_dir} patches failure!"
    fi
    unset patch_list patch_top_dir
}

echo "Apply utils/aosp_diff/common patches:"
fpnat "$patch_dir_aosp/common"
echo "apply utils/aosp_diff/common done, sleep 60"
sleep 60
patch_list=`find * -iname "*.patch" | sort -u`
apply_patch "$patch_list" "${patch_dir_aosp}/common"
unset patch_list

if [[ -e ${patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo "Apply utils/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        cd ${patch_dir_aosp}/${TARGET_PRODUCT}
        patch_list=`find * -iname "*.patch" | sort -u`
        apply_patch "$patch_list" "${patch_dir_aosp}/${TARGET_PRODUCT}"
        unset patch_list
        if [[ $? != 0 ]]; then
            echo "Apply ${patch_dir_aosp}/${TARGET_PRODUCT} patches failure!"
        fi
fi

if [[ -e ${patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo "Apply utils/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        cd ${patch_dir_bsp}/${TARGET_PRODUCT}
        patch_list=`find * -iname "*.patch" | sort -u`
        apply_patch "$patch_list" "${patch_dir_bsp}/${TARGET_PRODUCT}"
        unset patch_list
        if [[ $? != 0 ]]; then
            echo "Apply ${patch_dir_bsp}/${TARGET_PRODUCT} patches failure!"
        fi
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
  echo -e "SUCCESS : All patches applied fine in: \n$patch_dir_aosp \nand \n${patch_dir_bsp} !!"
  echo "==========================================================================="
  if [[ "$applied_already" == "y" ]]; then
    echo "==========================================================================="
    echo "           INFO : SOME PATCHES ARE APPLIED ALREADY  !!                     "
    echo "==========================================================================="
  fi
  if [ -f "$top_dir/vendor/intel/utils/Android.mk" ]; then
          sed -i '/^/d' $top_dir/vendor/intel/utils/Android.mk
  fi
fi

echo ""
echo "Apply Embargoed patches if exist"
if [[ -e ${private_utils_dir} ]] && [[ -d ${private_utils_dir} ]];then
        echo "Embargoed Patches Found"

    ## TMEP solution, please remove this part ##
    if [[ -e ${private_patch_dir_aosp}/common ]] && [[ -d ${private_patch_dir_aosp}/common ]];then
        cd ${private_patch_dir_aosp}/common
        patch_list=`find * -iname "*.patch" | sort -u`
        apply_patch "$patch_list" "${private_patch_dir_aosp}/common"
        unset patch_list
        if [[ $? != 0 ]]; then
            echo "Apply ${private_patch_dir_aosp}/common patches failure!"
        fi
    fi
    ## TMEP solution, please remove this part ##

    if [[ -e ${private_patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${private_patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo "Apply utils_priv/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        cd ${private_patch_dir_aosp}/${TARGET_PRODUCT}
        patch_list=`find * -iname "*.patch" | sort -u`
        apply_patch "${patch_list}" "$private_patch_dir_aosp/${TARGET_PRODUCT}"
        unset patch_list
        if [[ $? != 0 ]]; then
            echo "Apply ${private_patch_dir_aosp}/${TARGET_PRODUCT} patches failure!"
        fi
    fi

    if [[ -e ${private_patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${private_patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo "Apply utils_priv/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        cd ${private_patch_dir_bsp}/${TARGET_PRODUCT}
        patch_list=`find * -iname "*.patch" | sort -u`
        apply_patch "${patch_list}" "${private_patch_dir_bsp}/${TARGET_PRODUCT}"
        unset patch_list
        if [[ $? != 0 ]]; then
            echo "Apply ${private_patch_dir_bsp}/${TARGET_PRODUCT} patches failure!"
        fi
    fi
fi

echo ""
if [[ "$conflict" == "y" ]]; then
  echo "==========================================================================="
  echo "           ALERT : Conflicts Observed while patch application !!           "
  echo "==========================================================================="
  for i in $conflict_list ; do echo $i; done | sort -u
  echo "==========================================================================="
  echo -e "Error: Please resolve Conflict(s) and re-run lunch..."
  echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils_priv/Android.mk
else
  echo "==========================================================================="
  echo -e "           SUCCESS : All patches applied fine in \n${private_patch_dir_aosp} \nand \n${private_patch_dir_bsp}"
  echo "==========================================================================="
  if [[ "$applied_already" == "y" ]]; then
    echo "==========================================================================="
    echo "           INFO : SOME PATCHES ARE APPLIED ALREADY  !!                     "
    echo "==========================================================================="
  fi
  if [ -f "$top_dir/vendor/intel/utils_priv/Android.mk" ]; then
          sed -i '/^/d' $top_dir/vendor/intel/utils_priv/Android.mk
  fi
fi
