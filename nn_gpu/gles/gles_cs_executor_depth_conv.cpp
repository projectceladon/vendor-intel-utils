#include <math.h>
#include "gles_cs_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesCsExecutor::doDEPTHWISE_CONV_2D(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    int32_t input_width, input_height;
    int32_t output_width, output_height;
    int32_t input_chn, output_chn;
    int32_t filter_width, filter_height;
    int32_t padding_left, padding_right, padding_top, padding_bottom;
    int32_t stride_width, stride_height;
    int32_t depth_multiplier;
    int32_t activation;
    int32_t batch;
    // TODO: auto-tune to find the best local size config for performance
    int32_t lsz_x = 1;
    int32_t lsz_y = 1;
    int32_t lsz_z = 16;
    // FIXME:
    // Android NN don't set group, dilation, has_bias,
    // so make these assumptions: group = 1, dilation = 1, has_bias = 1
    int32_t image_offset = 0;
    int32_t bias_offset = 0;
    int32_t kernel_offset = 0;
    int32_t convolved_image_offset = 0;
    int32_t dilation_x = 1;
    int32_t dilation_y = 1;
    int32_t has_bias = 1;

    ASSERT(operation.type == OperationType::DEPTHWISE_CONV_2D);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;
    const size_t inCount = ins.size();
    ASSERT(inCount == 11 || inCount == 8);

    GlesOperand& input  = operands[ins[0]];
    GlesOperand& filter = operands[ins[1]];
    GlesOperand& bias   = operands[ins[2]];
    GlesOperand& output = operands[outs[0]];

    batch         = input.getDimensionSize(0);
    input_height  = input.getDimensionSize(1);
    input_width   = input.getDimensionSize(2);
    input_chn     = input.getDimensionSize(3);
    output_height = output.getDimensionSize(1);
    output_width  = output.getDimensionSize(2);
    output_chn    = output.getDimensionSize(3);
    filter_height = filter.getDimensionSize(1);
    filter_width  = filter.getDimensionSize(2);

    if (inCount == 11) {
        padding_left     = operands[ins[3]].getScalarData<int32_t>();
        padding_right    = operands[ins[4]].getScalarData<int32_t>();
        padding_top      = operands[ins[5]].getScalarData<int32_t>();
        padding_bottom   = operands[ins[6]].getScalarData<int32_t>();
        stride_width     = operands[ins[7]].getScalarData<int32_t>();
        stride_height    = operands[ins[8]].getScalarData<int32_t>();
        depth_multiplier = operands[ins[9]].getScalarData<int32_t>();
        activation       = operands[ins[10]].getScalarData<int32_t>();
    } else {
        int32_t padding_implicit = operands[ins[3]].getScalarData<int32_t>();
        stride_width     = operands[ins[4]].getScalarData<int32_t>();
        stride_height    = operands[ins[5]].getScalarData<int32_t>();
        depth_multiplier = operands[ins[6]].getScalarData<int32_t>();
        activation       = operands[ins[7]].getScalarData<int32_t>();
        calculateExplicitPadding(input_width, stride_width,
                                 filter_width, padding_implicit,
                                 &padding_left, &padding_right);
        calculateExplicitPadding(input_height, stride_height,
                                 filter_height, padding_implicit,
                                 &padding_top, &padding_bottom);
    }
    ASSERT(output_chn == input_chn * depth_multiplier);

    GlesCsProgramKeyDepthConv key;
    key.activation = activation;
    key.localSizeX = lsz_x;
    key.localSizeY = lsz_y;
    key.localSizeZ = lsz_z;
    key.itemZ      = depth_multiplier; // TODO: support anbitrary itemZ

    /*
    printf("depth conv: batch(%d), in(%d,%d,%d), out(%d,%d,%d), kernel(%d,%d), pad(%d,%d), lsz(%d,%d,%d), itemz(%d), activation(%d)\n",
    batch, input_height, input_width, input_chn, output_height, output_width, output_chn,
    filter_height, filter_width, padding_top, padding_left,
    lsz_x, lsz_y, lsz_z, depth_multiplier, activation);
    */

    if (input.getType() == OperandType::TENSOR_FLOAT32)
    {
        GLuint prog = progMgr.getProgram(&key);
        if (prog == 0)
        {
            return false;
        }

        bindOperand(input,  0);
        bindOperand(bias,   1);
        bindOperand(filter, 2);
        bindOperand(output, 3);

        glUseProgram(prog);
        glUniform1i(glGetUniformLocation(prog, "image_offset"), image_offset);
        glUniform1i(glGetUniformLocation(prog, "convolved_image_offset"), convolved_image_offset);
        glUniform1i(glGetUniformLocation(prog, "bias_offset"), bias_offset);
        glUniform1i(glGetUniformLocation(prog, "kernel_offset"), kernel_offset);
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
        glUniform1i(glGetUniformLocation(prog, "DILATION_X"), dilation_x);
        glUniform1i(glGetUniformLocation(prog, "DILATION_Y"), dilation_y);
        glUniform1i(glGetUniformLocation(prog, "CHANNELS"), input_chn);
        glUniform1i(glGetUniformLocation(prog, "TOTAL_OUTPUT_DEPTH"), output_chn);
        glUniform1i(glGetUniformLocation(prog, "OUTPUT_Z"), output_chn);
        glUniform1i(glGetUniformLocation(prog, "APPLY_BIAS"), has_bias);
        glUniform1i(glGetUniformLocation(prog, "depth_multiplier"), depth_multiplier);

        int group_x = ceil(static_cast<float>(output_width) / lsz_x);
        int group_y = ceil(static_cast<float>(output_height) / lsz_y);
        int group_z = ceil(static_cast<float>((ceil(static_cast<float>(output_chn) * batch / depth_multiplier))) / lsz_z);
        // effective global size is (output_width, output_height, input_chn), each thread to calculate depth_multiplier output elements
        glDispatchCompute(group_x, group_y, group_z);
        CHECKGLERROR();
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
