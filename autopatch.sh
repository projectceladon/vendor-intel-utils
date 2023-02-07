#!/bin/bash

# autopatch.sh: script to manage patches on top of repo
# Copyright (C) 2019 Intel Corporation. All rights reserved.
# Author: sgnanase <sundar.gnanasekaran@intel.com>
# Author: Sun, Yi J <yi.j.sun@intel.com>
#
# SPDX-License-Identifier: BSD-3-Clause

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
vertical_utils_dir="$top_dir/vendor/intel/utils_vertical"
vertical_patch_dir_aosp="$vertical_utils_dir/aosp_diff"
vertical_patch_dir_bsp="$vertical_utils_dir/bsp_diff"
private_vertical_utils_dir="$top_dir/vendor/intel/utils_vertical_priv"
private_vertical_patch_dir_aosp="$private_vertical_utils_dir/aosp_diff"
private_vertical_patch_dir_bsp="$private_vertical_utils_dir/bsp_diff"
vertical_prod_utils_dir="$top_dir/vendor/intel/utils_vertical_prod"
vertical_prod_patch_dir_aosp="$vertical_prod_utils_dir/aosp_diff"
vertical_prod_patch_dir_bsp="$vertical_prod_utils_dir/bsp_diff"
private_vertical_prod_utils_dir="$top_dir/vendor/intel/utils_vertical_priv_prod"
private_vertical_prod_patch_dir_aosp="$private_vertical_prod_utils_dir/aosp_diff"
private_vertical_prod_patch_dir_bsp="$private_vertical_prod_utils_dir/bsp_diff"

current_project=""
previous_project=""
conflict=""
conflict_list=""
applied_already=""
git_select=""

usage() {
	echo "USAGE"
	echo "Run below cmd to apply patches only to a specific repo:"
	echo "$ TARGET_PRODUCT=<name> vendor/intel/utils/autopatch.sh <repo name>"
	echo "OR just run to apply patches to all repos"
	echo "$ TARGET_PRODUCT=<name> vendor/intel/utils/autopatch.sh"
}

apply_patch() {

  pl=$1
  pd=$2

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
    a=`grep "Date: " ${pd}/$i | head -1`
    b=`echo ${a#"Date: "}`
    c=`git log -500 --pretty=format:%aD | grep "$b"`

    if [[ "$c" == "" ]] ; then
      git am -3 --keep-cr --whitespace=nowarn $pd/$i >& /dev/null
      if [[ $? == 0 ]]; then
        echo -e "\tApplying\t\t`basename $i`"
      else
        echo -e "\tConflicts\t\t`basename $i`"
        git am --abort >& /dev/null
        conflict="y"
        conflict_list="$current_project $conflict_list"
      fi
    else
      echo -e "\tAlready applied\t\t`basename $i`"
      applied_already="y"
    fi
  done
}

function fpnat() # find patch files and apply them
{
    local patch_top_dir=$1
    # arg #1 should be the .patch files' top directory,
    # either aosp_diff/preliminary or aosp_diff/${TARGET_PRODUCT}
    # and bsp_diff/preliminary bsp_diff/${TARGET_PRODUCT} directories.
    cd ${patch_top_dir}
    patch_file_number=`find ./$git_select -iname "*.patch" 2>/dev/null |wc -l`
    echo "Path: `basename ${patch_top_dir}` has ${patch_file_number} patch file(s) to apply!"
    if [[ ${patch_file_number} != 0 ]];then
        patch_list=`find ./$git_select -iname "*.patch" 2>/dev/null | sort -u`
        apply_patch "${patch_list}" "${patch_top_dir}"
        if [[ $? != 0 ]]; then
            echo "Apply ${patch_top_dir} patches failure!"
        fi
    fi
    unset patch_list patch_top_dir
}

if [[ $# -eq 1 ]]; then
	if [[ -d $top_dir/$1 ]]; then
		git_select="$1"
	else
		echo "$1 - this git doesn't exist"
		usage
		exit -1
	fi
fi

if [[ $# -gt 1 || $# -lt 0 ]]; then
	usage
	exit -1
fi

if [[ -e ${patch_dir_aosp}/${TARGET_PRODUCT}/include_preliminary ]];then
    echo -e "\nApply utils/aosp_diff/preliminary patches:"
    fpnat "$patch_dir_aosp/preliminary"
fi

if [[ -e ${patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${patch_dir_aosp}/${TARGET_PRODUCT}"
fi

if [[ -e ${patch_dir_bsp}/${TARGET_PRODUCT}/include_common ]];then
    echo -e "\nApply utils/bsp_diff/common Patches:"
    fpnat "${patch_dir_bsp}/common"
fi

if [[ -e ${patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${patch_dir_bsp}/${TARGET_PRODUCT}"
fi

if [[ "$conflict" == "y" ]]; then
  echo -e "\n\n\t\tALERT : Conflicts Observed while patch application !!           "
  for i in $conflict_list ; do echo $i; done | sort -u
  echo -e "\n\n\t\tError: Please resolve Conflict(s) and re-run lunch..."
  echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils/Android.mk
else
  echo -e "\n\t\tSUCCESS : All patches applied successfully in: `basename $patch_dir_aosp` and `basename ${patch_dir_bsp}` !!"
  if [[ "$applied_already" == "y" ]]; then
    echo -e "\n\t\tINFO : SOME PATCHES ARE APPLIED ALREADY  !!"
  fi
  if [ -f "$top_dir/vendor/intel/utils/Android.mk" ]; then
          sed -i '/^/d' $top_dir/vendor/intel/utils/Android.mk
  fi
fi

# Apply Embargoed patches if exist
if [[ -e ${private_utils_dir} ]] && [[ -d ${private_utils_dir} ]];then
        echo -e "\n============================="
        echo "Embargoed Patches Found"
        echo "============================="

    if [[ -e ${private_patch_dir_aosp}/${TARGET_PRODUCT}/include_preliminary ]];then
        echo -e "\nApply utils_priv/aosp_diff/preliminary Patches:"
        fpnat "${private_patch_dir_aosp}/preliminary"
    fi

    if [[ -e ${private_patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${private_patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils_priv/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${private_patch_dir_aosp}/${TARGET_PRODUCT}"
    fi

    if [[ -e ${private_patch_dir_bsp}/${TARGET_PRODUCT}/include_common ]];then
        echo -e "\nApply utils_priv/bsp_diff/common Patches:"
        fpnat "${private_patch_dir_bsp}/common"
    fi

    if [[ -e ${private_patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${private_patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils_priv/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${private_patch_dir_bsp}/${TARGET_PRODUCT}"
    fi

    if [[ "$conflict" == "y" ]]; then
      echo -e "\n\t\tALERT : Conflicts Observed while patch application !!           "
      for i in $conflict_list ; do echo $i; done | sort -u
      echo -e "\n\t\tError: Please resolve Conflict(s) and re-run lunch..."
      echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils_priv/Android.mk
    else
      echo -e "\n\t\tSUCCESS : All patches applied SUCCESSFULLY in `basename ${private_patch_dir_aosp}` and `basename ${private_patch_dir_bsp}`"
      if [[ "$applied_already" == "y" ]]; then
        echo -e "\n\t\tINFO : SOME PATCHES ARE APPLIED ALREADY  !!"
      fi
      if [ -f "$top_dir/vendor/intel/utils_priv/Android.mk" ]; then
              sed -i '/^/d' $top_dir/vendor/intel/utils_priv/Android.mk
      fi
    fi
fi

# Apply Vertical patches if exist
if [[ -e ${vertical_utils_dir} ]] && [[ -d ${vertical_utils_dir} ]];then
        echo -e "\n============================="
        echo "Vertical Patches Found"
        echo "============================="

    if [[ -e ${vertical_patch_dir_aosp}/${TARGET_PRODUCT}/include_preliminary ]];then
        echo -e "\nApply utils_vertical/aosp_diff/preliminary Patches:"
        fpnat "${vertical_patch_dir_aosp}/preliminary"
    fi

    if [[ -e ${vertical_patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${vertical_patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils_vertical/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${vertical_patch_dir_aosp}/${TARGET_PRODUCT}"
    fi

    if [[ -e ${vertical_patch_dir_bsp}/${TARGET_PRODUCT}/include_common ]];then
        echo -e "\nApply utils_vertical/bsp_diff/common Patches:"
        fpnat "${vertical_patch_dir_bsp}/common"
    fi

    if [[ -e ${vertical_patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${vertical_patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils_vertical/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${vertical_patch_dir_bsp}/${TARGET_PRODUCT}"
    fi

    if [[ "$conflict" == "y" ]]; then
      echo -e "\n\t\tALERT : Conflicts Observed while patch application !!           "
      for i in $conflict_list ; do echo $i; done | sort -u
      echo -e "\n\t\tError: Please resolve Conflict(s) and re-run lunch..."
      echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils_vertical/Android.mk
    else
      echo -e "\n\t\tSUCCESS : All patches applied SUCCESSFULLY in `basename ${vertical_patch_dir_aosp}` and `basename ${vertical_patch_dir_bsp}`"
      if [[ "$applied_already" == "y" ]]; then
        echo -e "\n\t\tINFO : SOME PATCHES ARE APPLIED ALREADY  !!"
      fi
      if [ -f "$top_dir/vendor/intel/utils_vertical/Android.mk" ]; then
              sed -i '/^/d' $top_dir/vendor/intel/utils_vertical/Android.mk
      fi
    fi
fi

# Apply Vertical private patches if exist
if [[ -e ${private_vertical_utils_dir} ]] && [[ -d ${private_vertical_utils_dir} ]];then
        echo -e "\n============================="
        echo "Vertical Embargoed Patches Found"
        echo "============================="

    if [[ -e ${private_vertical_patch_dir_aosp}/${TARGET_PRODUCT}/include_preliminary ]];then
        echo -e "\nApply private_utils_vertical/aosp_diff/preliminary Patches:"
        fpnat "${private_vertical_patch_dir_aosp}/preliminary"
    fi

    if [[ -e ${private_vertical_patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${private_vertical_patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply private_utils_vertical/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${private_vertical_patch_dir_aosp}/${TARGET_PRODUCT}"
    fi

    if [[ -e ${private_vertical_patch_dir_bsp}/${TARGET_PRODUCT}/include_common ]];then
        echo -e "\nApply private_utils_vertical/bsp_diff/common Patches:"
        fpnat "${private_vertical_patch_dir_bsp}/common"
    fi

    if [[ -e ${private_vertical_patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${private_vertical_patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply private_utils_vertical/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${private_vertical_patch_dir_bsp}/${TARGET_PRODUCT}"
    fi

    if [[ "$conflict" == "y" ]]; then
      echo -e "\n\t\tALERT : Conflicts Observed while patch application !!           "
      for i in $conflict_list ; do echo $i; done | sort -u
      echo -e "\n\t\tError: Please resolve Conflict(s) and re-run lunch..."
      echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils_vertical_priv/Android.mk
    else
      echo -e "\n\t\tSUCCESS : All patches applied SUCCESSFULLY in `basename ${private_vertical_patch_dir_aosp}` and `basename ${private_vertical_patch_dir_bsp}`"
      if [[ "$applied_already" == "y" ]]; then
        echo -e "\n\t\tINFO : SOME PATCHES ARE APPLIED ALREADY  !!"
      fi
      if [ -f "$top_dir/vendor/intel/utils_vertical_priv/Android.mk" ]; then
              sed -i '/^/d' $top_dir/vendor/intel/utils_vertical_priv/Android.mk
      fi
    fi
fi

# Apply Vertical prod patches if exist
if [[ -e ${vertical_prod_utils_dir} ]] && [[ -d ${vertical_prod_utils_dir} ]];then
        echo -e "\n============================="
        echo "Vertical prod Patches Found"
        echo "============================="

    if [[ -e ${vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT}/include_preliminary ]];then
        echo -e "\nApply utils_vertical_prod/aosp_diff/preliminary Patches:"
        fpnat "${vertical_prod_patch_dir_aosp}/preliminary"
    fi

    if [[ -e ${vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils_vertical_prod/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT}"
    fi

    if [[ -e ${vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT}/include_common ]];then
        echo -e "\nApply utils_vertical_prod/bsp_diff/common Patches:"
        fpnat "${vertical_prod_patch_dir_bsp}/common"
    fi

    if [[ -e ${vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply utils_vertical_prod/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT}"
    fi

    if [[ "$conflict" == "y" ]]; then
      echo -e "\n\t\tALERT : Conflicts Observed while patch application !!           "
      for i in $conflict_list ; do echo $i; done | sort -u
      echo -e "\n\t\tError: Please resolve Conflict(s) and re-run lunch..."
      echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils_vertical_prod/Android.mk
    else
      echo -e "\n\t\tSUCCESS : All patches applied SUCCESSFULLY in `basename ${vertical_prod_patch_dir_aosp}` and `basename ${vertical_prod_patch_dir_bsp}`"
      if [[ "$applied_already" == "y" ]]; then
        echo -e "\n\t\tINFO : SOME PATCHES ARE APPLIED ALREADY  !!"
      fi
      if [ -f "$top_dir/vendor/intel/utils_vertical_prod/Android.mk" ]; then
              sed -i '/^/d' $top_dir/vendor/intel/utils_vertical_prod/Android.mk
      fi
    fi
fi

# Apply Vertical Prod private patches if exist
if [[ -e ${private_vertical_prod_utils_dir} ]] && [[ -d ${private_vertical_prod_utils_dir} ]];then
        echo -e "\n============================="
        echo "Vertical Prod Embargoed Patches Found"
        echo "============================="

    if [[ -e ${private_vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT}/include_preliminary ]];then
        echo -e "\nApply private_utils_vertical_prod/aosp_diff/preliminary Patches:"
        fpnat "${private_vertical_prod_patch_dir_aosp}/preliminary"
    fi

    if [[ -e ${private_vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT} ]] && [[ -d ${private_vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply private_utils_vertical_prod/aosp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${private_vertical_prod_patch_dir_aosp}/${TARGET_PRODUCT}"
    fi

    if [[ -e ${private_vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT}/include_common ]];then
        echo -e "\nApply private_utils_vertical_prod/bsp_diff/common Patches:"
        fpnat "${private_vertical_prod_patch_dir_bsp}/common"
    fi

    if [[ -e ${private_vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT} ]] && [[ -d ${private_vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT} ]];then
        echo -e "\nApply private_utils_vertical_prod/bsp_diff Target ${TARGET_PRODUCT} Patches:"
        fpnat "${private_vertical_prod_patch_dir_bsp}/${TARGET_PRODUCT}"
    fi

    if [[ "$conflict" == "y" ]]; then
      echo -e "\n\t\tALERT : Conflicts Observed while patch application !!           "
      for i in $conflict_list ; do echo $i; done | sort -u
      echo -e "\n\t\tError: Please resolve Conflict(s) and re-run lunch..."
      echo '$(error "Conflicts seen while applying lunch patches !! Resolve and re-trigger")' > $top_dir/vendor/intel/utils_vertical_priv_prod/Android.mk
    else
      echo -e "\n\t\tSUCCESS : All patches applied SUCCESSFULLY in `basename ${private_vertical_prod_patch_dir_aosp}` and `basename ${private_vertical_prod_patch_dir_bsp}`"
      if [[ "$applied_already" == "y" ]]; then
        echo -e "\n\t\tINFO : SOME PATCHES ARE APPLIED ALREADY  !!"
      fi
      if [ -f "$top_dir/vendor/intel/utils_vertical_priv_prod/Android.mk" ]; then
				sed -i '/^/d' $top_dir/vendor/intel/utils_vertical_priv_prod/Android.mk
      fi
    fi
fi
