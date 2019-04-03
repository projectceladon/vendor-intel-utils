#include "gles_cs_program_manager.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

static const char mainpart[] =
"layout(binding = 0) readonly buffer Input0 {\n"
"    float data[];\n"
"} input0;\n"
"layout(binding = 1) writeonly buffer Output {\n"
"    float data[];\n"
"} output0;\n"
"uniform uint uniform_total_x;\n"
"void main()\n"
"{\n"
"    uint idx = gl_GlobalInvocationID.x;\n"
"    if (idx >= uniform_total_x) return;\n"
"    output0.data[idx] = 1.0f / (1.0f + exp(-input0.data[idx]));\n"
"}\n"
;

void GlesCsProgramManager::getProgNameLOGISTIC(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyBasic* key = reinterpret_cast<const GlesCsProgramKeyBasic*>(progKey);
    ASSERT(key->opType == OperationType::LOGISTIC);

    std::stringstream ss;
    ss << "optype" << (int)key->opType << "_"
       << "activation" << key->activation << "_"
       << "lsz("   << key->localSizeX << "," << key->localSizeY  << "," << key->localSizeZ << ")";
    name = ss.str();
}

void GlesCsProgramManager::getShaderSourceLOGISTIC(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyBasic* key = reinterpret_cast<const GlesCsProgramKeyBasic*>(progKey);
    ASSERT(key->opType == OperationType::LOGISTIC);

    std::stringstream ss;
    ss << "#version 320 es\n";
    ss << "layout(local_size_x = " << key->localSizeX << ") in;\n";
    ss << mainpart;

    src = ss.str();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
