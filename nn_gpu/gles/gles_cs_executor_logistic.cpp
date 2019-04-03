#include "gles_cs_executor.h"
#include "gles_cs_program_key.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesCsExecutor::doLOGISTIC(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    ASSERT(operation.type == OperationType::LOGISTIC);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    GlesOperand& in = operands[ins[0]];
    GlesOperand& out = operands[outs[0]];

    uint32_t total = in.getElementCount();
    uint32_t localSizeX = 8;

    GlesCsProgramKeyBasic key(OperationType::LOGISTIC);
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
    glDispatchCompute((total + localSizeX - 1) / localSizeX, 1, 1);

    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
