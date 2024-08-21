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

#ifndef CAR_EVS_APP_RENDERDIRECTVIEW_H
#define CAR_EVS_APP_RENDERDIRECTVIEW_H

#include "ConfigManager.h"
#include "RenderBase.h"
#include "VideoTex.h"

#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/CameraDesc.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <math/mat2.h>

/*
 * Renders the view from a single specified camera directly to the full display.
 */
class RenderDirectView final : public RenderBase {
public:
    RenderDirectView(
            std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> enumerator,
            const aidl::android::hardware::automotive::evs::CameraDesc& camDesc,
            const ConfigManager& config);

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual bool drawFrame(const aidl::android::hardware::automotive::evs::BufferDesc& tgtBuffer);

protected:
    std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> mEnumerator;
    ConfigManager::CameraInfo mCameraInfo;
    aidl::android::hardware::automotive::evs::CameraDesc mCameraDesc;
    const ConfigManager& mConfig;

    std::unique_ptr<VideoTex> mTexture;

    GLuint mShaderProgram = 0;

    android::mat2 mRotationMat;
};

#endif  // CAR_EVS_APP_RENDERDIRECTVIEW_H
