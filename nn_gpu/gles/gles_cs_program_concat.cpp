#include "gles_cs_program_manager.h"

NAME_SPACE_BEGIN

static const char concatShader[] = 
"#ifdef LAST_AXIS\n"
"#define concatSize 1\n"
"#else\n"
"uniform int concatSize;\n"
"#endif\n"
"uniform int numItems;\n"
"uniform int topConcatAxis;\n"
"uniform int bottomConcatAxis;\n"
"uniform int offsetConcatAxis;\n"
"layout(binding = 0) readonly buffer Input0{\n"
"    float data[];\n"
"} src;\n"
"layout(binding = 1) writeonly buffer Output{\n"
"    float data[];\n"
"} dst;\n"
"layout(local_size_x = LOCAL_SZ_X, local_size_y = 1, local_size_z = 1) in;\n"
"void main()\n"
"{\n"
"  int gid = int(gl_GlobalInvocationID.x); \n"
"  int gsz = int(gl_NumWorkGroups.x * gl_WorkGroupSize.x); \n"
"  for (int index = gid; index < numItems; index += gsz)\n"
"  {\n"
"    int total_concatSize = concatSize * bottomConcatAxis;\n"
"    int concatNum = index / total_concatSize;\n"
"    int concatIndex = index % total_concatSize;\n"
"    int outIndex = concatIndex + (concatNum * topConcatAxis + offsetConcatAxis) * concatSize;\n"
"    dst.data[outIndex] = src.data[index];\n"
"  }\n"
"}\n"
;

void GlesCsProgramManager::getProgNameCONCATENATION(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyConcatenation* key = reinterpret_cast<const GlesCsProgramKeyConcatenation*>(progKey);
    ASSERT(key->opType == OperationType::CONCATENATION);

    std::stringstream ss;
    ss << "optype" << (int)key->opType << "_"
       << "activation" << key->activation << "_"
       << "lastaxis" << key->lastaxis;
    name = ss.str();
}

void GlesCsProgramManager::getShaderSourceCONCATENATION(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyConcatenation* key = reinterpret_cast<const GlesCsProgramKeyConcatenation*>(progKey);
    ASSERT(key->opType == OperationType::CONCATENATION);

    std::stringstream ss;
    ss << "#version 320 es\n";
    ss << "#define LOCAL_SZ_X " << key->localSizeX << "\n";
    if (key->lastaxis)
        ss << "#define LAST_AXIS \n";
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
    ss << concatShader;
    src = ss.str();
}

NAME_SPACE_STOP
