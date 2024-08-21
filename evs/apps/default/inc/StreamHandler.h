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

#ifndef EVS_VTS_STREAMHANDLER_H
#define EVS_VTS_STREAMHANDLER_H

#include <aidl/android/hardware/automotive/evs/BnEvsCameraStream.h>
#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/DisplayState.h>
#include <aidl/android/hardware/automotive/evs/EvsEventDesc.h>
#include <aidl/android/hardware/automotive/evs/IEvsCamera.h>
#include <aidl/android/hardware/graphics/common/HardwareBuffer.h>
#include <ui/GraphicBuffer.h>

#include <queue>

/*
 * StreamHandler:
 * This class can be used to receive camera imagery from an IEvsCamera implementation.  It will
 * hold onto the most recent image buffer, returning older ones.
 * Note that the video frames are delivered on a background thread, while the control interface
 * is actuated from the applications foreground thread.
 */
class StreamHandler final : public aidl::android::hardware::automotive::evs::BnEvsCameraStream {
public:
    virtual ~StreamHandler() { shutdown(); };

    StreamHandler(
            const std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsCamera>& cameraObj,
            uint32_t numBuffers = 2, bool useOwnBuffers = false,
            android_pixel_format_t format = HAL_PIXEL_FORMAT_RGBA_8888, int32_t width = 640,
            int32_t height = 360);
    void shutdown();

    bool startStream();
    void asyncStopStream();
    void blockingStopStream();

    bool isRunning();

    bool newFrameAvailable();
    const aidl::android::hardware::automotive::evs::BufferDesc& getNewFrame();
    void doneWithFrame(const aidl::android::hardware::automotive::evs::BufferDesc& buffer);

private:
    // Implementation for aidl::android::hardware::automotive::evs::IEvsCameraStream
    ndk::ScopedAStatus deliverFrame(
            const std::vector<aidl::android::hardware::automotive::evs::BufferDesc>& buffers)
            override;
    ndk::ScopedAStatus notify(
            const aidl::android::hardware::automotive::evs::EvsEventDesc& event) override;

    // Values initialized as startup
    std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsCamera> mCamera;

    // Since we get frames delivered to us asnchronously via the IEvsCameraStream interface,
    // we need to protect all member variables that may be modified while we're streaming
    // (i.e.: those below)
    std::mutex mLock;
    std::condition_variable mSignal;

    bool mRunning = false;

    aidl::android::hardware::automotive::evs::BufferDesc mBuffers[2];
    int mHeldBuffer = -1;   // Index of the one currently held by the client
    int mReadyBuffer = -1;  // Index of the newest available buffer
    std::vector<aidl::android::hardware::automotive::evs::BufferDesc> mOwnBuffers;
    bool mUseOwnBuffers;
};

#endif  // EVS_VTS_STREAMHANDLER_H
