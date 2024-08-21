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

#include "RenderDirectView.h"

#include "VideoTex.h"
#include "glError.h"
#include "shader.h"
#include "shader_simpleTex.h"

#include <aidl/android/hardware/automotive/evs/CameraDesc.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <aidl/android/hardware/automotive/evs/Stream.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <android-base/logging.h>
#include <math/mat4.h>
#include <system/camera_metadata.h>

namespace {

using aidl::android::hardware::automotive::evs::BufferDesc;
using aidl::android::hardware::automotive::evs::CameraDesc;
using aidl::android::hardware::automotive::evs::IEvsEnumerator;
using aidl::android::hardware::automotive::evs::Stream;

typedef struct {
    int32_t id;
    int32_t width;
    int32_t height;
    int32_t format;
    int32_t direction;
    int32_t framerate;
} RawStreamConfig;

const size_t kStreamCfgSz = sizeof(RawStreamConfig) / sizeof(int32_t);

}  // namespace

RenderDirectView::RenderDirectView(std::shared_ptr<IEvsEnumerator> enumerator,
                                   const CameraDesc& camDesc, const ConfigManager& config) :
      mEnumerator(enumerator), mCameraDesc(camDesc), mConfig(config) {
    // Find and store the target camera configuration
    const auto& camList = mConfig.getCameras();
    const auto target = std::find_if(camList.begin(), camList.end(),
                                     [this](const ConfigManager::CameraInfo& info) {
                                         return info.cameraId == mCameraDesc.id;
                                     });
    if (target != camList.end()) {
        // Store the info
        mCameraInfo = *target;

        // Calculate a rotation matrix
        float sinRoll, cosRoll;
        sincosf(mCameraInfo.roll, &sinRoll, &cosRoll);
        mRotationMat = {cosRoll, -sinRoll, sinRoll, cosRoll};
    }
}

bool RenderDirectView::activate() {
    // Ensure GL is ready to go...
    if (!prepareGL()) {
        LOG(ERROR) << "Error initializing GL";
        return false;
    }

    // Load our shader program if we don't have it already
    if (!mShaderProgram) {
        mShaderProgram = buildShaderProgram(vtxShader_simpleTexture, pixShader_simpleTexture,
                                            "simpleTexture");
        if (!mShaderProgram) {
            LOG(ERROR) << "Error building shader program";
            return false;
        }
    }

    bool foundCfg = false;
    std::unique_ptr<Stream> targetCfg(new Stream());

    if (!foundCfg) {
        // This logic picks the first configuration in the list among them that
        // support RGBA8888 format and its frame rate is faster than minReqFps.
        const int32_t minReqFps = 15;
        int32_t maxArea = 0;
        camera_metadata_entry_t streamCfgs;
        if (!find_camera_metadata_entry(reinterpret_cast<camera_metadata_t*>(
                                                mCameraDesc.metadata.data()),
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                        &streamCfgs)) {
            // Stream configurations are found in metadata
            RawStreamConfig* ptr = reinterpret_cast<RawStreamConfig*>(streamCfgs.data.i32);
            for (unsigned idx = 0; idx < streamCfgs.count; idx += kStreamCfgSz) {
                if (ptr->direction == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT &&
                    ptr->format == HAL_PIXEL_FORMAT_RGBA_8888) {
                    if (ptr->framerate >= minReqFps && ptr->width * ptr->height > maxArea) {
                        targetCfg->id = ptr->id;
                        targetCfg->width = ptr->width;
                        targetCfg->height = ptr->height;

                        maxArea = ptr->width * ptr->height;

                        foundCfg = true;
                    }
                }
                ++ptr;
            }
        } else {
            LOG(WARNING) << "No stream configuration data is found; "
                         << "default parameters will be used.";
        }
    }
    // This client always wants below input data format
    targetCfg->format = aidl::android::hardware::graphics::common::PixelFormat::RGBA_8888;
    foundCfg = true;

    targetCfg->width = 1920;
    targetCfg->height = 1080;
    // Construct our video texture
    mTexture.reset(createVideoTexture(mEnumerator, mCameraDesc.id.c_str(),
                                      foundCfg ? std::move(targetCfg) : nullptr, sDisplay,
                                      mConfig.getUseExternalMemory(),
                                      mConfig.getExternalMemoryFormat()));
    if (!mTexture) {
        LOG(ERROR) << "Failed to set up video texture for " << mCameraDesc.id;
        return false;
    }

    return true;
}

void RenderDirectView::deactivate() {
    // Release our video texture
    // We can't hold onto it because some other Render object might need the same camera
    mTexture = nullptr;
}

bool RenderDirectView::drawFrame(const BufferDesc& tgtBuffer) {
    // Tell GL to render to the given buffer
    if (!attachRenderTarget(tgtBuffer)) {
        LOG(ERROR) << "Failed to attached render target";
        return false;
    }

    // Select our screen space simple texture shader
    glUseProgram(mShaderProgram);

    // Set up the model to clip space transform (identity matrix if we're modeling in screen space)
    android::vec2 leftTop = {-0.5f, 0.5f};
    android::vec2 rightTop = {0.5f, 0.5f};
    android::vec2 leftBottom = {-0.5f, -0.5f};
    android::vec2 rightBottom = {0.5f, -0.5f};
    GLint loc = glGetUniformLocation(mShaderProgram, "cameraMat");
    if (loc < 0) {
        LOG(ERROR) << "Couldn't set shader parameter 'cameraMat'";
        return false;
    } else {
        const android::mat4 identityMatrix;
        glUniformMatrix4fv(loc, 1, false, identityMatrix.asArray());

        // Rotate the preview
        leftTop = mRotationMat * leftTop;
        leftBottom = mRotationMat * leftBottom;
        rightTop = mRotationMat * rightTop;
        rightBottom = mRotationMat * rightBottom;
    }

    // Bind the texture and assign it to the shader's sampler
    mTexture->refresh();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture->glId());

    GLint sampler = glGetUniformLocation(mShaderProgram, "tex");
    if (sampler < 0) {
        LOG(ERROR) << "Couldn't set shader parameter 'tex'";
        return false;
    } else {
        // Tell the sampler we looked up from the shader to use texture slot 0 as its source
        glUniform1i(sampler, 0);
    }

    // We want our image to show up opaque regardless of alpha values
    glDisable(GL_BLEND);

    // Draw a rectangle on the screen
    GLfloat vertsCarPos[] = {
            -1.0, 1.0,  0.0f,  // left top in window space
            1.0,  1.0,  0.0f,  // right top
            -1.0, -1.0, 0.0f,  // left bottom
            1.0,  -1.0, 0.0f   // right bottom
    };

    // Flip the preview if needed
    if (mCameraInfo.hflip) {
        std::swap(leftTop.x, rightTop.x);
        std::swap(leftBottom.x, rightBottom.x);
    }

    if (mCameraInfo.vflip) {
        std::swap(leftTop.y, leftBottom.y);
        std::swap(rightTop.y, rightBottom.y);
    }

    GLfloat vertsCarTex[] = {leftTop.x + 0.5f,     leftTop.y + 0.5f,    rightTop.x + 0.5f,
                             rightTop.y + 0.5f,    leftBottom.x + 0.5f, leftBottom.y + 0.5f,
                             rightBottom.x + 0.5f, rightBottom.y + 0.5f};
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertsCarPos);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vertsCarTex);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    // Now that everything is submitted, release our hold on the texture resource
    detachRenderTarget();

    // Wait for the rendering to finish
    glFinish();
    detachRenderTarget();
    return true;
}
