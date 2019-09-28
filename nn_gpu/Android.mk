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
LOCAL_MODULE := android.hardware.neuralnetworks@1.2-service-gpgpu
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_INIT_RC := android.hardware.neuralnetworks@1.2-service-gpgpu.rc
LOCAL_SRC_FILES := \
service.cpp \
device.cpp \
prepare_model.cpp \
executor_manager.cpp \
base_executor.cpp \
gpu_executor.cpp \
vulkan/vk_cs_executor.cpp \
vulkan/vk_memory_manager.cpp \
vulkan/vk_pool_info.cpp \
vulkan/vk_memory_info.cpp \
vulkan/vk_operand.cpp \
vulkan/vk_buffer.cpp \
vulkan/vk_cs_executor_elewise.cpp \
vulkan/vk_cs_executor_conv.cpp \
vulkan/vk_cs_executor_depth_conv.cpp \
vulkan/vk_cs_executor_concat.cpp \
vulkan/vk_cs_executor_logistic.cpp \
vulkan/vk_cs_executor_softmax.cpp \
vulkan/vk_cs_executor_pool.cpp \
vulkan/vk_cs_executor_lrn.cpp \
vulkan/vk_cs_executor_reshape.cpp \
vulkan/vk_op_base.cpp \
vulkan/vk_wrapper.cpp \
vulkan/shader/elewise_spv.cpp \
vulkan/shader/conv_spv.cpp \
vulkan/shader/dw_conv_spv.cpp \
vulkan/shader/concat_spv.cpp \
vulkan/shader/logistic_spv.cpp \
vulkan/shader/softmax_spv.cpp \
vulkan/shader/avg_pool_spv.cpp \
vulkan/shader/conv_chn3to4_spv.cpp \
vulkan/shader/conv_gemmShader4_8_spv.cpp \
vulkan/shader/conv_gemm1_spv.cpp \
vulkan/shader/max_pool_spv.cpp \
vulkan/shader/lrn_spv.cpp \
gles/gles_cs_executor.cpp \
gles/gles_cs_executor_add.cpp \
gles/gles_cs_executor_avg_pool.cpp \
gles/gles_cs_executor_concat.cpp \
gles/gles_cs_executor_conv.cpp \
gles/gles_cs_executor_depth_conv.cpp \
gles/gles_cs_executor_logistic.cpp \
gles/gles_cs_executor_lrn.cpp  \
gles/gles_cs_executor_max_pool.cpp \
gles/gles_cs_executor_mul.cpp \
gles/gles_cs_executor_reshape.cpp \
gles/gles_cs_executor_softmax.cpp \
gles/gles_cs_program_add.cpp \
gles/gles_cs_program_avg_pool.cpp \
gles/gles_cs_program_concat.cpp \
gles/gles_cs_program_conv.cpp \
gles/gles_cs_program_depth_conv.cpp \
gles/gles_cs_program_logistic.cpp \
gles/gles_cs_program_lrn.cpp \
gles/gles_cs_program_manager.cpp \
gles/gles_cs_program_max_pool.cpp \
gles/gles_cs_program_mul.cpp \
gles/gles_cs_program_reshape.cpp \
gles/gles_cs_program_softmax.cpp \
gles/gles_memory_info.cpp \
gles/gles_memory_manager.cpp \
gles/gles_operand.cpp \
gles/gles_pool_info.cpp

LOCAL_CFLAGS += \
-DLOG_TAG=\"NN_GPU_HAL\" \
-DLOG_NDEBUG=0

ifeq ($(TARGET_PRODUCT), gordon_peak)
LOCAL_CFLAGS += -DTARGET_GORDON_PEAK
endif

ifeq ($(TARGET_PRODUCT), icl_presi_kbl)
LOCAL_CFLAGS += -DTARGET_KBL
endif

ifeq ($(TARGET_PRODUCT), celadon_tablet)
LOCAL_CFLAGS += -DTARGET_KBL
endif

LOCAL_C_INCLUDES := \
frameworks/ml/nn/common/include \
frameworks/ml/nn/runtime/include \
frameworks/native/libs/nativewindow/include \
frameworks/native/libs/ui/include \
frameworks/native/libs/nativebase/include


LOCAL_STATIC_LIBRARIES := libneuralnetworks_common

LOCAL_SHARED_LIBRARIES := \
libbase \
libdl \
libcutils \
libhardware \
libhidlbase \
libhidlmemory \
libhidltransport \
liblog \
libutils \
libEGL \
libGLESv3 \
libvulkan \
android.hardware.neuralnetworks@1.2 \
android.hardware.neuralnetworks@1.1 \
android.hardware.neuralnetworks@1.0 \
android.hidl.allocator@1.0 \
android.hidl.memory@1.0

LOCAL_MULTILIB := 64
include $(BUILD_EXECUTABLE)
