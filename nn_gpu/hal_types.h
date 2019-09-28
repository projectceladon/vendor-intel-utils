#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_HAL_TYPES_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_HAL_TYPES_H

#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModelCallback.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.2/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.2/IPreparedModelCallback.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include "base.h"
#include "Utils.h"

NAME_SPACE_BEGIN

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_handle;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::neuralnetworks::V1_0::DataLocation;
using ::android::hardware::neuralnetworks::V1_0::DeviceStatus;
using ::android::hardware::neuralnetworks::V1_0::ErrorStatus;
using ::android::hardware::neuralnetworks::V1_0::FusedActivationFunc;
using ::android::hardware::neuralnetworks::V1_0::OperandLifeTime;
using ::android::hardware::neuralnetworks::V1_0::Request;
using ::android::hardware::neuralnetworks::V1_0::PerformanceInfo;
using ::android::hardware::neuralnetworks::V1_0::RequestArgument;
using ::android::hardware::neuralnetworks::V1_1::ExecutionPreference;
using ::android::hardware::neuralnetworks::V1_0::Capabilities;
using ::android::hardware::neuralnetworks::V1_2::Constant;
using ::android::hardware::neuralnetworks::V1_2::DeviceType;
using ::android::hardware::neuralnetworks::V1_2::Extension;
using ::android::hardware::neuralnetworks::V1_2::FmqRequestDatum;
using ::android::hardware::neuralnetworks::V1_2::FmqResultDatum;
using ::android::hardware::neuralnetworks::V1_2::IBurstCallback;
using ::android::hardware::neuralnetworks::V1_2::IBurstContext;
using ::android::hardware::neuralnetworks::V1_2::IDevice;
using ::android::hardware::neuralnetworks::V1_2::IExecutionCallback;
using ::android::hardware::neuralnetworks::V1_2::IPreparedModel;
using ::android::hardware::neuralnetworks::V1_2::IPreparedModelCallback;
using ::android::hardware::neuralnetworks::V1_2::MeasureTiming;
using ::android::hardware::neuralnetworks::V1_2::Model;
using ::android::hardware::neuralnetworks::V1_2::Operand;
using ::android::hardware::neuralnetworks::V1_2::OperandType;
using ::android::hardware::neuralnetworks::V1_2::OperandTypeRange;
using ::android::hardware::neuralnetworks::V1_2::Operation;
using ::android::hardware::neuralnetworks::V1_2::OperationType;
using ::android::hardware::neuralnetworks::V1_2::OperationTypeRange;
using ::android::hardware::neuralnetworks::V1_2::OutputShape;
using ::android::hardware::neuralnetworks::V1_2::SymmPerChannelQuantParams;
using ::android::hardware::neuralnetworks::V1_2::Timing;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

NAME_SPACE_STOP

#endif
