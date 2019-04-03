#include "gles_cs_program_manager.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {


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
"uniform int input_width;\n"
"uniform int input_height;\n"
"uniform int output_width;\n"
"uniform int output_height;\n"
"uniform int pad_w;\n"
"uniform int pad_h;\n"
"uniform int KERNEL_W;\n"
"uniform int KERNEL_H;\n"
"uniform int STRIDE_W;\n"
"uniform int STRIDE_H;\n"
"uniform int CHANNELS;\n"
"layout(binding = 0) readonly buffer Input0{\n"
"    float data[];\n"
"} image_data;\n"
"layout(binding = 1) writeonly buffer Output{\n"
"    float data[];\n"
"} convolved_image;\n"
"layout(local_size_x = LOCAL_SZ_X, local_size_y = LOCAL_SZ_Y, local_size_z = LOCAL_SZ_Z) in;\n"
"void main()\n"
"{\n"
"    int outputX = int(gl_GlobalInvocationID.x);\n"
"    int outputY = int(gl_GlobalInvocationID.y);\n"
"    int outputZ = int(gl_GlobalInvocationID.z) * ZPAR;\n"
"    if(outputX < output_width && outputY < output_height)\n"
"    {\n"
"        float sum[BATCH * ZPAR];\n"
"        for(int outz = int(0); outz < BATCH * ZPAR; outz++)\n"
"        {\n"
"            sum[outz] = 0.0f;\n"
"        }\n"
"        int org_y = outputY * STRIDE_H - pad_h;\n"
"        int org_x = outputX * STRIDE_W - pad_w;\n"
"        int input_image_size  = input_width * input_height * CHANNELS;\n"
"        int output_image_size = output_width * output_height * CHANNELS;\n"
"        int local_image_offset = (org_y*input_width + org_x) * CHANNELS + outputZ;\n"
"        int batch_offset = 0;\n"
"        for (int b = 0; b < BATCH; b++)\n"
"        {\n"
"            for(int y = 0; y < KERNEL_H; y++)\n"
"            {\n"
"                for(int x = 0; x < KERNEL_W; x++)\n"
"                {\n"
"                    if(org_y + y >= 0 && org_y + y < input_height && org_x + x >= 0 && org_x + x < input_width)\n"
"                    {\n"
"                        for(int outz =0; outz < ZPAR; outz++)\n"
"                        {\n"
"                            sum[b * ZPAR + outz] = max(sum[b * ZPAR + outz],  image_data.data[local_image_offset + outz]);\n"
"                        }\n"
"                    }\n"
"                    local_image_offset += CHANNELS;\n"
"                }\n"
"                local_image_offset += input_width * CHANNELS - CHANNELS * KERNEL_W;\n"
"            }\n"
"            local_image_offset += input_image_size - CHANNELS * KERNEL_W - input_width * KERNEL_H * CHANNELS;\n"
"        }\n"
"\n"
"        for (int b = 0; b < BATCH; b++)\n"
"        {\n"
"            for(int outz = 0; outz < ZPAR; outz++)\n"
"            {\n"
"                if (outputZ + outz < CHANNELS)\n"
"                {\n"
"                    int offset = batch_offset + (outputY * output_width  + outputX) * CHANNELS + outputZ + outz;\n"
"                    convolved_image.data[offset] = ACTIVATION_FUNCTION(sum[b * ZPAR + outz]);\n"
"                }\n"
"            }\n"
"            batch_offset += output_image_size;\n"
"        }\n"
"    }\n"
"}\n"
;

void GlesCsProgramManager::getProgNameMAX_POOL_2D(const void* progKey, std::string& name)
{
    const GlesCsProgramKeyMaxPool* key = reinterpret_cast<const GlesCsProgramKeyMaxPool*>(progKey);
    ASSERT(key->opType == OperationType::MAX_POOL_2D);

    std::stringstream ss;
    ss << "optype" << (int)key->opType << "_"
       << "activation" << key->activation << "_"
       << "lsz("  << key->localSizeX << "," << key->localSizeY  << "," << key->localSizeZ << ")_"
       << "itemZ" << key->itemZ << "_"
       << "batch" << key->batch;
    name = ss.str();
}

void GlesCsProgramManager::getShaderSourceMAX_POOL_2D(const void* progKey, std::string& src)
{
    const GlesCsProgramKeyMaxPool* key = reinterpret_cast<const GlesCsProgramKeyMaxPool*>(progKey);
    ASSERT(key->opType == OperationType::MAX_POOL_2D);

    std::stringstream ss;
    ss << "#version 320 es\n";
    ss << "#define LOCAL_SZ_X " << key->localSizeX << "\n";
    ss << "#define LOCAL_SZ_Y " << key->localSizeY << "\n";
    ss << "#define LOCAL_SZ_Z " << key->localSizeZ << "\n";
    ss << "#define ZPAR " << key->itemZ << "\n";
    ss << "#define BATCH " << key->batch<< "\n";

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

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
