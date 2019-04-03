#include <math.h>
#include "gles_cs_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesCsExecutor::doMAX_POOL_2D(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    int32_t input_width, input_height;
    int32_t output_width, output_height;
    int32_t input_chn, output_chn;
    int32_t filter_width, filter_height;
    int32_t padding_left, padding_right, padding_top, padding_bottom;
    int32_t stride_width, stride_height;
    int32_t activation;
    int32_t batch;
    // TODO: support auto-tuning these params?
    int32_t item_z= 1;
    int32_t lsz_x = 8;
    int32_t lsz_y = 8;
    int32_t lsz_z = 1;

    ASSERT(operation.type == OperationType::MAX_POOL_2D);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;
    const size_t inCount = ins.size();
    ASSERT(inCount == 10 || inCount == 7);

    GlesOperand& input  = operands[ins[0]];
    GlesOperand& output = operands[outs[0]];

    batch         = input.getDimensionSize(0);
    input_height  = input.getDimensionSize(1);
    input_width   = input.getDimensionSize(2);
    input_chn     = input.getDimensionSize(3);
    output_height = output.getDimensionSize(1);
    output_width  = output.getDimensionSize(2);
    output_chn    = output.getDimensionSize(3);
    ASSERT(output_chn == input_chn);

    if (inCount == 10) {
        padding_left     = operands[ins[1]].getScalarData<int32_t>();
        padding_right    = operands[ins[2]].getScalarData<int32_t>();
        padding_top      = operands[ins[3]].getScalarData<int32_t>();
        padding_bottom   = operands[ins[4]].getScalarData<int32_t>();
        stride_width     = operands[ins[5]].getScalarData<int32_t>();
        stride_height    = operands[ins[6]].getScalarData<int32_t>();
        filter_width     = operands[ins[7]].getScalarData<int32_t>();
        filter_height    = operands[ins[8]].getScalarData<int32_t>();
        activation       = operands[ins[9]].getScalarData<int32_t>();
    } else {
        int32_t padding_implicit = operands[ins[1]].getScalarData<int32_t>();
        stride_width     = operands[ins[2]].getScalarData<int32_t>();
        stride_height    = operands[ins[3]].getScalarData<int32_t>();
        filter_width     = operands[ins[4]].getScalarData<int32_t>();
        filter_height    = operands[ins[5]].getScalarData<int32_t>();
        activation       = operands[ins[6]].getScalarData<int32_t>();
        calculateExplicitPadding(input_width, stride_width,
                                 filter_width, padding_implicit,
                                 &padding_left, &padding_right);
        calculateExplicitPadding(input_height, stride_height,
                                 filter_height, padding_implicit,
                                 &padding_top, &padding_bottom);
    }

    GlesCsProgramKeyMaxPool key;
    key.activation = activation;
    key.localSizeX = lsz_x;
    key.localSizeY = lsz_y;
    key.localSizeZ = lsz_z;
    key.itemZ      = item_z;
    key.batch      = batch;

    if (input.getType() == OperandType::TENSOR_FLOAT32)
    {
        GLuint prog = progMgr.getProgram(&key);
        if (prog == 0)
        {
            return false;
        }

        bindOperand(input,  0);
        bindOperand(output, 1);

        glUseProgram(prog);
        glUniform1i(glGetUniformLocation(prog, "input_width"), input_width);
        glUniform1i(glGetUniformLocation(prog, "input_height"), input_height);
        glUniform1i(glGetUniformLocation(prog, "output_width"), output_width);
        glUniform1i(glGetUniformLocation(prog, "output_height"), output_height);
        glUniform1i(glGetUniformLocation(prog, "pad_w"), padding_left);
        glUniform1i(glGetUniformLocation(prog, "pad_h"), padding_top);
        glUniform1i(glGetUniformLocation(prog, "KERNEL_W"), filter_width);
        glUniform1i(glGetUniformLocation(prog, "KERNEL_H"), filter_height);
        glUniform1i(glGetUniformLocation(prog, "STRIDE_W"), stride_width);
        glUniform1i(glGetUniformLocation(prog, "STRIDE_H"), stride_height);
        glUniform1i(glGetUniformLocation(prog, "CHANNELS"), input_chn);

        size_t group_x = ceil(static_cast<float>(output_width) / lsz_x);
        size_t group_y = ceil(static_cast<float>(output_height) / lsz_y);
        // compute BATCH * ZPAR elements in per thread
        size_t group_z = ceil(static_cast<float>((ceil(static_cast<float>(output_chn) / item_z))) / lsz_z);
        /*
        printf("max pool: batch(%d), in(%d,%d,%d), out(%d,%d,%d), kernel(%d,%d), pad(%d,%d), stride(%d,%d), lsz(%d,%d,%d), itemz(%d), activation(%d)\n", 
              batch, input_height, input_width, input_chn, output_height, output_width, output_chn,
              filter_height, filter_width, padding_top, padding_left, stride_height, stride_width,
              lsz_x, lsz_y, lsz_z, item_z, activation);
        */
        glDispatchCompute(group_x, group_y, group_z);
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
