#!/bin/bash

function apply()
{
    cd $1
    for proj in `find . -type d -name "*"`
    do
        ls ${proj}/*.patch 1>/dev/null 2>/dev/null
        if [ $? -ne 0 ]
        then
            continue
        fi
        echo "Applying patches under ${proj}..."
        for patch_name in `ls $1/${proj}`
        do
            if [ -e ${ROOT_DIR}/${proj} ]; then
                cd ${ROOT_DIR}/${proj}
            else
                echo "no source code, skip dir: ${ROOT_DIR}/${proj}"
                break
            fi

            patch=$1/${proj}/${patch_name}
            change_id=`grep -w "^Change-Id:" ${patch} | awk '{print $2}'`
			if [ -z "${change_id}" ]
			then
				echo "Error! change_id is missing in the patch"
				exit
			fi
            ret=`git log | grep -w "^    Change-Id: ${change_id}" 2>/dev/null`
            if [ "${ret}" == "" ]
            then
                echo "Applying ${patch_name}"
                git am -k -3 --ignore-space-change --ignore-whitespace ${patch}
                if [ $? -ne 0 ]
                then
                    echo "Failed at ${proj}"
                    echo "Abort..."
                    exit
                fi
            else
                echo "Applying ${patch_name}"
                echo "Applied, ignore and continue..."
            fi
        done
        cd $1 
    done
}

if [ "${ANDROID_BUILD_TOP}" != "" ]
then
    ret=`pwd | grep "${ANDROID_BUILD_TOP}"`
    if [ "$ret" != "" ]
    then
        ROOT_DIR=${ANDROID_BUILD_TOP}
    fi
fi
if [ "$ROOT_DIR" = "" ]
then
    ret=`cat Makefile 2>/dev/null | grep "include build/make/core/main.mk"`
    if [ "$ret" != "" ]
    then
        ROOT_DIR=`pwd`
    else
        echo "Do the source & lunch first or launch this from android build top directory"
        exit
    fi
fi

DIFFS_DIR=${ROOT_DIR}

PATCH_DIR=${DIFFS_DIR}/vendor/intel/utils/android/

if [ ! -e "${PATCH_DIR}" ]
then
    echo "no such patch directory: ${PATCH_DIR}"
    exit
fi

apply ${PATCH_DIR}/common

if [ ! -z "${TARGET_PRODUCT}" ]
then
    apply ${PATCH_DIR}/${TARGET_PRODUCT}
fi
