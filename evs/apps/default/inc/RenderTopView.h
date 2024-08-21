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

#ifndef CAR_EVS_APP_RENDERTOPVIEW_H
#define CAR_EVS_APP_RENDERTOPVIEW_H

#include "ConfigManager.h"
#include "RenderBase.h"
#include "VideoTex.h"

#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <math/mat4.h>

/*
 * Combines the views from all available cameras into one reprojected top down view.
 */
class RenderTopView final : public RenderBase {
public:
    RenderTopView(
            std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> enumerator,
            const std::vector<ConfigManager::CameraInfo>& camList, const ConfigManager& config);

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual bool drawFrame(const aidl::android::hardware::automotive::evs::BufferDesc& tgtBuffer);

protected:
    struct ActiveCamera {
        const ConfigManager::CameraInfo& info;
        std::unique_ptr<VideoTex> tex;

        ActiveCamera(const ConfigManager::CameraInfo& c) : info(c){};
    };

    void renderCarTopView();
    void renderCameraOntoGroundPlane(const ActiveCamera& cam);

    std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> mEnumerator;
    const ConfigManager& mConfig;
    std::vector<ActiveCamera> mActiveCameras;

    struct {
        std::unique_ptr<TexWrapper> checkerBoard;
        std::unique_ptr<TexWrapper> carTopView;
    } mTexAssets;

    struct {
        GLuint simpleTexture;
        GLuint projectedTexture;
    } mPgmAssets;

    android::mat4 orthoMatrix;
};

#endif  // CAR_EVS_APP_RENDERTOPVIEW_H
