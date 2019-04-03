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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_MEMORY_INFO_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_MEMORY_INFO_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

#include "base_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class GlesMemoryInfo
{
public:
    GlesMemoryInfo(uint8_t* us, size_t le) :
                ssbo(0), texture(0), userptr(us), length(le), inUsing(true), refCount(1), needSync(false)
                { UNUSED(texture); }
    ~GlesMemoryInfo() {}
    GLuint getSSbo();
    bool sync(std::string name);
    void clean();
    void setNeedSync() { needSync = true; }
    void setNotInUsing();
    void incRef() { refCount++; }
    void resetRef() { refCount = 0;}
    void shareFrom(GlesMemoryInfo* from) { ssbo = from->ssbo; from->incRef();}
private:
    GLuint ssbo;
    GLuint texture;
    uint8_t* userptr;
    size_t length;
    bool inUsing;
    uint32_t refCount;
    bool needSync;
    friend class GlesMemoryManager;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
