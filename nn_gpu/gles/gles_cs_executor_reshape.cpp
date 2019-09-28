#include "gles_cs_executor.h"
#include "gles_cs_program_key.h"

NAME_SPACE_BEGIN

bool GlesCsExecutor::doRESHAPE(const Operation& operation, GlesOperationResource& resource)
{
    UNUSED(resource);
    ASSERT(operation.type == OperationType::RESHAPE);
    const hidl_vec<uint32_t>& ins  = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    GlesOperand& input       = operands[ins[0]];
    // GlesOperand& targetShape = operands[ins[1]]; // Do we need this parameters?
    GlesOperand& output      = operands[outs[0]];

    output.shareGpuStorage(input);
    return true;
}

NAME_SPACE_STOP
