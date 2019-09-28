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
"uniform int image_offset;\n"
"uniform int convolved_image_offset;\n"
"uniform int bias_offset;\n"
"uniform int kernel_offset;\n"
"uniform int input_width;\n"
"uniform int input_height;\n"
"uniform int output_width;\n"
"uniform int output_height;\n"
"uniform int pad_w;\n"
"uniform int pad_h;\n"
"uniform int depth_multiplier;\n"
"uniform int KERNEL_W;\n"
"uniform int KERNEL_H;\n"
"uniform int STRIDE_W;\n"
"uniform int STRIDE_H;\n"
"uniform int DILATION_X;\n"
"uniform int DILATION_Y;\n"
"uniform int CHANNELS;\n"
"uniform int TOTAL_OUTPUT_DEPTH;\n"
"uniform int OUTPUT_Z;\n"
"uniform bool APPLY_BIAS;\n"
"layout(binding = 0) readonly buffer Input0{\n"
"    float data[];\n"
"} image_data;\n"
"layout(binding = 1) readonly buffer Input1 {\n"
"    float data[];\n"
"} bias;\n"
"layout(binding = 2) readonly buffer Input3{\n"
"    float data[];\n"
"} kernel_data;\n"
"layout(binding = 3) writeonly buffer Output{\n"
"    float data[];\n"
"} convolved_image;\n"
"layout(local_size_x = LOCAL_SZ_X, local_size_y = LOCAL_SZ_Y, local_size_z = LOCAL_SZ_Z) in;\n"
"void main()\n"
"{\n"
"    int outputX = int(gl_GlobalInvocationID.x);\n"
"    int outputY = int(gl_GlobalInvocationID.y);\n"
"    int inputZ  = int(gl_GlobalInvocationID.z);\n"
"    int outputZ = inputZ * depth_multiplier;\n"
"    if(outputX < output_width && outputY < output_height)\n"
"    {\n"
"        float sum[ZPAR];\n"
"        for(int outz = 0; outz < depth_multiplier; outz++)\n"
"        {\n"
"            sum[outz] = 0.0f;\n"
"        }\n"
"        int org_y = outputY * STRIDE_H - pad_h;\n"
"        int org_x = outputX * STRIDE_W - pad_w;\n"
"        int kernel_dataPtrFloat = kernel_offset + outputZ;\n"
"        int biasIndex=bias_offset + outputZ;\n"
"        int image_dataPtrFloat = image_offset + (org_y*input_width + org_x) * CHANNELS + inputZ;\n"
"        for(int y = 0; y < KERNEL_H; y++)\n"
"        {\n"
"            for(int x = 0; x < KERNEL_W; x++)\n"
"            {\n"
"                if(org_y + y * DILATION_Y >= 0 && org_y + y * DILATION_Y < input_height && org_x + x * DILATION_X >= 0 && org_x + x * DILATION_X < input_width)\n"
"                {\n"
"                    for(int outz =0; outz < depth_multiplier; outz++)\n"
"                    {\n"
"                        sum[outz] += image_data.data[image_dataPtrFloat] * kernel_data.data[kernel_dataPtrFloat + outz];\n"
"                    }\n"
"                }\n"
"                image_dataPtrFloat += DILATION_X * CHANNELS;\n"
"                kernel_dataPtrFloat += CHANNELS * depth_multiplier;\n"
"            }\n"
"            image_dataPtrFloat += input_width * DILATION_Y * CHANNELS - DILATION_X * CHANNELS * KERNEL_W;\n"
"        }\n"
"        if (APPLY_BIAS)\n"
"        {\n"
"        for(int outz = 0; outz < depth_multiplier; outz++)\n"
"        {\n"
"            if (outputZ + outz < TOTAL_OUTPUT_DEPTH)\n"
"            {\n"
"                int offset = convolved_image_offset + (outputY * output_width  + outputX) * TOTAL_OUTPUT_DEPTH + outputZ + outz;\n"
"                convolved_image.data[offset] = ACTIVATION_FUNCTION(sum[outz] + bias.data[biasIndex +outz]);\n"
"            }\n"
"        }\n"
"        }\n"
"        else\n"
"        {\n"
"        for(int outz = 0; outz < depth_multiplier; outz++)\n"
"        {\n"
"            if (outputZ + outz < TOTAL_OUTPUT_DEPTH)\n"
"            {\n"
"                int offset = convolved_image_offset + (outputY * output_width  + outputX) * TOTAL_OUTPUT_DEPTH + outputZ + outz;\n"
"                convolved_image.data[offset] = ACTIVATION_FUNCTION(sum[outz]);\n"
"            }\n"
"        }\n"
"        }\n"
"    }\n"
"}\n"
;

void GlesCsProgramManager::getProgNameDEPTHWISE_CONV_2D(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyDepthConv* key = reinterpret_cast<const GlesCsProgramKeyDepthConv*>(progKey);
    ASSERT(key->opType == OperationType::DEPTHWISE_CONV_2D);

    std::stringstream ss;
    ss << "optype" << (int)key->opType << "_"
       << "activation" << key->activation << "_"
       << "lsz("   << key->localSizeX << "," << key->localSizeY  << "," << key->localSizeZ << ")_"
       << "itemZ" << key->itemZ;
    name = ss.str();
}

void GlesCsProgramManager::getShaderSourceDEPTHWISE_CONV_2D(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyDepthConv* key = reinterpret_cast<const GlesCsProgramKeyDepthConv*>(progKey);
    ASSERT(key->opType == OperationType::DEPTHWISE_CONV_2D);

    std::stringstream ss;
    ss << "#version 320 es\n";
    ss << "#define LOCAL_SZ_X " << key->localSizeX << "\n";
    ss << "#define LOCAL_SZ_Y " << key->localSizeY << "\n";
    ss << "#define LOCAL_SZ_Z " << key->localSizeZ << "\n";
    ss << "#define ZPAR " << key->itemZ << "\n";

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
            break;
    }

    ss << mainpart;
    src = ss.str();
}

NAME_SPACE_STOP
