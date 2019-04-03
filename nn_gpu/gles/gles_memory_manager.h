/*
 * Copyright @2017 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Guo Yejun <yejun.guo@intel.com>
 */

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_MEMORY_MANAGER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_MEMORY_MANAGER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

#include "base_executor.h"
#include "gles_pool_info.h"
#include "gles_memory_info.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class GlesMemoryManager
{
public:
    GlesMemoryManager() {}
    ~GlesMemoryManager() {}

    bool initFromModel(const Model& model);
    bool resetFromRequest(const Request& request);
    bool sync();
    void clean();

    GlesPoolInfo* getModelPoolInfo(size_t index)
    {
        ASSERT(index < modelPoolInfos.size());
        return &modelPoolInfos[index];
    }
    GlesMemoryInfo* createModelMemoryInfo(uint8_t* userptr, size_t length);

    GlesPoolInfo* getRequestPoolInfo(size_t index)
    {
        ASSERT(index < requestPoolInfos.size());
        return &requestPoolInfos[index];
    }
    GlesMemoryInfo* createRequestMemoryInfo(uint8_t* userptr, size_t length);

    GlesMemoryInfo* createIntermediumMemoryInfo(size_t length);
    GlesMemoryInfo* createIntermediumMemoryInfo(uint8_t* userptr, size_t length);

private:
    std::vector<GlesPoolInfo> modelPoolInfos;
    std::vector<GlesMemoryInfo> modelMemInfos;

    std::vector<GlesPoolInfo> requestPoolInfos;
    std::vector<GlesMemoryInfo> requestMemInfos;

    std::vector<GlesMemoryInfo> intermediumMemInfos;

    void cleanPoolInfos(std::vector<GlesPoolInfo>& poolInfos) const;
    GlesMemoryInfo* createMemoryInfo(std::vector<GlesMemoryInfo>& memInfos, uint8_t* userptr, size_t length) const;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
