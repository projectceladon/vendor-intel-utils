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

#include "StreamHandler.h"

#include "Utils.h"

#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/EvsEventDesc.h>
#include <aidl/android/hardware/automotive/evs/EvsEventType.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android-base/logging.h>
#include <cutils/native_handle.h>
#include <ui/GraphicBufferAllocator.h>

#include <stdio.h>
#include <string.h>

namespace {

using aidl::android::hardware::automotive::evs::BufferDesc;
using aidl::android::hardware::automotive::evs::EvsEventDesc;
using aidl::android::hardware::automotive::evs::EvsEventType;
using aidl::android::hardware::automotive::evs::IEvsCamera;

}  // namespace

StreamHandler::StreamHandler(const std::shared_ptr<IEvsCamera>& pCamera, uint32_t numBuffers,
                             bool useOwnBuffers, android_pixel_format_t format, int32_t width,
                             int32_t height) :
      mCamera(pCamera), mUseOwnBuffers(useOwnBuffers) {
    if (!useOwnBuffers) {
        // We rely on the camera having at least two buffers available since we'll hold one and
        // expect the camera to be able to capture a new image in the background.
        pCamera->setMaxFramesInFlight(numBuffers);
    } else {
        mOwnBuffers.resize(numBuffers);

        // Acquire the graphics buffer allocator
        buffer_handle_t memHandle = nullptr;
        android::GraphicBufferAllocator& alloc(android::GraphicBufferAllocator::get());
        const auto usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY |
                GRALLOC_USAGE_SW_WRITE_OFTEN;
        for (size_t i = 0; i < numBuffers; ++i) {
            unsigned pixelsPerLine;
            android::status_t result = alloc.allocate(width, height, format, 1, usage, &memHandle,
                                                      &pixelsPerLine, 0, "EvsApp");
            if (result != android::NO_ERROR) {
                LOG(ERROR) << __FUNCTION__ << " failed to allocate memory.";
            } else {
                BufferDesc buf;
                AHardwareBuffer_Desc* pDesc =
                        reinterpret_cast<AHardwareBuffer_Desc*>(&buf.buffer.description);
                pDesc->width = width;
                pDesc->height = height;
                pDesc->layers = 1;
                pDesc->format = format;
                pDesc->usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_READ_RARELY |
                        GRALLOC_USAGE_SW_WRITE_OFTEN;
                pDesc->stride = pixelsPerLine;
                buf.buffer.handle = android::dupToAidl(memHandle);
                buf.bufferId = i;  // Unique number to identify this buffer
                mOwnBuffers[i] = std::move(buf);
            }
        }

        int delta = 0;
        if (auto status = pCamera->importExternalBuffers(mOwnBuffers, &delta); !status.isOk()) {
            PLOG(ERROR) << "Failed to import buffers.";
            return;
        }

        LOG(INFO) << delta << " buffers are imported by EVS.";
    }
}

void StreamHandler::shutdown() {
    // Make sure we're not still streaming
    blockingStopStream();

    // At this point, the receiver thread is no longer running, so we can safely drop
    // our remote object references so they can be freed
    mCamera = nullptr;

    if (!mUseOwnBuffers) {
        return;
    }

    android::GraphicBufferAllocator& alloc(android::GraphicBufferAllocator::get());
    for (auto& b : mOwnBuffers) {
        alloc.free(android::makeFromAidl(b.buffer.handle));
    }

    mOwnBuffers.resize(0);
}

bool StreamHandler::startStream() {
    std::lock_guard lock(mLock);
    if (mRunning) {
        return true;
    }

    // Tell the camera to start streaming
    if (auto status = mCamera->startVideoStream(ref<StreamHandler>()); !status.isOk()) {
        LOG(ERROR) << "Failed to request a video stream.";
        return false;
    }

    // Mark ourselves as running
    mRunning = true;
    return true;
}

void StreamHandler::asyncStopStream() {
    // Tell the camera to stop streaming.
    // This will result in a null frame being delivered when the stream actually stops.
    if (auto status = mCamera->stopVideoStream(); !status.isOk()) {
        LOG(WARNING) << "Failed to stop a video stream.";
    }
}

void StreamHandler::blockingStopStream() {
    // Tell the stream to stop
    asyncStopStream();

    // Wait until the stream has actually stopped
    std::unique_lock lock(mLock);
    if (mRunning) {
        mSignal.wait(lock, [this]() { return !mRunning; });
    }
}

bool StreamHandler::isRunning() {
    std::lock_guard lock(mLock);
    return mRunning;
}

bool StreamHandler::newFrameAvailable() {
    std::lock_guard lock(mLock);
    return (mReadyBuffer >= 0);
}

const BufferDesc& StreamHandler::getNewFrame() {
    std::lock_guard lock(mLock);

    if (mHeldBuffer >= 0) {
        LOG(ERROR) << "Ignored call for new frame while still holding the old one.";
    } else {
        if (mReadyBuffer < 0) {
            LOG(ERROR) << "Returning invalid buffer because we don't have any.  "
                       << "Call newFrameAvailable first?";
            mReadyBuffer = 0;  // This is a lie!
        }

        // Move the ready buffer into the held position, and clear the ready position
        mHeldBuffer = mReadyBuffer;
        mReadyBuffer = -1;
    }

    return mBuffers[mHeldBuffer];
}

void StreamHandler::doneWithFrame(const BufferDesc& bufDesc) {
    std::lock_guard lock(mLock);

    // We better be getting back the buffer we original delivered!
    if (mHeldBuffer < 0) {
        // Safely ignores a call with an invalid buffer reference.
        return;
    }

    if (bufDesc.bufferId != mBuffers[mHeldBuffer].bufferId) {
        LOG(WARNING) << __FUNCTION__ << " ignores an unexpected buffer; expected = "
                     << mBuffers[mHeldBuffer].bufferId << ", received = " << bufDesc.bufferId;
        return;
    }

    // Send the buffer back to the underlying camera
    std::vector<BufferDesc> frames(1);
    frames[0] = std::move(mBuffers[mHeldBuffer]);
    if (auto status = mCamera->doneWithFrame(frames); !status.isOk()) {
        LOG(WARNING) << __FUNCTION__ << " fails to return a buffer";
    }

    // Clear the held position
    mHeldBuffer = -1;
}

ndk::ScopedAStatus StreamHandler::deliverFrame(const std::vector<BufferDesc>& buffers) {
    LOG(DEBUG) << "Received frames from the camera";

    // Take the lock to protect our frame slots and running state variable
    std::unique_lock lock(mLock);
    const BufferDesc& bufferToUse = buffers[0];

    // Do we already have a "ready" frame?
    if (mReadyBuffer >= 0) {
        // Send the previously saved buffer back to the camera unused
        std::vector<BufferDesc> frames(1);
        frames[0] = std::move(mBuffers[mReadyBuffer]);
        if (auto status = mCamera->doneWithFrame(frames); !status.isOk()) {
            LOG(WARNING) << __FUNCTION__ << " fails to return a buffer";
        }

        // We'll reuse the same ready buffer index
    } else if (mHeldBuffer >= 0) {
        // The client is holding a buffer, so use the other slot for "on deck"
        mReadyBuffer = 1 - mHeldBuffer;
    } else {
        // This is our first buffer, so just pick a slot
        mReadyBuffer = 0;
    }

    // Save this frame until our client is interested in it
    mBuffers[mReadyBuffer] = dupBufferDesc(bufferToUse);

    // Notify anybody who cares that things have changed
    mSignal.notify_all();
    lock.unlock();

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamHandler::notify(const EvsEventDesc& event) {
    switch (event.aType) {
        case EvsEventType::STREAM_STOPPED: {
            {
                std::lock_guard<std::mutex> lock(mLock);

                // Signal that the last frame has been received and the stream is stopped
                mRunning = false;
            }
            LOG(INFO) << "Received a STREAM_STOPPED event";
            break;
        }

        case EvsEventType::PARAMETER_CHANGED:
            LOG(INFO) << "Camera parameter " << std::hex << event.payload[0] << " is set to "
                      << event.payload[1];
            break;

        // Below events are ignored in reference implementation.
        case EvsEventType::STREAM_STARTED:
            [[fallthrough]];
        case EvsEventType::FRAME_DROPPED:
            [[fallthrough]];
        case EvsEventType::TIMEOUT:
            LOG(INFO) << "Event " << std::hex << static_cast<unsigned>(event.aType)
                      << "is received but ignored.";
            break;
        default:
            LOG(ERROR) << "Unknown event id: " << static_cast<unsigned>(event.aType);
            break;
    }

    return ndk::ScopedAStatus::ok();
}
