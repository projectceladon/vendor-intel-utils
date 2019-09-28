#include "gles_cs_program_manager.h"

NAME_SPACE_BEGIN

static const char mainpart[] =
"#ifdef ACTIVATION_RELU\n"
"#define ACTIVATION_FUNCTION(x)  ((x) < 0.f ? 0.f : (x))\n"
"#elif defined (ACTIVATION_RELU1)\n"
"#define ACTIVATION_FUNCTION(x)  ((x) > 1.f ? 1.f : (x) < -1.f ? -1.f : (x))\n"
"#elif defined (ACTIVATION_RELU6)\n"
"#define ACTIVATION_FUNCTION(x)  ((x) > 6.f ? 6.f : (x) < 0.f ? 0.f : (x))\n"
"#else\n"
"#define ACTIVATION_FUNCTION(x)  (x)\n"
"#endif\n"
"layout(binding = 0) readonly buffer Input0 {\n"
"    float data[];\n"
"} input0;\n"
"layout(binding = 1) readonly buffer Input1 {\n"
"    float data[];\n"
"} input1;\n"
"layout(binding = 2) writeonly buffer Output {\n"
"    float data[];\n"
"} output0;\n"
"uniform uint uniform_total_x;\n"
"uniform uint uniform_round;\n"
"void main()\n"
"{\n"
"    uint idx = gl_GlobalInvocationID.x;\n"
"    if (idx >= uniform_total_x) return;\n"
""
;

void GlesCsProgramManager::getProgNameADD(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyAdd* key = reinterpret_cast<const GlesCsProgramKeyAdd*>(progKey);
    ASSERT(key->opType == OperationType::ADD);

    std::stringstream ss;
    ss << "optype" << (int)key->opType << "_"
       << "activation" << key->activation << "_"
       << "lsz("   << key->localSizeX << "," << key->localSizeY  << "," << key->localSizeZ << ")_"
       << "broadcast" << key->broadcast;
    name = ss.str();
}

void GlesCsProgramManager::getShaderSourceADD(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyAdd* key = reinterpret_cast<const GlesCsProgramKeyAdd*>(progKey);
    ASSERT(key->opType == OperationType::ADD);

    std::stringstream ss;
    ss << "#version 320 es\n";
    switch (key->activation)
    {
        case FusedActivationFunctionType::kRelu6:
            ss << "#define ACTIVATION_RELU6\n";
            break;
        case FusedActivationFunctionType::kRelu1:
            ss << "#define ACTIVATION_RELU1\n";
            break;
        case FusedActivationFunctionType::kRelu:
            ss << "#define ACTIVATION_RELU\n";
            break;
        case FusedActivationFunctionType::kNone:
            break;
        default:
            NOT_REACH_HERE;
    }

    ss << "layout(local_size_x = " << key->localSizeX << ") in;\n";
    ss << mainpart;

    if (key->broadcast)
    {
        ss << "    float f = input0.data[idx] + input1.data[idx % uniform_round];\n";
    }
    else
    {
        ss << "    float f = input0.data[idx] + input1.data[idx];\n";
    }

    ss << "    output0.data[idx] = ACTIVATION_FUNCTION(f);\n";
    ss << "}\n";

    src = ss.str();
}

NAME_SPACE_STOP

