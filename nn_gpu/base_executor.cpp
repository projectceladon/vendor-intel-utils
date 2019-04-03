#include "base_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

std::string BaseExecutor::getOpName(const Operation& operation)
{
    switch (operation.type)
    {

#define SETUP_OP(op)                \
    case OperationType::op:         \
        return std::string(#op);    \
        break;
#include "setup_op.hxx"
#undef SETUP_OP

    default:
        break;
    }
    return "unknown";
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android