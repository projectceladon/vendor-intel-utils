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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GLES_CS_EXECUTOR_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GLES_CS_EXECUTOR_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

#include "gpu_executor.h"
#include "gles_operand.h"
#include "gles_memory_manager.h"
#include "gles_cs_program_manager.h"
#include "gles_cpu_timer.h"

NAME_SPACE_BEGIN

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
    static void getCapabilities(V1_0::Capabilities& cap);
    static std::vector<bool> getSupportedOperations(const Model& model);
    static bool checkGroupParam(int* localSize, int* groupCount);

    GlesCsExecutor(const Model& model);
    ~GlesCsExecutor() override;

    bool initPerModel() override;
    bool initPerExecThread() override;
    bool run(const Request& request) override;
    void deinitPerExecThread() override;
    void deinitPerModel() override;
    std::string getOpName(const Operation& operation);
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
        LOGE("eglGetError returns 0x%x", err);
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
#include "gles_setup_op.hxx"
#undef SETUP_OP
};

NAME_SPACE_STOP

#endif
