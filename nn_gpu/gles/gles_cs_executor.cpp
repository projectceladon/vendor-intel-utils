#include "gles_cs_executor.h"
#include "gles_memory_manager.h"

NAME_SPACE_BEGIN

EGLDisplay GlesCsExecutor::dpy = EGL_NO_DISPLAY;
EGLConfig GlesCsExecutor::cfg = nullptr;
GLint GlesCsExecutor::max_wg_count_x = 0;
GLint GlesCsExecutor::max_wg_count_y = 0;
GLint GlesCsExecutor::max_wg_count_z = 0;
GLint GlesCsExecutor::max_wg_size_x = 0;
GLint GlesCsExecutor::max_wg_size_y = 0;
GLint GlesCsExecutor::max_wg_size_z = 0;
GLint GlesCsExecutor::max_wg_invocations = 0;

bool GlesCsExecutor::initPerProcess()
{
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY)
    {
        LOGE("eglGetDisplay returned EGL_NO_DISPLAY");
        return false;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    if (eglInitialize(dpy, &majorVersion, &minorVersion) != EGL_TRUE)
    {
        LOGE("eglInitialize failed");
        return false;
    }

    EGLint count;
    EGLint cfgAttribs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR, EGL_NONE };
    if (eglChooseConfig(dpy, cfgAttribs, &cfg, 1, &count) != EGL_TRUE)
    {
        LOGE("eglChooseConfig failed");
        return false;
    }

    //then, just have a try
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE)
    {
        LOGE("eglBindAPI failed");
        return false;
    }
    EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttribs);
    if (ctx == EGL_NO_CONTEXT)
    {
        LOGE("eglCreateContext failed");
        return false;
    }
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
    {
        LOGE("eglMakeCurrent failed to set");
        return false;
    }

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_wg_count_x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &max_wg_count_y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &max_wg_count_z);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_wg_size_x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_wg_size_y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_wg_size_z);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_wg_invocations);
    NN_GPU_DEBUG("%s: max_wg_count(%d,%d,%d), max_wg_size(%d,%d,%d), max_wg_invocation %d\n",
            __func__,
            max_wg_count_x, max_wg_count_y, max_wg_count_z,
            max_wg_size_x, max_wg_size_y, max_wg_size_z,
            max_wg_invocations);

    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        LOGE("eglMakeCurrent failed to clear");
        return false;
    }
    if (eglDestroyContext(dpy, ctx) != EGL_TRUE)
    {
        LOGE("eglDestoryContext failed");
        return false;
    }
    if (eglReleaseThread() != EGL_TRUE)
    {
        LOGE("eglReleaseThread failed");
        return false;
    }

    return true;
}

void GlesCsExecutor::deinitPerProcess()
{
    if (eglTerminate(dpy) != EGL_TRUE)
    {
        LOGE("eglTerminate failed");
    }
}

GlesCsExecutor::GlesCsExecutor(const Model& model) :
                        GpuExecutor(model),
                        _ctx(EGL_NO_CONTEXT)
{

}

GlesCsExecutor::~GlesCsExecutor()
{

}

bool GlesCsExecutor::initPerModel()
{
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE)
    {
        LOGE("eglBindAPI failed within initPerModel");
        showEglError();
        return false;
    }

    EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    _ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttribs);
    if (_ctx == EGL_NO_CONTEXT)
    {
        LOGE("eglCreateContext failed within initPerModel");
        showEglError();
        return false;
    }

    memMgr.initFromModel(model);
    initOperands();
    initOperationTimers();
    initOperationResources();

    return true;
}

void GlesCsExecutor::initOperationResources()
{
    operationResources.resize(model.operations.size());
}

void GlesCsExecutor::deinitOperationResources()
{
    for (size_t i = 0; i < operationResources.size(); ++i)
    {
        GlesOperationResource res = operationResources[i];
        for (size_t j = 0; j < res.tmpBo.size(); ++j)
        {
            glDeleteBuffers(1, &res.tmpBo[j]);
        }
    }
}

void GlesCsExecutor::initOperationTimers()
{
    operationTimers.resize(model.operations.size());
    for (size_t i = 0; i < operationTimers.size(); ++i)
    {
        const Operation& operation = model.operations[i];
        operationTimers[i].set(i, getOpName(operation));
    }
}

void GlesCsExecutor::showOperationTimers()
{
    float sum = 0.f;
    for (size_t i = 0; i < operationTimers.size(); ++i)
    {
        OperationCpuTimer& timer = operationTimers[i];
        long avg = timer.getAvg();
        if (avg == 0)   // it means that no timer recorded
        {
            return;
        }
        sum += avg;
    }

    std::sort(operationTimers.begin(), operationTimers.end(), OperationCpuTimer::sort);

    NN_GPU_PERF("each operation average CPU time (with glFinish), the first run ignored when run more than once");
    long acc = 0;
    for (size_t i = 0; i < operationTimers.size(); ++i)
    {
        OperationCpuTimer& timer = operationTimers[i];
        acc = timer.show(acc, sum);
    }
    NN_GPU_PERF("all operations average sum: %f ms", sum/1000.f);
}

void GlesCsExecutor::deinitPerModel()
{
    showOperationTimers();
    deinitOperationResources();

    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, _ctx) != EGL_TRUE)
    {
        LOGE("eglMakeCurrent failed to set within deinitPerModel");
        showEglError();
    }

    memMgr.clean();
    progMgr.clean();
    CHECKGLERROR();

    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        LOGE("eglMakeCurrent failed to clear within deinitPerModel");
        showEglError();
    }

    if (eglDestroyContext(dpy, _ctx) != EGL_TRUE)
    {
        LOGE("eglDestoryContext failed within deinitPerModel");
        showEglError();
    }

    if (eglReleaseThread() != EGL_TRUE)
    {
        LOGE("eglReleaseThread failed within deinitPerModel");
        showEglError();
    }
}

bool GlesCsExecutor::initPerExecThread()
{
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE)
    {
        LOGE("eglBindAPI failed within initPerExecThread");
        showEglError();
        return false;
    }

    // for the case with 2+ exec thread for the same model,
    // carefully use shared context to handle it.
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, _ctx) != EGL_TRUE)
    {
        LOGE("eglMakeCurrent failed to set within initPerExecThread");
        showEglError();
        return false;
    }

    return true;
}

void GlesCsExecutor::deinitPerExecThread()
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        LOGE("eglMakeCurrent failed to clear within deinitPerExecThread");
        showEglError();
    }

    if (eglReleaseThread() != EGL_TRUE)
    {
        LOGE("eglReleaseThread failed deinitPerExecThread");
        showEglError();
    }
}

void GlesCsExecutor::initOperands()
{
    const size_t count = model.operands.size();
    operands.resize(count, memMgr);

    for (size_t i = 0; i < count; i++)
    {
        const Operand& from = model.operands[i];
        GlesOperand& to = operands[i];
        to.set(from, const_cast<uint8_t*>(&model.operandValues[from.location.offset]), i);
    }
}

void GlesCsExecutor::restoreOperands()
{
    const size_t count = model.operands.size();

    for (size_t i = 0; i < count; i++)
    {
        const Operand& from = model.operands[i];
        GlesOperand& to = operands[i];
        to.restore(from);
    }
}

void GlesCsExecutor::setArgOperands(const Request& request)
{
    auto updateForArguments = [this](const hidl_vec<uint32_t>& indexes,
                                  const hidl_vec<RequestArgument>& arguments) {
        ASSERT(indexes.size() == arguments.size());
        for (size_t i = 0; i < indexes.size(); i++)
        {
            const RequestArgument& from = arguments[i];
            const size_t operandIndex = indexes[i];
            GlesOperand& to = operands[operandIndex];
            to.setArg(from);
        }
    };
    updateForArguments(model.inputIndexes, request.inputs);
    updateForArguments(model.outputIndexes, request.outputs);
}

bool GlesCsExecutor::run(const Operation& operation, OperationCpuTimer* timer, GlesOperationResource& resource)
{
    GlesCpuTimer t(timer);

    //maybe some checking here
    const hidl_vec<uint32_t>& inputs = operation.inputs;
    const hidl_vec<uint32_t>& outputs = operation.outputs;

    bool ret = true;

    switch (operation.type)
    {

#define SETUP_OP(op)                \
    case OperationType::op:         \
        NN_GPU_DEBUG("run operation type with %d", OperationType::op); \
        ret = do##op(operation, resource);    \
        break;
#include "gles_setup_op.hxx"
#undef SETUP_OP

    default:
        NOT_IMPLEMENTED;
        break;
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    for (uint32_t i : inputs)
    {
        operands[i].markOpFinished();
    }

    for (uint32_t i : outputs)
    {
        UNUSED(i);
        //operands[i].retrieveData();
    }

    return ret;
}

bool GlesCsExecutor::run(const Request& request)
{
    restoreOperands();
    memMgr.resetFromRequest(request);
    setArgOperands(request);
    for (size_t i = 0; i < model.operations.size(); ++i)
    {
        const Operation& operation = model.operations[i];
        OperationCpuTimer* timer = &operationTimers[i];
        if (!run(operation, timer, operationResources[i]))
        {
            return false;
        }
    }
    memMgr.sync();
    CHECKGLERROR();
    return true;
}

void GlesCsExecutor::setUniform1f(GLuint prog, const char* name, GLfloat v)
{
    GLint loc = glGetProgramResourceLocation(prog, GL_UNIFORM, name);
    if (loc != -1)
    {
        glUniform1f(loc, v);
    }
    else
    {
        LOGE("glGetProgramResourceLocation(\"%s\") returns -1", name);
    }
}

void GlesCsExecutor::setUniform1ui(GLuint prog, const char* name, GLuint v)
{
    GLint loc = glGetProgramResourceLocation(prog, GL_UNIFORM, name);
    if (loc != -1)
    {
        glUniform1ui(loc, v);
    }
    else
    {
        LOGE("glGetProgramResourceLocation(\"%s\") returns -1", name);
    }
}

void GlesCsExecutor::setTotal(GLuint prog, uint32_t totalX)
{
    setUniform1ui(prog, "uniform_total_x", totalX);
}

void GlesCsExecutor::setTotal(GLuint prog, uint32_t totalX, uint32_t totalY)
{
    setTotal(prog, totalX);
    setUniform1ui(prog, "uniform_total_y", totalY);
}

void GlesCsExecutor::setTotal(GLuint prog, uint32_t totalX, uint32_t totalY, uint32_t totalZ)
{
    setTotal(prog, totalX, totalY);
    setUniform1ui(prog, "uniform_total_z", totalZ);
}
void GlesCsExecutor::bindOperand(GlesOperand& operand, GLuint boindex)
{
    GLuint bo = operand.getSSbo();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, boindex, bo);
}

void GlesCsExecutor::getCapabilities(V1_0::Capabilities &cap)
{
    cap = {.float32Performance = {.execTime = 0.91f, .powerUsage = 0.91f},
                                  .quantized8Performance = {.execTime = 0.91f, .powerUsage = 0.91f}};
}

std::vector<bool> GlesCsExecutor::getSupportedOperations(const Model& model)
{
    const size_t count = model.operations.size();
    std::vector<bool> supported(count, true);
    for (size_t i = 0; i < count; ++i)
    {
        const Operation& operation = model.operations[i];
        const hidl_vec<uint32_t>& inputs = operation.inputs;
        for (auto input : inputs)
        {
            if (model.operands[input].type == OperandType::TENSOR_QUANT8_ASYMM)
            {
                LOGW("data type TENSOR_QUANT8_ASYMM not supported.");
                supported[i] = false;
                break;
            }
        }

        switch (operation.type)
        {

#define SETUP_OP(op)                    \
        case OperationType::op:         \
            /* do nothing */            \
            supported[i] = true; \
            break;
#include "gles_setup_op.hxx"
#undef SETUP_OP

        default:
            supported[i] = false;
            break;
        }
    }

    return supported;
}

bool GlesCsExecutor::checkGroupParam(int* localSize, int* groupCount)
{
    return (localSize[0] * localSize[1] * localSize[2] < max_wg_invocations &&
            localSize[0] < max_wg_size_x &&
            localSize[1] < max_wg_size_y &&
            localSize[2] < max_wg_size_z &&
            groupCount[0] < max_wg_count_x &&
            groupCount[1] < max_wg_count_y &&
            groupCount[2] < max_wg_count_z);
}

std::string GlesCsExecutor::getOpName(const Operation& operation)
{
    switch (operation.type)
    {

#define SETUP_OP(op)                \
    case OperationType::op:         \
        return std::string(#op);    \
        break;
#include "gles_setup_op.hxx"
#undef SETUP_OP

    default:
        break;
    }
    return "unknown";
}

NAME_SPACE_STOP
