#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GLES_CS_PROGRAM_KEY_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GLES_CS_PROGRAM_KEY_H

#include "base_executor.h"

NAME_SPACE_BEGIN

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
        size_t size = sizeof(struct GlesCsProgramKeyBasic);
        memset(this, 0xBB, size);
        opType = type;
    }
    OperationType opType;
    int32_t activation = {0};
    uint32_t localSizeX;
    uint32_t localSizeY;
    uint32_t localSizeZ;
};

struct GlesCsProgramKeyAdd : GlesCsProgramKeyBasic
{
    GlesCsProgramKeyAdd() : GlesCsProgramKeyBasic(OperationType::ADD), broadcast(false) {};
    bool broadcast;
};

struct GlesCsProgramKeyConcatenation: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyConcatenation() : GlesCsProgramKeyBasic(OperationType::CONCATENATION), lastaxis(false) {};
    bool lastaxis;
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
    };

    ConvParam(): batch(0), inH(0), inW(0), inC(0), outH(0), outW(0), outC(0),
        filterH(0), filterW(0), strideH(0), strideW(0), padH(0), padW(0), activation(0), hasBias(0)
    {};

    int batch;
    int inH;
    int inW;
    int inC;
    int outH;
    int outW;
    int outC;
    int filterH;
    int filterW;
    int strideH;
    int strideW;
    int padH;
    int padW;
    int activation;
    bool hasBias;
};

struct GlesCsProgramKeyConv: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyConv() :
        GlesCsProgramKeyBasic(OperationType::CONV_2D), blockHeight(0), blockWidth(0), blockDepth(0), shaderType(0)
    {};

    int blockHeight;
    int blockWidth;
    int blockDepth;
    int shaderType;
    ConvParam convParam;
};

struct GlesCsProgramKeyDepthConv: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyDepthConv() : GlesCsProgramKeyBasic(OperationType::DEPTHWISE_CONV_2D), itemZ(0) {};
    uint32_t itemZ;
};

struct GlesCsProgramKeyLRN: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyLRN() : GlesCsProgramKeyBasic(OperationType::LOCAL_RESPONSE_NORMALIZATION) {}
};

struct GlesCsProgramKeyAvgPool: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyAvgPool() : GlesCsProgramKeyBasic(OperationType::AVERAGE_POOL_2D),
        itemZ(0), batch(0)
    {};

    uint32_t itemZ;
    uint32_t batch;
};

struct GlesCsProgramKeyMaxPool: GlesCsProgramKeyBasic
{
    GlesCsProgramKeyMaxPool() : GlesCsProgramKeyBasic(OperationType::MAX_POOL_2D),
        itemZ(0), batch(0)
    {};

    uint32_t itemZ;
    uint32_t batch;
};

struct GlesCsProgramKeyMul : GlesCsProgramKeyBasic
{
    GlesCsProgramKeyMul() : GlesCsProgramKeyBasic(OperationType::MUL), broadcast(false) {};
    bool broadcast;
};

NAME_SPACE_STOP

#endif
