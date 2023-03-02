#!/bin/bash

function apply()
{
    local root=$1
    local prev=$(pwd)

    echo "Applying patches in ${root}"
    if [[ -n "$DIFF_VARIANT" && -d "${root}.$DIFF_VARIANT" ]]; then
        echo "$(basename ${root}).$DIFF_VARIANT overrides $(basename ${root})"
        cd "${root}.$DIFF_VARIANT"
    elif [[ -d "${root}" ]]; then
        cd ${root}
    else
        echo -e "\nWARNING: ${root} not found, skip"
        return
    fi

    for proj in `find . -type d`
    do
        proj=${proj#./}
        [[ -z "$(ls ${root}/${proj}/*.patch 2>/dev/null)" ]] && continue

        echo "Applying patches for ${proj}"
        if [[ -d "${ROOT_DIR}/${proj}/.git" ]]; then
            cd ${ROOT_DIR}/${proj}
        else
            echo "no git repo found for ${proj}, skip"
            continue
        fi

        for name in $(ls "${root}/${proj}"); do
            echo "Applying ${name}"
            patch=${root}/${proj}/${name}
	    change_id=`grep -w "^Change-Id:" ${patch} | awk '{print $2}'`
	    # adding the condition to check if change id is missing.
			if [ -z "${change_id}" ]
			then
				echo "Error! change_id is missing in the patch"
				exit
			fi
            # we limit the log to 200 commits, or it can be very slow
            applied=`git log -200 | grep -w "^    Change-Id: ${change_id}" 2>/dev/null`
            if [[ -z "${applied}" ]]; then
                git am -k -3 --ignore-space-change --ignore-whitespace ${patch}
                if [[ $? -ne 0 ]]; then
                    conflict="y"
                    echo "Failed at ${proj}/${name}"
                    echo -e "\nALERT: Conflicts observed while applying patch!!!!"
                    echo "Abort..."
                    exit 1
                fi
            else
                echo "Already applied, continue"
            fi
        done
    done

    cd "${prev}"
}

if [[ -n "${ANDROID_BUILD_TOP}" ]]; then
    ret=`pwd | grep "${ANDROID_BUILD_TOP}"`
    if [[ -n "$ret" ]]; then
        ROOT_DIR=${ANDROID_BUILD_TOP}
    fi
fi
if [[ -z "$ROOT_DIR" ]]; then
    ret=`cat Makefile 2>/dev/null | grep "include build/make/core/main.mk"`
    if [[ -n "$ret" ]]; then
        ROOT_DIR=`pwd`
    else
        echo "Do the source & lunch first or launch this from android build top directory"
        exit 1
    fi
fi

PATCH_ROOT="${ROOT_DIR}/vendor/intel/utils"
echo "Applying patches in vendor/intel/utils"
for d in "aosp_diff" "bsp_diff"; do
    PATCH_DIR="${PATCH_ROOT}/${d}"
    echo -e "\nApplying ${d} patches\n"
    if [[ -d "${PATCH_DIR}" ]]; then
        apply "${PATCH_DIR}/common"

        if [[ -n "${TARGET_PRODUCT}" ]]; then
            apply "${PATCH_DIR}/${TARGET_PRODUCT}"
        fi
    else
        echo "no such directory: ${PATCH_DIR}, skip"
    fi
done

if [[ ! "$conflict" == "y" ]]; then
    echo -e "\nSUCCESS: All patches applied successfully"
fi
