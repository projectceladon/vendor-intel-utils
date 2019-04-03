#include "gles_cs_executor.h"
#include "gles_cs_program_key.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

bool GlesCsExecutor::doCONCATENATION(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    ASSERT(operation.type == OperationType::CONCATENATION);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    if (outs.size() != 1 || ins.size() < 2) {
        return false;
    }

    int32_t numInputTensors = ins.size() - 1;
    int32_t axis = operands[ins[numInputTensors]].getScalarData<int32_t>();
    int32_t activation = 0; // FIXME: CTS don't set activation, use default value here;
    GlesOperand& firstInput = operands[ins[0]];
    uint32_t numDims = firstInput.getNumberOfDimensions();
    NN_OPS_CHECK(axis >= 0);
    NN_OPS_CHECK(axis < (int32_t)numDims);
    int32_t concatSize;
    bool lastaxis = (axis == (int32_t)numDims - 1);
    if (lastaxis)
        concatSize = 1;
    else
        concatSize = firstInput.getElementCount(axis + 1);

    GlesOperand& output = operands[outs[0]];
    int32_t topConcatAxis = output.getDimensionSize(axis);
    if (firstInput.getType() == OperandType::TENSOR_FLOAT32)
    {
        uint32_t sum_axis = firstInput.getDimensionSize(axis);
        for (int32_t i = 1; i < numInputTensors; ++i)
        {
            GlesOperand& tensor = operands[ins[i]];
            NN_OPS_CHECK(tensor.getNumberOfDimensions() == numDims);
            NN_OPS_CHECK(tensor.getType() == firstInput.getType());
            for (int32_t d = 0; d < (int32_t)numDims; ++d)
            {
                if (d == axis)
                {
                    sum_axis += tensor.getDimensionSize(axis);
                }
                else
                {
                    NN_OPS_CHECK(firstInput.getDimensionSize(d) ==
                                 tensor.getDimensionSize(d));
                }
            }
        }

        for (int32_t d = 0; d < (int32_t)numDims; ++d)
        {
            if (d != axis)
            {
                NN_OPS_CHECK(output.getDimensionSize(d) ==
                             firstInput.getDimensionSize(d));
            }
        }
        NN_OPS_CHECK(output.getDimensionSize(axis) == sum_axis);

        int32_t lsz_x = 16;
        GlesCsProgramKeyConcatenation key;
        key.activation = activation;
        key.lastaxis = lastaxis;
        key.localSizeX = lsz_x;
        GLuint prog = progMgr.getProgram(&key);
        NN_CHECK(prog > 0);
        glUseProgram(prog);

        bindOperand(output, 1);
        int32_t offsetConcatAxis = 0;
        for (int32_t i = 0; i < numInputTensors; ++i)
        {
            GlesOperand& tensor = operands[ins[i]];
            bindOperand(tensor, 0);

            int32_t bottomConcatAxis = tensor.getDimensionSize(axis);
            int32_t numItems = tensor.getElementCount();

            GLint loc = glGetProgramResourceLocation(prog, GL_UNIFORM, "numItems");
            NN_CHECK(loc != -1);
            glUniform1i(loc, numItems);
            loc = glGetProgramResourceLocation(prog, GL_UNIFORM, "topConcatAxis");
            NN_CHECK(loc != -1);
            glUniform1i(loc, topConcatAxis);
            loc = glGetProgramResourceLocation(prog, GL_UNIFORM, "bottomConcatAxis");
            NN_CHECK(loc != -1);
            glUniform1i(loc, bottomConcatAxis);
            loc = glGetProgramResourceLocation(prog, GL_UNIFORM, "offsetConcatAxis");
            NN_CHECK(loc != -1);
            glUniform1i(loc, offsetConcatAxis);
            if (!lastaxis)
            {
                loc = glGetProgramResourceLocation(prog, GL_UNIFORM, "concatSize");
                NN_CHECK(loc != -1);
                glUniform1i(loc, concatSize);
            }
            glDispatchCompute(ALIGN(numItems, lsz_x) / lsz_x, 1, 1);
            offsetConcatAxis += bottomConcatAxis;
        }
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
