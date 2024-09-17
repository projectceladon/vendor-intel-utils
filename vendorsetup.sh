
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
# Override lunch function to filter lunch targets
function lunch
{
    local T=$(gettop)
    if [ ! "$T" ]; then
        echo "[lunch] Couldn't locate the top of the tree.  Try setting TOP." >&2
        return
    fi
    local answer=' '
    # Set the TARGET_RELEASE variable to the release_config for
    # which we want to build CELADON. It should be one among
    # $(TOP)/build/release/release_configs/*
    # When updating TARGET_RELEASE value here please change in 
    # "function _get_build_var_cached" below too.
    TARGET_RELEASE=ap3a
    if [ "$1" ]; then
        local valid_targets=`mixinup -t`
        local count=`echo "$1" | grep -o "-" | wc -l`
        if [ "${count}" -eq 1 ]; then
            local array=(${1/-/ })
            local target=${array[0]}
            if [[ "${valid_targets}" =~ "${target}" ]]; then
                answer=${target}-$TARGET_RELEASE-${array[1]}
            else
                answer=$1
            fi
        else
            answer=$1
        fi
    fi

    aosp_lunch $answer

    rm -rf vendor/intel/utils/Android.mk vendor/intel/utils_priv/Android.mk
    vendor/intel/utils/autopatch.sh

    # check if product configuration files are out of date
    if [[ $(find device/intel -path "*mixins*" -prune -o -name ${TARGET_PRODUCT}.mk -print) ]]; then
        product_dir=$(dirname $(find device/intel -path "*mixins*" -prune -o -name ${TARGET_PRODUCT}.mk -print))
	echo "Executing mixin update..."
        mixinup -s $product_dir/mixins.spec
    fi
}

# Get the exact value of a build variable.
function _get_build_var_cached()
{
    # Set the TARGET_RELEASE variable to the release_config for
    # which we want to build CELADON. It should be one among
    # $(TOP)/build/release/release_configs/*
    TARGET_RELEASE=ap3a
    if [ "$1" = "COMMON_LUNCH_CHOICES" ]
    then
        valid_targets=`mixinup -t`
        save=`build/soong/soong_ui.bash --dumpvar-mode $1`
        unset LUNCH_MENU_CHOICES
        for t in ${save[@]}; do
            array=(${t/-/ })
            target=${array[0]}
            if [[ "${valid_targets}" =~ "$target" ]]; then
                   tgt=$target-$TARGET_RELEASE-${array[1]}
                   LUNCH_MENU_CHOICES+=($tgt)
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
