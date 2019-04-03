# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.neuralnetworks@1.0-service-gpgpu
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_INIT_RC := android.hardware.neuralnetworks@1.0-service-gpgpu.rc
LOCAL_SRC_FILES := \
service.cpp \
device.cpp \
prepare_model.cpp \
executor_manager.cpp \
base_executor.cpp \
gpu_executor.cpp \
validate.cpp \
gles/gles_cs_executor.cpp \
gles/gles_memory_manager.cpp \
gles/gles_pool_info.cpp \
gles/gles_memory_info.cpp \
gles/gles_operand.cpp \
gles/gles_cs_program_manager.cpp \
gles/gles_cs_executor_add.cpp \
gles/gles_cs_program_add.cpp \
gles/gles_cs_executor_conv.cpp \
gles/gles_cs_program_conv.cpp \
gles/gles_cs_executor_logistic.cpp \
gles/gles_cs_program_logistic.cpp \
gles/gles_cs_executor_mul.cpp \
gles/gles_cs_program_mul.cpp \
gles/gles_cs_executor_depth_conv.cpp \
gles/gles_cs_program_depth_conv.cpp \
gles/gles_cs_executor_avg_pool.cpp \
gles/gles_cs_program_avg_pool.cpp \
gles/gles_cs_executor_softmax.cpp \
gles/gles_cs_program_softmax.cpp \
gles/gles_cs_executor_reshape.cpp \
gles/gles_cs_program_reshape.cpp \
gles/gles_cs_executor_concat.cpp \
gles/gles_cs_program_concat.cpp \
gles/gles_cs_executor_lrn.cpp \
gles/gles_cs_program_lrn.cpp \
gles/gles_cs_executor_max_pool.cpp \
gles/gles_cs_program_max_pool.cpp 

LOCAL_CFLAGS += \
-DLOG_TAG=\"NNGPUHAL\" \
-DLOG_NDEBUG=0

ifeq ($(TARGET_PRODUCT), gordon_peak)
LOCAL_CFLAGS += -DTARGET_GORDON_PEAK
endif

ifeq ($(TARGET_PRODUCT), icl_presi_kbl)
LOCAL_CFLAGS += -DTARGET_KBL
endif

LOCAL_SHARED_LIBRARIES := \
libbase \
libdl \
libcutils \
libhardware \
libhidlbase \
libhidlmemory \
libhidltransport \
libtextclassifier_hash \
liblog \
libutils \
libEGL \
libGLESv3 \
android.hardware.neuralnetworks@1.0 \
android.hidl.allocator@1.0 \
android.hidl.memory@1.0

LOCAL_MULTILIB := 64
include $(BUILD_EXECUTABLE)
