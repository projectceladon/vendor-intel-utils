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

#ifndef CAR_EVS_APP_RENDERPIXELCOPY_H
#define CAR_EVS_APP_RENDERPIXELCOPY_H

#include "ConfigManager.h"
#include "RenderBase.h"
#include "VideoTex.h"

#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>

/*
 * Renders the view from a single specified camera directly to the full display.
 */
class RenderPixelCopy final : public RenderBase {
public:
    RenderPixelCopy(
            std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> enumerator,
            const ConfigManager::CameraInfo& cam);

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual bool drawFrame(const aidl::android::hardware::automotive::evs::BufferDesc& tgtBuffer);

protected:
    std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> mEnumerator;
    ConfigManager::CameraInfo mCameraInfo;

    std::shared_ptr<StreamHandler> mStreamHandler;
};

#endif  // CAR_EVS_APP_RENDERPIXELCOPY_H
