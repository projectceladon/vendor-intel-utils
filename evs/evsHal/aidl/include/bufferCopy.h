/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CPP_EVS_SAMPLEDRIVER_AIDL_INCLUDE_BUFFERCOPY_H
#define CPP_EVS_SAMPLEDRIVER_AIDL_INCLUDE_BUFFERCOPY_H

#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <android/hardware_buffer.h>

namespace aidl::android::hardware::automotive::evs::implementation {

void fillNV21FromNV21(const ::aidl::android::hardware::automotive::evs::BufferDesc& tgtBuff,
                      uint8_t* tgt, void* imgData, unsigned imgStride);

void fillNV21FromYUYV(const ::aidl::android::hardware::automotive::evs::BufferDesc& tgtBuff,
                      uint8_t* tgt, void* imgData, unsigned imgStride);

void fillRGBAFromYUYV(const ::aidl::android::hardware::automotive::evs::BufferDesc& tgtBuff,
                      uint8_t* tgt, void* imgData, unsigned imgStride);

void fillYUYVFromYUYV(const ::aidl::android::hardware::automotive::evs::BufferDesc& tgtBuff,
                      uint8_t* tgt, void* imgData, unsigned imgStride);

void fillYUYVFromUYVY(const ::aidl::android::hardware::automotive::evs::BufferDesc& tgtBuff,
                      uint8_t* tgt, void* imgData, unsigned imgStride);

}  // namespace aidl::android::hardware::automotive::evs::implementation

#endif  // CPP_EVS_SAMPLEDRIVER_AIDL_INCLUDE_BUFFERCOPY_H
