#include "gles_cs_program_manager.h"

NAME_SPACE_BEGIN

static const char mainpart[] =
"#define UINT_0 (uint(0))\n"
"#ifdef ACTIVATION_RELU\n"
"#define ACTIVATION_FUNCTION(x)  ((x) < 0.f ? 0.f : (x))\n"
"#elif defined (ACTIVATION_RELU1)\n"
"#define ACTIVATION_FUNCTION(x)  ((x) > 1.f ? 1.f : (x) < -1.f ? -1.f : (x))\n"
"#elif defined (ACTIVATION_RELU6)\n"
"#define ACTIVATION_FUNCTION(x)  ((x) > 6.f ? 6.f : (x) < 0.f ? 0.f : (x))\n"
"#else\n"
"#define ACTIVATION_FUNCTION(x)  (x)\n"
"#endif\n"
"uniform uint input_width;\n"
"uniform uint input_height;\n"
"uniform uint output_width;\n"
"uniform uint output_height;\n"
"uniform uint pad_w;\n"
"uniform uint pad_h;\n"
"uniform uint KERNEL_W;\n"
"uniform uint KERNEL_H;\n"
"uniform uint STRIDE_W;\n"
"uniform uint STRIDE_H;\n"
"uniform uint CHANNELS;\n"
"layout(binding = 0) readonly buffer Input0{\n"
"    float data[];\n"
"} image_data;\n"
"layout(binding = 1) writeonly buffer Output{\n"
"    float data[];\n"
"} convolved_image;\n"
"layout(local_size_x = LOCAL_SZ_X, local_size_y = LOCAL_SZ_Y, local_size_z = LOCAL_SZ_Z) in;\n"
"void main()\n"
"{\n"
"    uint outputX = gl_GlobalInvocationID.x;\n"
"    uint outputY = gl_GlobalInvocationID.y;\n"
"    uint outputZ = gl_GlobalInvocationID.z * ZPAR;\n"
"    if(outputX < output_width && outputY < output_height)\n"
"    {\n"
"        float sum[BATCH * ZPAR];\n"
"        for(uint outz = uint(0); outz < BATCH * ZPAR; outz++)\n"
"        {\n"
"            sum[outz] = 0.0f;\n"
"        }\n"
"        int org_y = int(outputY * STRIDE_H - pad_h);\n"
"        int org_x = int(outputX * STRIDE_W - pad_w);\n"
"        uint input_image_size  = input_width * input_height * CHANNELS;\n"
"        uint output_image_size = output_width * output_height * CHANNELS;\n"
"        uint local_image_offset = (uint(org_y)*input_width + uint(org_x)) * CHANNELS + outputZ;\n"
"        uint batch_offset = UINT_0;\n"
"        uint cnt = UINT_0;\n"
"        uint num = UINT_0;\n"
"        for (uint b = UINT_0; b < BATCH; b++)\n"
"        {\n"
"            for(uint y = UINT_0; y < KERNEL_H; y++)\n"
"            {\n"
"                for(uint x = UINT_0; x < KERNEL_W; x++)\n"
"                {\n"
"                    if(org_y + int(y) >= 0 && org_y + int(y) < int(input_height) && org_x + int(x) >= 0 && org_x + int(x) < int(input_width))\n"
"                    {\n"
"                        for(uint outz =UINT_0; outz < ZPAR; outz++)\n"
"                        {\n"
"                            sum[b * ZPAR + outz] += image_data.data[local_image_offset + outz];\n"
"                        }\n"
"                        cnt++;\n"
"                    }\n"
"                    local_image_offset += CHANNELS;\n"
"                }\n"
"                local_image_offset += input_width * CHANNELS - CHANNELS * KERNEL_W;\n"
"            }\n"
"            local_image_offset += input_image_size - CHANNELS * KERNEL_W - input_width * KERNEL_H * CHANNELS;\n"
"            if (b == UINT_0)\n"
"                num = cnt;\n"
"        }\n"
"        for (uint b = UINT_0; b < BATCH; b++)\n"
"        {\n"
"            for(uint outz = UINT_0; outz < ZPAR; outz++)\n"
"            {\n"
"                if (outputZ + outz < CHANNELS)\n"
"                {\n"
"                    uint offset = batch_offset + (outputY * output_width  + outputX) * CHANNELS + outputZ + outz;\n"
"                    convolved_image.data[offset] = ACTIVATION_FUNCTION(sum[b * ZPAR + outz] / float(num));\n"
"                }\n"
"            }\n"
"            batch_offset += output_image_size;\n"
"        }\n"
"    }\n"
"}\n"
;

void GlesCsProgramManager::getProgNameAVERAGE_POOL_2D(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyAvgPool* key = reinterpret_cast<const GlesCsProgramKeyAvgPool*>(progKey);
    ASSERT(key->opType == OperationType::AVERAGE_POOL_2D);

    std::stringstream ss;
    ss << "optype" << (int)key->opType << "_"
       << "activation" << key->activation << "_"
       << "lsz("  << key->localSizeX << "," << key->localSizeY  << "," << key->localSizeZ << ")_"
       << "itemZ" << key->itemZ << "_"
       << "batch" << key->batch;
    name = ss.str();
}

void GlesCsProgramManager::getShaderSourceAVERAGE_POOL_2D(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyAvgPool* key = reinterpret_cast<const GlesCsProgramKeyAvgPool*>(progKey);
    ASSERT(key->opType == OperationType::AVERAGE_POOL_2D);

    std::stringstream ss;
    ss << "#version 320 es\n";
    ss << "#define LOCAL_SZ_X " << key->localSizeX << "\n";
    ss << "#define LOCAL_SZ_Y " << key->localSizeY << "\n";
    ss << "#define LOCAL_SZ_Z " << key->localSizeZ << "\n";
    ss << "#define ZPAR (uint(" << key->itemZ << "))\n";
    ss << "#define BATCH (uint(" << key->batch<< "))\n";

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

    ss << mainpart;
    src = ss.str();
}

NAME_SPACE_STOP
