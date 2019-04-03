#include "gles_cs_executor.h"
#include "gles_cs_program_key.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesCsExecutor::doSOFTMAX(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    ASSERT(operation.type == OperationType::SOFTMAX);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    GlesOperand& in = operands[ins[0]];
    GlesOperand& out = operands[outs[0]];

    float beta = operands[ins[1]].getScalarData<float>();

    uint32_t numDim = in.getNumberOfDimensions();
    uint32_t total = in.getDimensionSize(0);   // one work item per batch
    uint32_t arraysize = 0;
    if(numDim == 2)
    {
        arraysize = in.getDimensionSize(1);
    }
    else
    {
        ASSERT(numDim == 4);
        arraysize = in.getDimensionSize(1) * in.getDimensionSize(2) * in.getDimensionSize(3);
    }

    uint32_t localSizeX = 8;

    GlesCsProgramKeyBasic key(OperationType::SOFTMAX);
    key.localSizeX = localSizeX;
    GLuint prog = progMgr.getProgram(&key);
    if (prog == 0)
    {
        return false;
    }
    glUseProgram(prog);

    bindOperand(in, 0);
    bindOperand(out, 1);
    setTotal(prog, total);
    setUniform1ui(prog, "uniform_array_size", arraysize);
    setUniform1f(prog, "uniform_beta", beta);
    glDispatchCompute((total + localSizeX - 1) / localSizeX, 1, 1);

    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
