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

#ifndef CPP_EVS_SAMPLEDRIVER_AIDL_INCLUDE_EVSV4LCAMERA_H
#define CPP_EVS_SAMPLEDRIVER_AIDL_INCLUDE_EVSV4LCAMERA_H

#include "ConfigManager.h"
#include "VideoCapture.h"

#include <aidl/android/hardware/automotive/evs/BnEvsCamera.h>
#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraParam.h>
#include <aidl/android/hardware/automotive/evs/EvsResult.h>
#include <aidl/android/hardware/automotive/evs/IEvsCameraStream.h>
#include <aidl/android/hardware/automotive/evs/IEvsDisplay.h>
#include <aidl/android/hardware/automotive/evs/ParameterRange.h>
#include <aidl/android/hardware/automotive/evs/Stream.h>
#include <android-base/result.h>
#include <android/hardware_buffer.h>
#include <ui/GraphicBuffer.h>

#include <functional>
#include <thread>

namespace aidl::android::hardware::automotive::evs::implementation {

namespace aidlevs = ::aidl::android::hardware::automotive::evs;

class EvsV4lCamera : public ::aidl::android::hardware::automotive::evs::BnEvsCamera {
public:
    // Methods from ::android::hardware::automotive::aidlevs::IEvsCamera follow.
    ::ndk::ScopedAStatus doneWithFrame(const std::vector<aidlevs::BufferDesc>& buffers) override;
    ::ndk::ScopedAStatus forcePrimaryClient(
            const std::shared_ptr<aidlevs::IEvsDisplay>& display) override;
    ::ndk::ScopedAStatus getCameraInfo(aidlevs::CameraDesc* _aidl_return) override;
    ::ndk::ScopedAStatus getExtendedInfo(int32_t opaqueIdentifier,
                                         std::vector<uint8_t>* value) override;
    ::ndk::ScopedAStatus getIntParameter(aidlevs::CameraParam id,
                                         std::vector<int32_t>* value) override;
    ::ndk::ScopedAStatus getIntParameterRange(aidlevs::CameraParam id,
                                              aidlevs::ParameterRange* _aidl_return) override;
    ::ndk::ScopedAStatus getParameterList(std::vector<aidlevs::CameraParam>* _aidl_return) override;
    ::ndk::ScopedAStatus getPhysicalCameraInfo(const std::string& deviceId,
                                               aidlevs::CameraDesc* _aidl_return) override;
    ::ndk::ScopedAStatus importExternalBuffers(const std::vector<aidlevs::BufferDesc>& buffers,
                                               int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus pauseVideoStream() override;
    ::ndk::ScopedAStatus resumeVideoStream() override;
    ::ndk::ScopedAStatus setExtendedInfo(int32_t opaqueIdentifier,
                                         const std::vector<uint8_t>& opaqueValue) override;
    ::ndk::ScopedAStatus setIntParameter(aidlevs::CameraParam id, int32_t value,
                                         std::vector<int32_t>* effectiveValue) override;
    ::ndk::ScopedAStatus setPrimaryClient() override;
    ::ndk::ScopedAStatus setMaxFramesInFlight(int32_t bufferCount) override;
    ::ndk::ScopedAStatus startVideoStream(
            const std::shared_ptr<aidlevs::IEvsCameraStream>& receiver) override;
    ::ndk::ScopedAStatus stopVideoStream() override;
    ::ndk::ScopedAStatus unsetPrimaryClient() override;

    static std::shared_ptr<EvsV4lCamera> Create(const char* deviceName);
    static std::shared_ptr<EvsV4lCamera> Create(const char* deviceName,
                                                std::unique_ptr<ConfigManager::CameraInfo>& camInfo,
                                                const aidlevs::Stream* streamCfg = nullptr);
    EvsV4lCamera(const EvsV4lCamera&) = delete;
    EvsV4lCamera& operator=(const EvsV4lCamera&) = delete;

    virtual ~EvsV4lCamera() override;
    void shutdown();

    const aidlevs::CameraDesc& getDesc() { return mDescription; }

    // Dump captured frames to the filesystem
    ::android::base::Result<void> startDumpFrames(const std::string& path);
    ::android::base::Result<void> stopDumpFrames();

    // Constructors
    EvsV4lCamera(const char* deviceName, std::unique_ptr<ConfigManager::CameraInfo>& camInfo);

private:
    // These three functions are expected to be called while mAccessLock is held
    bool setAvailableFrames_Locked(unsigned bufferCount);
    unsigned increaseAvailableFrames_Locked(unsigned numToAdd);
    unsigned decreaseAvailableFrames_Locked(unsigned numToRemove);

    void forwardFrame(imageBuffer* tgt, void* data);
    inline bool convertToV4l2CID(aidlevs::CameraParam id, uint32_t& v4l2cid);

    // The callback used to deliver each frame
    std::shared_ptr<aidlevs::IEvsCameraStream> mStream;

    // Interface to the v4l device
    VideoCapture mVideo = {};

    // The properties of this camera
    aidlevs::CameraDesc mDescription = {};

    uint32_t mFormat = 0;  // Values from android_pixel_format_t
    uint32_t mUsage = 0;   // Values from from Gralloc.h
    uint32_t mStride = 0;  // Pixels per row (may be greater than image width)

    struct BufferRecord {
        buffer_handle_t handle;
        bool inUse;

        explicit BufferRecord(buffer_handle_t h) : handle(h), inUse(false){};
    };

    // Graphics buffers to transfer images
    std::vector<BufferRecord> mBuffers;
    // How many buffers are we currently using
    unsigned mFramesAllowed;
    // How many buffers are currently outstanding
    unsigned mFramesInUse;

    std::set<uint32_t> mCameraControls;  // Available camera controls

    // Which format specific function we need to use to move camera imagery into our output buffers
    void (*mFillBufferFromVideo)(const aidlevs::BufferDesc& tgtBuff, uint8_t* tgt, void* imgData,
                                 unsigned imgStride);

    aidlevs::EvsResult doneWithFrame_impl(const aidlevs::BufferDesc& bufferDesc);
    aidlevs::EvsResult doneWithFrame_impl(uint32_t id, buffer_handle_t handle);

    // Synchronization necessary to deconflict the capture thread from the main service thread
    // Note that the service interface remains single threaded (ie: not reentrant)
    mutable std::mutex mAccessLock;

    // Static camera module information
    std::unique_ptr<ConfigManager::CameraInfo>& mCameraInfo;

    // Extended information
    std::unordered_map<uint32_t, std::vector<uint8_t>> mExtInfo;

    // Dump captured frames
    std::atomic<bool> mDumpFrame = false;

    // Path to store captured frames
    std::string mDumpPath;

    // Frame counter
    uint64_t mFrameCounter = 0;
};

}  // namespace aidl::android::hardware::automotive::evs::implementation

#endif  // CPP_EVS_SAMPLEDRIVER_AIDL_INCLUDE_EVSV4LCAMERA_H
