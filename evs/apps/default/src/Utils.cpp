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

#include "Utils.h"

#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>

namespace {

using aidl::android::hardware::automotive::evs::BufferDesc;
using aidl::android::hardware::common::NativeHandle;
using aidl::android::hardware::graphics::common::HardwareBuffer;

NativeHandle dupNativeHandle(const NativeHandle& handle) {
    NativeHandle dup;

    dup.fds = std::vector<::ndk::ScopedFileDescriptor>(handle.fds.size());
    const size_t n = handle.fds.size();
    for (size_t i = 0; i < n; ++i) {
        dup.fds[i] = std::move(handle.fds[i].dup());
    }
    dup.ints = handle.ints;

    return std::move(dup);
}

HardwareBuffer dupHardwareBuffer(const HardwareBuffer& buffer) {
    HardwareBuffer dup = {
            .description = buffer.description,
            .handle = dupNativeHandle(buffer.handle),
    };

    return std::move(dup);
}

}  // namespace

BufferDesc dupBufferDesc(const BufferDesc& src) {
    BufferDesc dup = {
            .buffer = dupHardwareBuffer(src.buffer),
            .pixelSizeBytes = src.pixelSizeBytes,
            .bufferId = src.bufferId,
            .deviceId = src.deviceId,
            .timestamp = src.timestamp,
            .metadata = src.metadata,
    };

    return std::move(dup);
}

native_handle_t* getNativeHandle(const BufferDesc& buffer) {
    native_handle_t* nativeHandle = android::makeFromAidl(buffer.buffer.handle);
    if (nativeHandle == nullptr ||
        !std::all_of(nativeHandle->data + 0, nativeHandle->data + nativeHandle->numFds,
                     [](int fd) { return fd >= 0; })) {
        LOG(ERROR) << "Buffer " << buffer.bufferId << " contains an invalid native handle.";
        free(nativeHandle);
        return nullptr;
    }

    return nativeHandle;
}
