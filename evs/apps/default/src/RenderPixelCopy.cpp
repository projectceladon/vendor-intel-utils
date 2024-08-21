/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "RenderPixelCopy.h"

#include "FormatConvert.h"
#include "Utils.h"

#include <aidl/android/hardware/automotive/evs/IEvsCamera.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <aidl/android/hardware/automotive/evs/Stream.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <android-base/scopeguard.h>
#include <android/binder_manager.h>

namespace {

using aidl::android::hardware::automotive::evs::BufferDesc;
using aidl::android::hardware::automotive::evs::IEvsCamera;
using aidl::android::hardware::automotive::evs::IEvsEnumerator;
using aidl::android::hardware::automotive::evs::Stream;

}  // namespace

RenderPixelCopy::RenderPixelCopy(std::shared_ptr<IEvsEnumerator> enumerator,
                                 const ConfigManager::CameraInfo& cam) {
    mEnumerator = enumerator;
    mCameraInfo = cam;
}

bool RenderPixelCopy::activate() {
    // Set up the camera to feed this texture
    Stream emptyConfig;
    std::shared_ptr<IEvsCamera> pCamera;
    if (auto status = mEnumerator->openCamera(mCameraInfo.cameraId.c_str(), emptyConfig, &pCamera);
        !status.isOk()) {
        LOG(ERROR) << "Failed to allocate new EVS Camera interface";
        return false;
    }

    // Initialize the stream that will help us update this texture's contents
    std::shared_ptr<StreamHandler> pStreamHandler =
            ndk::SharedRefBase::make<StreamHandler>(pCamera);
    if (!pStreamHandler) {
        LOG(ERROR) << "Failed to allocate FrameHandler";
        return false;
    }

    // Start the video stream
    if (!pStreamHandler->startStream()) {
        LOG(ERROR) << "Start stream failed";
        return false;
    }

    mStreamHandler = pStreamHandler;
    return true;
}

void RenderPixelCopy::deactivate() {
    mStreamHandler.reset();
}

bool RenderPixelCopy::drawFrame(const BufferDesc& tgtBuffer) {
    bool success = true;
    native_handle_t* targetBufferNativeHandle = getNativeHandle(tgtBuffer);
    if (targetBufferNativeHandle == nullptr) {
        LOG(ERROR) << "Target buffer has an invalid native handle.";
        return false;
    }

    const auto handleGuard = android::base::make_scope_guard(
            [targetBufferNativeHandle] { free(targetBufferNativeHandle); });
    const AHardwareBuffer_Desc* pTgtDesc =
            reinterpret_cast<const AHardwareBuffer_Desc*>(&tgtBuffer.buffer.description);
    android::sp<android::GraphicBuffer> tgt =
            new android::GraphicBuffer(targetBufferNativeHandle,
                                       android::GraphicBuffer::CLONE_HANDLE, pTgtDesc->width,
                                       pTgtDesc->height, pTgtDesc->format, pTgtDesc->layers,
                                       pTgtDesc->usage, pTgtDesc->stride);

    // Lock our target buffer for writing (should be RGBA8888 format)
    uint32_t* tgtPixels = nullptr;
    tgt->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)&tgtPixels);

    if (tgtPixels) {
        if (pTgtDesc->format != HAL_PIXEL_FORMAT_RGBA_8888) {
            // We always expect 32 bit RGB for the display output for now.  Is there a need for 565?
            LOG(ERROR) << "Diplay buffer is always expected to be 32bit RGBA";
            success = false;
        } else {
            // Make sure we have the latest frame data
            if (mStreamHandler->newFrameAvailable()) {
                const BufferDesc& srcBuffer = mStreamHandler->getNewFrame();
                native_handle_t* srcBufferNativeHandle = getNativeHandle(srcBuffer);
                if (srcBufferNativeHandle == nullptr) {
                    LOG(ERROR) << "Target buffer has an invalid native handle.";
                    return false;
                }

                const auto handleGuard = android::base::make_scope_guard(
                        [srcBufferNativeHandle] { free(srcBufferNativeHandle); });
                const AHardwareBuffer_Desc* pSrcDesc =
                        reinterpret_cast<const AHardwareBuffer_Desc*>(
                                &srcBuffer.buffer.description);

                // Lock our source buffer for reading (current expectation are for this to be NV21
                // format)
                android::sp<android::GraphicBuffer> src =
                        new android::GraphicBuffer(srcBufferNativeHandle,
                                                   android::GraphicBuffer::CLONE_HANDLE,
                                                   pSrcDesc->width, pSrcDesc->height,
                                                   pSrcDesc->format, pSrcDesc->layers,
                                                   pSrcDesc->usage, pSrcDesc->stride);

                unsigned char* srcPixels = nullptr;
                src->lock(GRALLOC_USAGE_SW_READ_OFTEN, (void**)&srcPixels);
                if (srcPixels != nullptr) {
                    // Make sure we don't run off the end of either buffer
                    const unsigned width = std::min(pTgtDesc->width, pSrcDesc->width);
                    const unsigned height = std::min(pTgtDesc->height, pSrcDesc->height);

                    if (pSrcDesc->format == HAL_PIXEL_FORMAT_YCRCB_420_SP) {  // 420SP == NV21
                        copyNV21toRGB32(width, height, srcPixels, tgtPixels, pTgtDesc->stride);
                    } else if (pSrcDesc->format == HAL_PIXEL_FORMAT_YV12) {  // YUV_420P == YV12
                        copyYV12toRGB32(width, height, srcPixels, tgtPixels, pTgtDesc->stride);
                    } else if (pSrcDesc->format == HAL_PIXEL_FORMAT_YCBCR_422_I) {  // YUYV
                        copyYUYVtoRGB32(width, height, srcPixels, pSrcDesc->stride, tgtPixels,
                                        pTgtDesc->stride);
                    } else if (pSrcDesc->format == pTgtDesc->format) {  // 32bit RGBA
                        copyMatchedInterleavedFormats(width, height, srcPixels, pSrcDesc->stride,
                                                      tgtPixels, pTgtDesc->stride,
                                                      tgtBuffer.pixelSizeBytes);
                    }
                } else {
                    LOG(ERROR) << "Failed to get pointer into src image data";
                    success = false;
                }

                mStreamHandler->doneWithFrame(srcBuffer);
            }
        }
    } else {
        LOG(ERROR) << "Failed to lock buffer contents for contents transfer";
        success = false;
    }

    if (tgtPixels) {
        tgt->unlock();
    }

    return success;
}
