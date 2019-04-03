#include "gles_cs_executor.h"
#include "gles_cs_program_key.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool computeGroupParam(uint32_t totalThreadX,
                       uint32_t preferLocalSizeX,
                       uint32_t& localSizeX,
                       uint32_t& groupCountX)
{
    int lsz[3] = {1, 1, 1};
    int groupCount[3] = {1, 1, 1};

    lsz[0] = preferLocalSizeX;
    groupCount[0] = (totalThreadX + lsz[0] - 1) / lsz[0];
    if (GlesCsExecutor::checkGroupParam(lsz, groupCount))
    {
        localSizeX = lsz[0];
        groupCountX = groupCount[0];
        return true;
    }

    for (int localX = 1; localX < GlesCsExecutor::max_wg_size_x; localX *= 2)
    {
        lsz[0] = localX;
        groupCount[0] = (totalThreadX + lsz[0] - 1) / lsz[0];
        if (GlesCsExecutor::checkGroupParam(lsz, groupCount))
        {
            localSizeX = lsz[0];
            groupCountX = groupCount[0];
            return true;
        }
    }

    return false;
}

bool GlesCsExecutor::doADD(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    ASSERT(operation.type == OperationType::ADD);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    GlesOperand& in0 = operands[ins[0]];
    GlesOperand& in1 = operands[ins[1]];
    const GlesOperand& in2 = operands[ins[2]];
    int32_t activation = in2.getScalarData<int32_t>();

    GlesOperand& out = operands[outs[0]];

    uint32_t total = in0.getElementCount();
    uint32_t localSizeX = 8;
    uint32_t groupCountX;

    computeGroupParam(total, localSizeX, localSizeX, groupCountX);

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

    GlesCsProgramKeyAdd key;
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
    glDispatchCompute(groupCountX, 1, 1);

    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
