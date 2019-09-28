#include "gles_cs_program_manager.h"

NAME_SPACE_BEGIN

static const char lrnShader[] = 
"uniform int numItems;\n"
"uniform int channels;\n"
"uniform int height;\n"
"uniform int width;\n"
"uniform int filterLen;\n"
"uniform int radius;\n"
"uniform float alpha;\n"
"uniform float bias;\n"
"uniform float negativeBeta;\n"
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
"    int x = index % width;\n"
"    int y = (index / width) % height;\n"
"    int n = index / width / height;\n"
"    int offset = channels * ((n * height + y) * width + x);\n"
"    float scale_val;\n"
"    int head = 0;\n"
"    float accum_scale = 0.0f;\n"
"    int min_val = radius < channels ? radius : channels;\n"
"    while (head < min_val) {\n"
"      accum_scale += src.data[offset + head] * src.data[offset + head];\n"
"      ++head;\n"
"    }\n"
"    while (head < channels) {\n"
"      accum_scale += src.data[offset + head] * src.data[offset + head];\n"
"      if (head - filterLen >= 0) {\n"
"        accum_scale -= src.data[offset + (head - filterLen)]\n"
"            * src.data[offset + (head - filterLen)];\n"
"      }\n"
"      scale_val = bias + accum_scale * alpha;\n"
"      dst.data[offset + (head - radius)] = src.data[offset + (head - radius)] * pow(scale_val, negativeBeta);\n"
"      ++head;\n"
"    }\n"
"    int pos = head - min_val;\n"
"    while (pos >= 0 && pos < channels) {\n"
"      if (head - filterLen >= 0) {\n"
"        accum_scale -= src.data[offset + (head - filterLen)]\n"
"            * src.data[offset + (head - filterLen)];\n"
"      }\n"
"      scale_val = bias + accum_scale * alpha;\n"
"      dst.data[offset + pos] = src.data[offset + pos] * pow(scale_val, negativeBeta);\n"
"      ++head;\n"
"      ++pos;\n"
"    }\n"
"  }\n"
"}\n"
;

void GlesCsProgramManager::getProgNameLOCAL_RESPONSE_NORMALIZATION(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyLRN* key = reinterpret_cast<const GlesCsProgramKeyLRN*>(progKey);
    ASSERT(key->opType == OperationType::LOCAL_RESPONSE_NORMALIZATION);

    std::stringstream ss;
    ss << "optype" << (int)key->opType;
    name = ss.str();
}

void GlesCsProgramManager::getShaderSourceLOCAL_RESPONSE_NORMALIZATION(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyLRN* key = reinterpret_cast<const GlesCsProgramKeyLRN*>(progKey);
    ASSERT(key->opType == OperationType::LOCAL_RESPONSE_NORMALIZATION);

    std::stringstream ss;
    ss << "#version 320 es\n";
    ss << "#define LOCAL_SZ_X " << key->localSizeX << "\n";
    ss << lrnShader;
    src = ss.str();
}

NAME_SPACE_STOP
