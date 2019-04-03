#include <math.h>
#include "gles_cs_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesCsExecutor::doLOCAL_RESPONSE_NORMALIZATION(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    ASSERT(operation.type == OperationType::LOCAL_RESPONSE_NORMALIZATION);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    GlesOperand& input = operands[ins[0]];
    GlesOperand& oprd_radius = operands[ins[1]];
    GlesOperand& oprd_bias   = operands[ins[2]];
    GlesOperand& oprd_alpha  = operands[ins[3]];
    GlesOperand& oprd_beta   = operands[ins[4]];
    GlesOperand& output = operands[outs[0]];

    NN_OPS_CHECK(input.getNumberOfDimensions() == 4);
    NN_OPS_CHECK(output.getNumberOfDimensions() == 4);
    for (int i = 0; i < 4; i++)
    {
        NN_CHECK(input.getDimensionSize(i) == output.getDimensionSize(i));
    }

    int32_t batch  = input.getDimensionSize(0);
    int32_t height = input.getDimensionSize(1);
    int32_t width  = input.getDimensionSize(2);
    int32_t channels = input.getDimensionSize(3);
    int32_t radius   = oprd_radius.getScalarData<int32_t>();
    int32_t numItems = batch * height * width;
    int32_t filterLen = radius * 2 + 1;
    float bias   = oprd_bias.getScalarData<float>();
    float alpha  = oprd_alpha.getScalarData<float>();
    float negativeBeta = -1.0 * oprd_beta.getScalarData<float>();

    if (input.getType() == OperandType::TENSOR_FLOAT32)
    {
        int32_t lsz_x = 16;
        GlesCsProgramKeyLRN key;
        key.localSizeX = lsz_x;
        GLuint prog = progMgr.getProgram(&key);
        if (prog == 0)
        {
            return false;
        }

        bindOperand(input,  0);
        bindOperand(output, 1);
        glUseProgram(prog);
        glUniform1i(glGetUniformLocation(prog, "numItems"), numItems);
        glUniform1i(glGetUniformLocation(prog, "channels"), channels);
        glUniform1i(glGetUniformLocation(prog, "height"), height);
        glUniform1i(glGetUniformLocation(prog, "width"), width);
        glUniform1i(glGetUniformLocation(prog, "filterLen"), filterLen);
        glUniform1i(glGetUniformLocation(prog, "radius"), radius);
        glUniform1f(glGetUniformLocation(prog, "alpha"), alpha);
        glUniform1f(glGetUniformLocation(prog, "bias"), bias);
        glUniform1f(glGetUniformLocation(prog, "negativeBeta"), negativeBeta);
        glDispatchCompute(ALIGN(numItems, lsz_x) / lsz_x, 1, 1);
        CHECK_GL_STATE_RET();
    }
    else
    {
        NOT_IMPLEMENTED;
    }
    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
