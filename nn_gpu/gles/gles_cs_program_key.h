#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_CS_PROGRAM_KEY_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_GLES_CS_PROGRAM_KEY_H

#include "base_executor.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

// should be the same as defined in types.hal
enum FusedActivationFunctionType { kNone, kRelu, kRelu1, kRelu6 };
enum ConvShaderType
{
    CONV_SHADER_TYPE_BASIC = 0,
    CONV_SHADER_TYPE_GEMM1,
    CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL,
    CONV_SHADER_TYPE_GEMM_4_4_GENERIC,
    CONV_SHADER_TYPE_GEMM_4_4_CHN3,
    CONV_SHADER_TYPE_GEMM_4_8_GENERIC,
    CONV_SHADER_TYPE_CHN3_TO_CHN4,
    CONV_SHADER_TYPE_NUM
};

struct GlesCsProgramKeyBasic
{
    GlesCsProgramKeyBasic(OperationType type)
    {
        localSizeX = 0;
        localSizeY = 0;
        localSizeZ = 0;
        opType = type;
    }
    OperationType opType;
    int32_t activation;
    uint32_t localSizeX;
    uint32_t localSizeY;
    uint32_t localSizeZ;
};

struct GlesCsProgramKeyAdd : GlesCsProgramKeyBasic
{
    GlesCsProgramKeyAdd() : GlesCsProgramKeyBasic(OperationType::ADD) {}
    bool broadcast = {false};
};

struct GlesCsProgramKeyConcatenation: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyConcatenation() : GlesCsProgramKeyBasic(OperationType::CONCATENATION) {}
    bool lastaxis = {false};
};

struct ConvParam
{
    ConvParam(int b,
              int inh, int inw, int inc,
              int outh, int outw, int outc,
              int filterh, int filterw,
              int strideh, int stridew,
              int padh, int padw,
              int activ, bool hasbias)
    {
        batch = b;
        inH = inh;
        inW = inw;
        inC = inc;
        outH = outh;
        outW = outw;
        outC = outc;
        filterH = filterh;
        filterW = filterw;
        strideH = strideh;
        strideW = stridew;
        padH = padh;
        padW = padw;
        activation = activ;
        hasBias = hasbias;
    }
    ConvParam(){}

    int batch = {0};
    int inH = {0};
    int inW = {0};
    int inC = {0};
    int outH = {0};
    int outW = {0};
    int outC = {0};
    int filterH = {0};
    int filterW = {0};
    int strideH = {0};
    int strideW = {0};
    int padH = {0};
    int padW = {0};
    int activation = {0};
    bool hasBias = {false};
};

struct GlesCsProgramKeyConv: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyConv() : GlesCsProgramKeyBasic(OperationType::CONV_2D) {}
    int blockHeight = {0};
    int blockWidth = {0};
    int blockDepth = {0};
    int shaderType = {0};
    ConvParam convParam;
};

struct GlesCsProgramKeyDepthConv: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyDepthConv() : GlesCsProgramKeyBasic(OperationType::DEPTHWISE_CONV_2D) {}
    uint32_t itemZ = {0};
};

struct GlesCsProgramKeyLRN: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyLRN() : GlesCsProgramKeyBasic(OperationType::LOCAL_RESPONSE_NORMALIZATION) {}
};

struct GlesCsProgramKeyAvgPool: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyAvgPool() : GlesCsProgramKeyBasic(OperationType::AVERAGE_POOL_2D) {}
    uint32_t itemZ = {0};
    uint32_t batch = {0};
};

struct GlesCsProgramKeyMaxPool: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyMaxPool() : GlesCsProgramKeyBasic(OperationType::MAX_POOL_2D) {}
    uint32_t itemZ = {0};
    uint32_t batch = {0};
};

struct GlesCsProgramKeyMul : GlesCsProgramKeyBasic
{
    GlesCsProgramKeyMul() : GlesCsProgramKeyBasic(OperationType::MUL) {}
    bool broadcast = {false};
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
