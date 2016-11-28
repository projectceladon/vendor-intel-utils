#!/bin/sh

PATCH_DIR=`pwd`/device/intel/android_ia/patches

echo "Android IA: Patching AOSP Tree"

echo "Android IA: Applying patches to frameworks/native"
cd `pwd`/frameworks/native/
git am $PATCH_DIR/frameworks/native/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to frameworks/native"
echo
echo

echo "Android IA: Applying patches to frameworks/base"
cd `pwd`/frameworks/base/
git am $PATCH_DIR/frameworks/base/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to frameworks/base"
echo
echo

echo "Android IA: Applying patches to frameworks/compile/libbcc"
cd `pwd`/frameworks/compile/libbcc/
git am $PATCH_DIR/frameworks/compile/libbcc/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to frameworks/compile/libbcc"
echo
echo

echo "Android IA: Applying patches to system/core"
cd `pwd`/system/core
git am $PATCH_DIR/system/core/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to system/core"
echo
echo

echo "Android IA: Applying patches to system/bt"
cd `pwd`/system/bt
git am $PATCH_DIR/system/bt/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to system/bt"
echo
echo

echo "Android IA: Applying patches to system/vold"
cd `pwd`/system/vold
git am $PATCH_DIR/system/vold/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to system/vold"
echo
echo

echo "Android IA: Applying patches to bionic"
cd `pwd`/bionic
git am $PATCH_DIR/bionic/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to bionic"
echo
echo

echo "Android IA: Applying patches to hardware/intel/audio_media"
cd `pwd`/hardware/intel/audio_media
git am $PATCH_DIR/hardware/intel/audio_media/00*.patch
cd $OLDPWD
echo "Android IA: Succesfully applied all needed patches to hardware/intel/audio_media"
echo
echo


echo "Android IA: Succesfully applied all needed patches to AOSP tree."
