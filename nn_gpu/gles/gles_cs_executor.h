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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_CS_EXECUTOR_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_CS_EXECUTOR_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

#include "gpu_executor.h"
#include "gles_operand.h"
#include "gles_memory_manager.h"
#include "gles_cs_program_manager.h"
#include "gles_cpu_timer.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

#define CHECKGLERROR()                                      \
    do {                                                    \
        GLenum err = glGetError();                          \
        if (err != GL_NO_ERROR)                             \
        {                                                   \
            ALOGE("glGetError returns error 0x%x at %s:%d", err, __FILE__, __LINE__); \
        }                                                   \
    } while(0);

#define CHECK_GL_STATE_RET() \
{\
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) \
    {\
        ALOGE("glGetError returns %d, func:%s, line: %d\n", err, __func__, __LINE__); \
        return false; \
    }\
    return true;\
}

struct GlesOperationResource
{
    GlesOperationResource(){}
    std::vector<GLuint> tmpBo;
};

class GlesCsExecutor : public GpuExecutor
{
public:
    static bool initPerProcess();
    static void deinitPerProcess();
    static void getCapabilities(Capabilities& cap);
    static std::vector<bool> getSupportedOperations(const Model& model);
    static bool checkGroupParam(int* localSize, int* groupCount);

    GlesCsExecutor(const Model& model);
    ~GlesCsExecutor() override;

    bool initPerModel() override;
    bool initPerExecThread() override;
    bool run(const Request& request) override;
    void deinitPerExecThread() override;
    void deinitPerModel() override;
    static GLint max_wg_count_x;
    static GLint max_wg_count_y;
    static GLint max_wg_count_z;
    static GLint max_wg_size_x;
    static GLint max_wg_size_y;
    static GLint max_wg_size_z;
    static GLint max_wg_invocations;

private:
    static EGLDisplay dpy;
    static EGLConfig cfg;
    EGLContext _ctx;
    //cannot be a global memMgr per process since the gl objects belong to one context (_ctx)
    GlesMemoryManager memMgr;
    GlesCsProgramManager progMgr;
    std::vector<GlesOperand> operands;
    std::vector<OperationCpuTimer> operationTimers;
    std::vector<GlesOperationResource> operationResources;

    void showEglError()
    {
        EGLint err = eglGetError();
        ALOGE("eglGetError returns 0x%x", err);
    }

    void initOperands();
    void restoreOperands();
    void setArgOperands(const Request& request);

    void initOperationTimers();
    void showOperationTimers();
    void initOperationResources();
    void deinitOperationResources();

    void bindOperand(GlesOperand& operand, GLuint boindex);
    void setTotal(GLuint prog, uint32_t totalX);
    void setTotal(GLuint prog, uint32_t totalX, uint32_t totalY);
    void setTotal(GLuint prog, uint32_t totalX, uint32_t totalY, uint32_t totalZ);
    void setUniform1ui(GLuint prog, const char* name, GLuint v);
    void setUniform1f(GLuint prog, const char* name, GLfloat f);

    bool run(const Operation& operation, OperationCpuTimer* timer, GlesOperationResource& resource);

#define SETUP_OP(op) bool do##op(const Operation& operation, GlesOperationResource& resource);
#include "setup_op.hxx"
#undef SETUP_OP
};

enum PaddingScheme
{
    kPaddingUnknown = 0,
    kPaddingSame = 1,
    kPaddingValid = 2,
};

inline void calculateExplicitPadding(int32_t in_size, int32_t stride,
                                     int32_t filter_size, int32_t padding_implicit,
                                     int32_t* padding_head, int32_t* padding_tail) {
    *padding_head = 0;
    *padding_tail = 0;

    if (padding_implicit == kPaddingSame) {
        int32_t out_size = (in_size + stride - 1) / stride;
        int32_t tmp = (out_size - 1) * stride + filter_size;
        if (tmp > in_size) {
            *padding_head = (tmp - in_size) / 2;
            *padding_tail = (tmp - in_size) - *padding_head;
        }
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
