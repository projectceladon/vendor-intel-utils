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
"uniform float uniform_beta;\n"
"uniform uint uniform_array_size;\n"
"uniform uint uniform_total_x;\n"
"void main()\n"
"{\n"
"    uint idx = gl_GlobalInvocationID.x;\n"
"    if (idx >= uniform_total_x) return;\n"
"\n"
"    uint start = idx * uniform_array_size;\n"
"    float max = 0.0f;\n"
"    for (uint i = 0u; i < uniform_array_size; ++i)\n"
"    {\n"
"       uint index = start + i;\n"
"       if (max < input0.data[index])\n"
"       {\n"
"               max = input0.data[index];\n"
"       }\n"
"    }\n"
"\n"
"    float sum = 0.0f;\n"
"    for (uint i = 0u; i < uniform_array_size; ++i)\n"
"    {\n"
"       uint index = start + i;\n"
"       float f = exp(uniform_beta * (input0.data[index] - max));\n"
"       sum += f;\n"
"    }\n"
"\n"
"    for (uint i = 0u; i < uniform_array_size; ++i)\n"
"    {\n"
"       uint index = start + i;\n"
"       float f = exp(uniform_beta * (input0.data[index] - max));\n"
"       output0.data[index] = f / sum;\n"
"    }\n"
"}\n"
;

void GlesCsProgramManager::getProgNameSOFTMAX(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyBasic* key = reinterpret_cast<const GlesCsProgramKeyBasic*>(progKey);
    ASSERT(key->opType == OperationType::SOFTMAX);

    std::stringstream ss;
    ss << "optype" << (int)key->opType << "_"
       << "activation" << key->activation << "_"
       << "lsz(" << key->localSizeX << "," << key->localSizeY  << "," << key->localSizeZ << ")";
    name = ss.str();
}


void GlesCsProgramManager::getShaderSourceSOFTMAX(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyBasic* key = reinterpret_cast<const GlesCsProgramKeyBasic*>(progKey);
    ASSERT(key->opType == OperationType::SOFTMAX);

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
