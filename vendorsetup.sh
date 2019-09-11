
# autopatch.sh: script to manage patches on top of repo
# Copyright (C) 2019 Intel Corporation. All rights reserved.
# Author: sgnanase <sundar.gnanasekaran@intel.com>
# Author: Sun, Yi J <yi.j.sun@intel.com>
#
# SPDX-License-Identifier: BSD-3-Clause

# save the official lunch command to aosp_lunch() and source it
tmp_lunch=`mktemp`
sed '/ lunch()/,/^}/!d'  build/envsetup.sh | sed 's/function lunch/function aosp_lunch/' > ${tmp_lunch}
source ${tmp_lunch}
rm -f ${tmp_lunch}

function  apply_patch
{

local patch_folder=$1
echo "patch folder: $patch_folder"
# Check if there is a list of files to parse and apply patches listed in them if any
for file in `find $patch_folder -type f -o -type l 2>/dev/null` ; do
    if [[ "$TARGET_PRODUCT" == $(basename $file) ]]; then
        echo "Applying patch(es) needed for $TARGET_PRODUCT"
        vendor/intel/utils_priv/autopatch.py -f $file --remote=origin
        local ret=$?
        if [ $ret -ne 0 ]; then
            local files_with_issues="$files_with_issues $file"
        fi
        # If some patch does not apply create Android.mk to stop compilation.
        if [ -n "$files_with_issues" ]; then
            echo "\$(error \"[GOOGLE_DIFF] Some patches given to autopatch did not applied correctly in $files_with_issues.\") " > vendor/intel/utils/Android.mk
        fi
    fi
done

}

# Override lunch function to filter lunch targets
function lunch
{
    local T=$(gettop)
    if [ ! "$T" ]; then
        echo "[lunch] Couldn't locate the top of the tree.  Try setting TOP." >&2
        return
    fi

    aosp_lunch $*

    rm -rf vendor/intel/utils/Android.mk vendor/intel/utils_priv/Android.mk
    vendor/intel/utils/autopatch.sh

    if [[ -e vendor/intel/utils_priv/ ]] && [[ -e vendor/intel/utils_priv/autopatch.py ]];then
        local aosp_patch_folder=vendor/intel/utils_priv/aosp_diff
        local bsp_patch_folder=vendor/intel/utils_priv/bsp_diff
        for file in $aosp_patch_folder $bsp_patch_folder; do
            apply_patch $file
        done
    fi

    # check if product configuration files are out of date
    if [[ $(find device/intel -path "*mixins*" -prune -o -name ${TARGET_PRODUCT}.mk -print) ]]; then
        product_dir=$(dirname $(find device/intel -path "*mixins*" -prune -o -name ${TARGET_PRODUCT}.mk -print))
	echo "Executing mixin update..."
        mixinup -s $product_dir/mixins.spec
    fi
}

# Get the exact value of a build variable.
function get_build_var()
{
    if [ "$1" = "COMMON_LUNCH_CHOICES" ]
    then
        valid_targets=`mixinup -t`
        save=`build/soong/soong_ui.bash --dumpvar-mode $1`
        unset LUNCH_MENU_CHOICES
        for t in ${save[@]}; do
            array=(${t/-/ })
            target=${array[0]}
            if [[ "${valid_targets}" =~ "$target" ]]; then
                   LUNCH_MENU_CHOICES+=($t)
            fi
        done
        echo ${LUNCH_MENU_CHOICES[@]}
        return
    else
        if [ "$BUILD_VAR_CACHE_READY" = "true" ]
        then
            eval "echo \"\${var_cache_$1}\""
            return
        fi

        local T=$(gettop)
        if [ ! "$T" ]; then
            echo "Couldn't locate the top of the tree.  Try setting TOP." >&2
            return
        fi
        (\cd $T; build/soong/soong_ui.bash --dumpvar-mode $1)
    fi
}
