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
#ifndef VIDEOTEX_H
#define VIDEOTEX_H

#include "StreamHandler.h"
#include "TexWrapper.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/IEvsCamera.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <aidl/android/hardware/automotive/evs/Stream.h>

#include <system/graphics-base.h>

class VideoTex final : public TexWrapper {
    friend VideoTex* createVideoTexture(
            const std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator>& pEnum,
            const char* evsCameraId,
            std::unique_ptr<aidl::android::hardware::automotive::evs::Stream> streamCfg,
            EGLDisplay glDisplay, bool useExternalMemory, android_pixel_format_t format);

public:
    VideoTex() = delete;
    virtual ~VideoTex();

    bool refresh();  // returns true if the texture contents were updated

private:
    VideoTex(std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> pEnum,
             std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsCamera> pCamera,
             std::shared_ptr<StreamHandler> pStreamHandler, EGLDisplay glDisplay);

    std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> mEnumerator;
    std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsCamera> mCamera;
    std::shared_ptr<StreamHandler> mStreamHandler;
    aidl::android::hardware::automotive::evs::BufferDesc mImageBuffer;

    EGLDisplay mDisplay;
    EGLImageKHR mKHRimage = EGL_NO_IMAGE_KHR;
};

// Creates a video texture to draw the camera preview.  format is effective only
// when useExternalMemory is true.
VideoTex* createVideoTexture(
        const std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator>& pEnum,
        const char* deviceName,
        std::unique_ptr<aidl::android::hardware::automotive::evs::Stream> streamCfg,
        EGLDisplay glDisplay, bool useExternalMemory = false,
        android_pixel_format_t format = HAL_PIXEL_FORMAT_RGBA_8888);

#endif  // VIDEOTEX_H
