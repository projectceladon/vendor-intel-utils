#include "gles_cs_executor.h"
#include "gles_cs_program_key.h"

NAME_SPACE_BEGIN

bool GlesCsExecutor::doMUL(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    ASSERT(operation.type == OperationType::MUL);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    GlesOperand& in0 = operands[ins[0]];
    GlesOperand& in1 = operands[ins[1]];
    const GlesOperand& in2 = operands[ins[2]];
    int32_t activation = in2.getScalarData<int32_t>();

    GlesOperand& out = operands[outs[0]];

    uint32_t total = in0.getElementCount();
    uint32_t localSizeX = 8;

    bool needBroadcast = false;
    GlesOperand* longer = nullptr;
    GlesOperand* shorter = nullptr;
    if (in0.getElementCount() > in1.getElementCount())
    {
        longer = &in0;
        shorter = &in1;
        needBroadcast = true;
    }
    else if (in0.getElementCount() < in1.getElementCount())
    {
        longer = &in1;
        shorter = &in0;
        needBroadcast = true;
    }

    GlesCsProgramKeyMul key;
    key.activation = activation;
    key.localSizeX = localSizeX;
    key.broadcast = needBroadcast;
    GLuint prog = progMgr.getProgram(&key);
    if (prog == 0)
    {
        return false;
    }
    glUseProgram(prog);

    if (needBroadcast)
    {
        bindOperand(*longer, 0);
        bindOperand(*shorter, 1);
        setUniform1ui(prog, "uniform_round", std::min(in0.getElementCount(), in1.getElementCount()));
    }
    else
    {
        bindOperand(in0, 0);
        bindOperand(in1, 1);
    }
    bindOperand(out, 2);
    setTotal(prog, total);
    glDispatchCompute((total + localSizeX - 1) / localSizeX, 1, 1);

    return true;
}

NAME_SPACE_STOP
