#include <math.h>
#include <cutils/properties.h>
#include "gles_cs_executor.h"

NAME_SPACE_BEGIN

#define OUTPUT_SIZE(convParam) \
    (convParam.batch * convParam.outH * convParam.outW * convParam.outC)
#define INPUT_SIZE(convParam) \
    (convParam.batch * convParam.inH * convParam.inW * convParam.inC)
#define FILTER_SIZE(convParam) \
    (convParam.outC * convParam.filterH * convParam.filterW * convParam.inC)

#include <sys/time.h>
#define TIMER_START(start) \
    struct timeval start; \
    gettimeofday(&start, 0);

#define TIMER_STOP(start) \
    ({ \
     struct timeval stop; \
     gettimeofday(&stop, 0); \
     long elapsed_us = (stop.tv_sec-start.tv_sec)*1000000 + stop.tv_usec-start.tv_usec; \
     elapsed_us; \
     })


typedef std::map<std::string, std::string> ShaderConfigMap;
typedef std::map<long, int> TimedConfig;
static ShaderConfigMap shaderConfigMap;
static bool inited = false;
static std::mutex mtx;
const char *prop_prefix = "persist.nn.gpgpu.shader.config.";
static GLint max_wg_count_x;
static GLint max_wg_count_y;
static GLint max_wg_count_z;
static GLint max_wg_size_x;
static GLint max_wg_size_y;
static GLint max_wg_size_z;
static GLint max_wg_invocations;
bool computeGroupParam(uint32_t totalThreadX,
                       uint32_t preferLocalSizeX,
                       uint32_t& localSizeX,
                       uint32_t& groupCountX);


struct ShaderConfig
{
    ShaderConfig(int type, int lx, int ly, int lz, int bx, int by, int bz)
    {
        shaderType = type;
        localSizeX  = lx;
        localSizeY  = ly;
        localSizeZ  = lz;
        blockWidth  = bx;
        blockHeight = by;
        blockDepth  = bz;
    };

    ShaderConfig(): localSizeX(0), localSizeY(0), localSizeZ(0),
        blockWidth(0), blockHeight(0), blockDepth(0), shaderType(0)
    {};

    int localSizeX;
    int localSizeY;
    int localSizeZ;
    int blockWidth;
    int blockHeight;
    int blockDepth;
    int shaderType;
};

// in(inH, inW, inC) out(outH, outW, outC)
// lsz(localSizeX, localSizeY, localSizeZ)
// block(blockWidth, blockHeight, blockDepth)
static const char* defaultConfig[] =
{
#ifdef TARGET_GORDON_PEAK
    /* inception-v3 */
    "optype3_batch1_in149_149_32_out147_147_32_filter3_3_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_168_1_block8_4_1",
    "optype3_batch1_in147_147_32_out147_147_64_filter3_3_pad1_1_stride1_1_activation1_bias1", "type5_lsz1_64_1_block8_4_1",
    "optype3_batch1_in73_73_64_out73_73_80_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in73_73_80_out71_71_192_filter3_3_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_72_1_block8_4_1",
    "optype3_batch1_in35_35_192_out35_35_64_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_104_1_block8_4_1",
    "optype3_batch1_in35_35_192_out35_35_48_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_64_1_block8_4_1",
    "optype3_batch1_in35_35_48_out35_35_64_filter5_5_pad2_2_stride1_1_activation1_bias1", "type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in35_35_64_out35_35_96_filter3_3_pad1_1_stride1_1_activation1_bias1", "type5_lsz1_88_1_block8_4_1",
    "optype3_batch1_in35_35_96_out35_35_96_filter3_3_pad1_1_stride1_1_activation1_bias1", "type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in35_35_192_out35_35_32_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_64_1_block8_4_1",
    "optype3_batch1_in35_35_256_out35_35_64_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_5_1_block8_4_1",
    "optype3_batch1_in35_35_256_out35_35_48_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in35_35_288_out35_35_64_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in35_35_288_out35_35_48_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_232_1_block8_4_1",
    "optype3_batch1_in35_35_288_out17_17_384_filter3_3_pad0_0_stride2_2_activation1_bias1", "type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in35_35_96_out17_17_96_filter3_3_pad0_0_stride2_2_activation1_bias1", "type5_lsz1_72_1_block8_4_1",
    "optype3_batch1_in17_17_768_out17_17_192_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in17_17_768_out17_17_128_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_6_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_128_filter1_7_pad0_3_stride1_1_activation1_bias1", "type5_lsz1_184_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_192_filter7_1_pad3_0_stride1_1_activation1_bias1", "type5_lsz1_184_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_128_filter7_1_pad3_0_stride1_1_activation1_bias1", "type5_lsz1_104_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_192_filter1_7_pad0_3_stride1_1_activation1_bias1", "type5_lsz1_72_1_block8_4_1",
    "optype3_batch1_in17_17_768_out17_17_160_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_160_filter1_7_pad0_3_stride1_1_activation1_bias1", "type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_192_filter7_1_pad3_0_stride1_1_activation1_bias1", "type5_lsz1_72_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_160_filter7_1_pad3_0_stride1_1_activation1_bias1", "type5_lsz1_88_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_192_filter1_7_pad0_3_stride1_1_activation1_bias1", "type5_lsz1_136_1_block8_4_1",
    "optype3_batch1_in17_17_192_out17_17_192_filter1_7_pad0_3_stride1_1_activation1_bias1", "type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in17_17_192_out17_17_192_filter7_1_pad3_0_stride1_1_activation1_bias1", "type5_lsz1_72_1_block8_4_1",
    "optype3_batch1_in17_17_192_out8_8_320_filter3_3_pad0_0_stride2_2_activation1_bias1", "type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in17_17_192_out8_8_192_filter3_3_pad0_0_stride2_2_activation1_bias1", "type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_320_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_152_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_384_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_64_1_block8_4_1",
    "optype3_batch1_in8_8_384_out8_8_384_filter1_3_pad0_1_stride1_1_activation1_bias1", "type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in8_8_384_out8_8_384_filter3_1_pad1_0_stride1_1_activation1_bias1", "type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_448_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_176_1_block8_4_1",
    "optype3_batch1_in8_8_448_out8_8_384_filter3_3_pad1_1_stride1_1_activation1_bias1", "type5_lsz1_48_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_192_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_7_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_320_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_112_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_384_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_48_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_448_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_240_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_192_filter1_1_pad0_0_stride1_1_activation1_bias1", "type5_lsz1_8_1_block8_4_1",
    "optype3_batch1_in1_1_2048_out1_1_1001_filter1_1_pad0_0_stride1_1_activation0_bias1", "type1_lsz4_64_1_block1_1_1",
    /* mobilenet */
    "optype3_batch1_in224_224_3_out112_112_32_filter3_3_pad0_0_stride2_2_activation3_bias1","type4_lsz4_64_1_block4_4_1",
    "optype3_batch1_in112_112_32_out112_112_64_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_64_1_block8_4_1",
    "optype3_batch1_in56_56_64_out56_56_128_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_96_1_block8_4_1",
    "optype3_batch1_in56_56_128_out56_56_128_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in28_28_128_out28_28_256_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_48_1_block8_4_1",
    "optype3_batch1_in28_28_256_out28_28_256_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in14_14_256_out14_14_512_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in14_14_512_out14_14_512_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in7_7_512_out7_7_1024_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_152_1_block8_4_1",
    "optype3_batch1_in7_7_1024_out7_7_1024_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_240_1_block8_4_1",
    "optype3_batch1_in1_1_1024_out1_1_1001_filter1_1_pad0_0_stride1_1_activation0_bias1","type1_lsz4_64_1_block1_1_1",
    /* resnet50 */
    "optype3_batch1_in224_224_3_out112_112_64_filter7_7_pad2_2_stride2_2_activation1_bias1","type4_lsz4_4_1_block4_4_1",
    "optype3_batch1_in56_56_64_out56_56_256_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in56_56_64_out56_56_64_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in56_56_64_out56_56_64_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_104_1_block8_4_1",
    "optype3_batch1_in56_56_256_out56_56_64_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in56_56_256_out28_28_512_filter1_1_pad0_0_stride2_2_activation0_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in56_56_256_out28_28_128_filter1_1_pad0_0_stride2_2_activation1_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in28_28_128_out28_28_128_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_32_1_block8_4_1",
    "optype3_batch1_in28_28_128_out28_28_512_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in28_28_512_out28_28_128_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in28_28_512_out14_14_1024_filter1_1_pad0_0_stride2_2_activation0_bias1","type5_lsz1_7_1_block8_4_1",
    "optype3_batch1_in28_28_512_out14_14_256_filter1_1_pad0_0_stride2_2_activation1_bias1","type5_lsz1_7_1_block8_4_1",
    "optype3_batch1_in14_14_256_out14_14_256_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_136_1_block8_4_1",
    "optype3_batch1_in14_14_256_out14_14_1024_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_32_1_block8_4_1",
    "optype3_batch1_in14_14_1024_out14_14_256_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_7_1_block8_4_1",
    "optype3_batch1_in14_14_1024_out7_7_2048_filter1_1_pad0_0_stride2_2_activation0_bias1","type5_lsz1_224_1_block8_4_1",
    "optype3_batch1_in14_14_1024_out7_7_512_filter1_1_pad0_0_stride2_2_activation1_bias1","type5_lsz1_168_1_block8_4_1",
    "optype3_batch1_in7_7_512_out7_7_512_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_208_1_block8_4_1",
    "optype3_batch1_in7_7_512_out7_7_2048_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_152_1_block8_4_1",
    "optype3_batch1_in7_7_2048_out7_7_512_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_224_1_block8_4_1",
    /* cts */
    "optype3_batch1_in1_1_3_out1_1_3_filter1_1_pad0_0_stride1_1_activation0_bias1", "type0_lsz16_1_1_block1_1_1",
    "optype3_batch1_in2_3_3_out2_3_3_filter1_1_pad0_0_stride1_1_activation0_bias1", "type0_lsz1_1_4_block1_1_1",
    "optype3_batch1_in3_3_1_out2_2_1_filter2_2_pad0_0_stride1_1_activation0_bias1", "type0_lsz1_1_4_block1_1_1",
    "optype3_batch1_in8_8_3_out8_8_1_filter3_2_pad1_0_stride1_1_activation0_bias1", "type0_lsz64_1_4_block1_1_1",
    "optype3_batch1_in8_8_3_out6_7_1_filter3_2_pad0_0_stride1_1_activation0_bias1", "type0_lsz4_4_1_block1_1_1",
    "optype3_batch1_in8_8_3_out8_8_3_filter3_2_pad1_0_stride1_1_activation0_bias1", "type0_lsz256_1_1_block1_1_1",
    "optype3_batch1_in8_8_3_out6_7_3_filter3_2_pad0_0_stride1_1_activation0_bias1", "type0_lsz1_1_1_block1_1_1",
    "optype3_batch1_in224_224_3_out112_112_16_filter3_3_pad0_0_stride2_2_activation3_bias1", "type4_lsz1_16_1_block4_4_1",
    "optype3_batch1_in112_112_16_out112_112_16_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_88_1_block8_4_1",
    "optype3_batch1_in56_56_16_out56_56_32_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_200_1_block8_4_1",
    "optype3_batch1_in56_56_32_out56_56_32_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_120_1_block8_4_1",
    "optype3_batch1_in28_28_32_out28_28_64_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_112_1_block8_4_1",
    "optype3_batch1_in28_28_64_out28_28_64_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_208_1_block8_4_1",
    "optype3_batch1_in14_14_64_out14_14_128_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_152_1_block8_4_1",
    "optype3_batch1_in14_14_128_out14_14_128_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_88_1_block8_4_1",
    "optype3_batch1_in7_7_128_out7_7_256_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in7_7_256_out7_7_256_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_176_1_block8_4_1",
    "optype3_batch1_in1_1_256_out1_1_11_filter1_1_pad0_0_stride1_1_activation0_bias1", "type1_lsz1_256_1_block1_1_1",
    /* channel 3 to channel 4 transformed*/
    "optype3_batch1_in224_224_4_out112_112_32_filter3_3_pad0_0_stride2_2_activation3_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in224_224_4_out112_112_64_filter7_7_pad2_2_stride2_2_activation1_bias1", "type5_lsz1_88_1_block8_4_1",
    "optype3_batch1_in1_1_4_out1_1_3_filter1_1_pad0_0_stride1_1_activation0_bias1", "type1_lsz4_4_1_block1_1_1",
    "optype3_batch1_in2_3_4_out2_3_3_filter1_1_pad0_0_stride1_1_activation0_bias1", "type1_lsz1_4_1_block1_1_1",
    "optype3_batch1_in8_8_4_out8_8_1_filter3_2_pad1_0_stride1_1_activation0_bias1", "type0_lsz4_1_16_block1_1_1",
    "optype3_batch1_in8_8_4_out6_7_1_filter3_2_pad0_0_stride1_1_activation0_bias1", "type0_lsz1_16_16_block1_1_1",
    "optype3_batch1_in8_8_4_out8_8_3_filter3_2_pad1_0_stride1_1_activation0_bias1", "type0_lsz1_4_4_block1_1_1",
    "optype3_batch1_in8_8_4_out6_7_3_filter3_2_pad0_0_stride1_1_activation0_bias1", "type0_lsz1_64_1_block1_1_1",
    "optype3_batch1_in224_224_4_out112_112_16_filter3_3_pad0_0_stride2_2_activation3_bias1", "type5_lsz1_152_1_block8_4_1",
#else
    /* inception-v3 */
    "optype3_batch1_in299_299_4_out149_149_32_filter3_3_pad0_0_stride2_2_activation1_bias1","type5_lsz1_56_1_block8_4_1",
    "optype3_batch1_in149_149_32_out147_147_32_filter3_3_pad0_0_stride1_1_activation1_bias1","type5_lsz1_88_1_block8_4_1",
    "optype3_batch1_in147_147_32_out147_147_64_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_48_1_block8_4_1",
    "optype3_batch1_in73_73_64_out73_73_80_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_224_1_block8_4_1",
    "optype3_batch1_in73_73_80_out71_71_192_filter3_3_pad0_0_stride1_1_activation1_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in35_35_192_out35_35_32_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_208_1_block8_4_1",
    "optype3_batch1_in35_35_192_out35_35_64_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_136_1_block8_4_1",
    "optype3_batch1_in35_35_64_out35_35_96_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in35_35_96_out35_35_96_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_216_1_block8_4_1",
    "optype3_batch1_in35_35_192_out35_35_48_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_208_1_block8_4_1",
    "optype3_batch1_in35_35_48_out35_35_64_filter5_5_pad2_2_stride1_1_activation1_bias1","type5_lsz1_248_1_block8_4_1",
    "optype3_batch1_in35_35_256_out35_35_64_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_176_1_block8_4_1",
    "optype3_batch1_in35_35_256_out35_35_48_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_200_1_block8_4_1",
    "optype3_batch1_in35_35_288_out35_35_64_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_120_1_block8_4_1",
    "optype3_batch1_in35_35_288_out35_35_48_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_224_1_block8_4_1",
    "optype3_batch1_in35_35_96_out17_17_96_filter3_3_pad0_0_stride2_2_activation1_bias1","type5_lsz1_6_1_block8_4_1",
    "optype3_batch1_in35_35_288_out17_17_384_filter3_3_pad0_0_stride2_2_activation1_bias1","type5_lsz1_176_1_block8_4_1",
    "optype3_batch1_in17_17_768_out17_17_192_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in17_17_768_out17_17_128_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_120_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_128_filter7_1_pad3_0_stride1_1_activation1_bias1","type5_lsz1_6_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_128_filter1_7_pad0_3_stride1_1_activation1_bias1","type5_lsz1_120_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_192_filter1_7_pad0_3_stride1_1_activation1_bias1","type5_lsz1_8_1_block8_4_1",
    "optype3_batch1_in17_17_128_out17_17_192_filter7_1_pad3_0_stride1_1_activation1_bias1","type5_lsz1_80_1_block8_4_1",
    "optype3_batch1_in17_17_768_out17_17_160_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_160_filter7_1_pad3_0_stride1_1_activation1_bias1","type5_lsz1_32_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_160_filter1_7_pad0_3_stride1_1_activation1_bias1","type5_lsz1_8_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_192_filter1_7_pad0_3_stride1_1_activation1_bias1","type5_lsz1_80_1_block8_4_1",
    "optype3_batch1_in17_17_160_out17_17_192_filter7_1_pad3_0_stride1_1_activation1_bias1","type5_lsz1_8_1_block8_4_1",
    "optype3_batch1_in17_17_192_out17_17_192_filter7_1_pad3_0_stride1_1_activation1_bias1","type5_lsz1_80_1_block8_4_1",
    "optype3_batch1_in17_17_192_out17_17_192_filter1_7_pad0_3_stride1_1_activation1_bias1","type5_lsz1_72_1_block8_4_1",
    "optype3_batch1_in17_17_192_out8_8_192_filter3_3_pad0_0_stride2_2_activation1_bias1","type5_lsz1_5_1_block8_4_1",
    "optype3_batch1_in17_17_192_out8_8_320_filter3_3_pad0_0_stride2_2_activation1_bias1","type5_lsz1_5_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_192_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_224_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_448_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in8_8_448_out8_8_384_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in8_8_384_out8_8_384_filter3_1_pad1_0_stride1_1_activation1_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in8_8_384_out8_8_384_filter1_3_pad0_1_stride1_1_activation1_bias1","type5_lsz1_8_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_384_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_56_1_block8_4_1",
    "optype3_batch1_in8_8_1280_out8_8_320_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_7_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_192_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_160_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_448_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_152_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_384_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in8_8_2048_out8_8_320_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in1_1_2048_out1_1_1001_filter1_1_pad0_0_stride1_1_activation0_bias1","type1_lsz1_16_1_block1_1_1",
    /* mobilenet */
    "optype3_batch1_in224_224_3_out112_112_32_filter3_3_pad0_0_stride2_2_activation3_bias1","type4_lsz4_64_1_block4_4_1",
    "optype3_batch1_in112_112_32_out112_112_64_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_80_1_block8_4_1",
    "optype3_batch1_in56_56_64_out56_56_128_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in56_56_128_out56_56_128_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_144_1_block8_4_1",
    "optype3_batch1_in28_28_128_out28_28_256_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_240_1_block8_4_1",
    "optype3_batch1_in28_28_256_out28_28_256_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_96_1_block8_4_1",
    "optype3_batch1_in14_14_256_out14_14_512_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_7_1_block8_4_1",
    "optype3_batch1_in14_14_512_out14_14_512_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_56_1_block8_4_1",
    "optype3_batch1_in7_7_512_out7_7_1024_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_112_1_block8_4_1",
    "optype3_batch1_in7_7_1024_out7_7_1024_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in1_1_1024_out1_1_1001_filter1_1_pad0_0_stride1_1_activation0_bias1","type1_lsz1_16_1_block1_1_1",
    /* resnet50 */
    "optype3_batch1_in224_224_3_out112_112_64_filter7_7_pad2_2_stride2_2_activation1_bias1","type4_lsz4_16_1_block4_4_1",
    "optype3_batch1_in56_56_64_out56_56_256_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_64_1_block8_4_1",
    "optype3_batch1_in56_56_64_out56_56_64_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_152_1_block8_4_1",
    "optype3_batch1_in56_56_64_out56_56_64_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_80_1_block8_4_1",
    "optype3_batch1_in56_56_256_out56_56_64_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in56_56_256_out28_28_512_filter1_1_pad0_0_stride2_2_activation0_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in56_56_256_out28_28_128_filter1_1_pad0_0_stride2_2_activation1_bias1","type5_lsz1_256_1_block8_4_1",
    "optype3_batch1_in28_28_128_out28_28_128_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in28_28_128_out28_28_512_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_8_1_block8_4_1",
    "optype3_batch1_in28_28_512_out28_28_128_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_88_1_block8_4_1",
    "optype3_batch1_in28_28_512_out14_14_1024_filter1_1_pad0_0_stride2_2_activation0_bias1","type5_lsz1_184_1_block8_4_1",
    "optype3_batch1_in28_28_512_out14_14_256_filter1_1_pad0_0_stride2_2_activation1_bias1","type5_lsz1_80_1_block8_4_1",
    "optype3_batch1_in14_14_256_out14_14_256_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in14_14_256_out14_14_1024_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_176_1_block8_4_1",
    "optype3_batch1_in14_14_1024_out14_14_256_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_8_1_block8_4_1",
    "optype3_batch1_in14_14_1024_out7_7_2048_filter1_1_pad0_0_stride2_2_activation0_bias1","type5_lsz1_32_1_block8_4_1",
    "optype3_batch1_in14_14_1024_out7_7_512_filter1_1_pad0_0_stride2_2_activation1_bias1","type5_lsz1_6_1_block8_4_1",
    "optype3_batch1_in7_7_512_out7_7_512_filter3_3_pad1_1_stride1_1_activation1_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in7_7_512_out7_7_2048_filter1_1_pad0_0_stride1_1_activation0_bias1","type5_lsz1_216_1_block8_4_1",
    "optype3_batch1_in7_7_2048_out7_7_512_filter1_1_pad0_0_stride1_1_activation1_bias1","type5_lsz1_40_1_block8_4_1",
    /* cts */
    "optype3_batch1_in1_1_3_out1_1_3_filter1_1_pad0_0_stride1_1_activation0_bias1","type0_lsz4_4_4_block1_1_1",
    "optype3_batch1_in2_3_3_out2_3_3_filter1_1_pad0_0_stride1_1_activation0_bias1","type0_lsz1_64_1_block1_1_1",
    "optype3_batch1_in3_3_1_out2_2_1_filter2_2_pad0_0_stride1_1_activation0_bias1","type0_lsz1_16_1_block1_1_1",
    "optype3_batch1_in8_8_3_out8_8_1_filter3_2_pad1_0_stride1_1_activation0_bias1","type0_lsz1_4_16_block1_1_1",
    "optype3_batch1_in8_8_3_out6_7_1_filter3_2_pad0_0_stride1_1_activation0_bias1","type0_lsz4_4_4_block1_1_1",
    "optype3_batch1_in8_8_3_out8_8_3_filter3_2_pad1_0_stride1_1_activation0_bias1","type0_lsz4_16_4_block1_1_1",
    "optype3_batch1_in8_8_3_out6_7_3_filter3_2_pad0_0_stride1_1_activation0_bias1","type0_lsz1_1_16_block1_1_1",
    "optype3_batch1_in224_224_3_out112_112_16_filter3_3_pad0_0_stride2_2_activation3_bias1","type4_lsz4_16_1_block4_4_1",
    "optype3_batch1_in112_112_16_out112_112_16_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_40_1_block8_4_1",
    "optype3_batch1_in56_56_16_out56_56_32_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_168_1_block8_4_1",
    "optype3_batch1_in56_56_32_out56_56_32_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_192_1_block8_4_1",
    "optype3_batch1_in28_28_32_out28_28_64_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_176_1_block8_4_1",
    "optype3_batch1_in28_28_64_out28_28_64_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_16_1_block8_4_1",
    "optype3_batch1_in14_14_64_out14_14_128_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_168_1_block8_4_1",
    "optype3_batch1_in14_14_128_out14_14_128_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_112_1_block8_4_1",
    "optype3_batch1_in7_7_128_out7_7_256_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_3_1_block8_4_1",
    "optype3_batch1_in7_7_256_out7_7_256_filter1_1_pad0_0_stride1_1_activation3_bias1","type5_lsz1_6_1_block8_4_1",
    "optype3_batch1_in1_1_256_out1_1_11_filter1_1_pad0_0_stride1_1_activation0_bias1","type1_lsz4_4_1_block1_1_1",
    /* channel 3 to channel 4 transformed*/
    "optype3_batch1_in1_1_4_out1_1_3_filter1_1_pad0_0_stride1_1_activation0_bias1", "type1_lsz64_1_1_block1_1_1",
    "optype3_batch1_in2_3_4_out2_3_3_filter1_1_pad0_0_stride1_1_activation0_bias1", "type1_lsz1_4_1_block1_1_1",
    "optype3_batch1_in8_8_4_out8_8_1_filter3_2_pad1_0_stride1_1_activation0_bias1", "type0_lsz16_16_1_block1_1_1",
    "optype3_batch1_in8_8_4_out6_7_1_filter3_2_pad0_0_stride1_1_activation0_bias1", "type0_lsz1_4_4_block1_1_1",
    "optype3_batch1_in8_8_4_out8_8_3_filter3_2_pad1_0_stride1_1_activation0_bias1", "type0_lsz4_64_1_block1_1_1",
    "optype3_batch1_in8_8_4_out6_7_3_filter3_2_pad0_0_stride1_1_activation0_bias1", "type0_lsz256_1_1_block1_1_1",
    "optype3_batch1_in224_224_4_out112_112_16_filter3_3_pad0_0_stride2_2_activation3_bias1", "type5_lsz1_248_1_block8_4_1",
    "optype3_batch1_in224_224_4_out112_112_64_filter7_7_pad2_2_stride2_2_activation1_bias1", "type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in224_224_4_out112_112_32_filter3_3_pad0_0_stride2_2_activation3_bias1", "type5_lsz1_56_1_block8_4_1",
    /* temporarily for unit test purpose */
    "optype3_batch1_in3_3_4_out3_3_1_filter1_1_pad0_0_stride1_1_activation0_bias1", "type1_lsz1_4_1_block1_1_1",
    "optype3_batch1_in8_8_4_out6_6_1_filter3_3_pad0_0_stride1_1_activation0_bias1", "type5_lsz4_16_1_block8_4_1",
    "optype3_batch1_in7_7_1024_out7_7_1_filter1_1_pad0_0_stride1_1_activation0_bias1", "type5_lsz1_1_1_block8_4_1",
    "optype3_batch1_in8_8_8_out6_6_8_filter3_3_pad0_0_stride1_1_activation0_bias1", "type5_lsz1_1_1_block8_4_1",
    "optype3_batch1_in7_7_8_out5_5_8_filter3_3_pad0_0_stride1_1_activation0_bias1", "type5_lsz1_1_1_block8_4_1",
    "optype3_batch1_in8_8_4_out6_6_8_filter3_3_pad0_0_stride1_1_activation0_bias1", "type5_lsz1_256_1_block8_4_1"

#endif
};

void string2Config(const char* confString, ShaderConfig &conf)
{
    sscanf(confString, "type%d_lsz%d_%d_%d_block%d_%d_%d",
           &conf.shaderType, &conf.localSizeX,  &conf.localSizeY, &conf.localSizeZ,
           &conf.blockWidth, &conf.blockHeight, &conf.blockDepth);
}

std::string genShaderConfigString(ShaderConfig &conf)
{
    std::stringstream ss;
    ss << "type"   << conf.shaderType << "_"
       << "lsz"   << conf.localSizeX << "_" << conf.localSizeY  << "_" << conf.localSizeZ << "_"
       << "block" << conf.blockWidth << "_" << conf.blockHeight << "_" << conf.blockDepth;
    return ss.str();
}

std::string genConvSignature(ConvParam &convParam)
{
    std::stringstream sig;
    sig << "optype"   << (int)OperationType::CONV_2D << "_"
        << "batch"   << convParam.batch << "_"
        << "in"      << convParam.inH  << "_" << convParam.inW  << "_" << convParam.inC << "_"
        << "out"     << convParam.outH << "_" << convParam.outW << "_" << convParam.outC << "_"
        << "filter"  << convParam.filterH << "_" << convParam.filterW << "_"
        << "pad"     << convParam.padH    << "_" << convParam.padW << "_"
        << "stride"  << convParam.strideH << "_" << convParam.strideW << "_"
        << "activation" << convParam.activation << "_"
        << "bias"  << convParam.hasBias;
    return sig.str();
}

bool computeGroupCount(ConvParam &convParam, ShaderConfig &conf, int &group_x, int &group_y, int &group_z)
{
    int M = convParam.outW * convParam.outH;
    int N = convParam.outC;

    if (conf.shaderType == CONV_SHADER_TYPE_BASIC)
    {
        group_x = ALIGN(ALIGN(N, conf.blockWidth) / conf.blockWidth, conf.localSizeX) / conf.localSizeX;
        group_y = ALIGN(ALIGN(M, conf.blockHeight) / conf.blockHeight, conf.localSizeY) / conf.localSizeY;
        group_z = ALIGN(ALIGN(convParam.batch, conf.blockDepth), conf.localSizeZ) / conf.localSizeZ;
    }
    else if (conf.shaderType == CONV_SHADER_TYPE_GEMM1)
    {
        ASSERT(conf.blockWidth == 1 && conf.blockHeight == 1 && conf.blockDepth == 1 && conf.localSizeZ == 1);
        group_x = ALIGN(N, conf.localSizeX) / conf.localSizeX;
        group_y = ALIGN(M, conf.localSizeY) / conf.localSizeY;
        group_z = convParam.batch;
    }
    else if (conf.shaderType == CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL)
    {
        ASSERT(conf.blockWidth == 4 && conf.blockHeight == 4 && conf.blockDepth == 1 && conf.localSizeZ == 1);
        group_x = ALIGN(N / 4, conf.localSizeX) / conf.localSizeX;
        group_y = ALIGN(ALIGN(M, 4) / 4, conf.localSizeY) / conf.localSizeY;
        group_z = convParam.batch;
    }
    else if (conf.shaderType == CONV_SHADER_TYPE_GEMM_4_4_GENERIC)
    {
        ASSERT(conf.blockWidth == 4 && conf.blockHeight == 4 && conf.blockDepth == 1 && conf.localSizeZ == 1);
        group_x = ALIGN(N / 4, conf.localSizeX) / conf.localSizeX;
        group_y = ALIGN(ALIGN(M, 4) / 4, conf.localSizeY) / conf.localSizeY;
        group_z = convParam.batch;
    }
    else if (conf.shaderType == CONV_SHADER_TYPE_GEMM_4_8_GENERIC)
    {
        ASSERT(conf.blockWidth == 8 && conf.blockHeight == 4 && conf.blockDepth == 1 && conf.localSizeZ == 1);
        group_x = ALIGN(N / conf.blockWidth, conf.localSizeX) / conf.localSizeX;
        group_y = ALIGN(ALIGN(M, 4) / 4, conf.localSizeY) / conf.localSizeY;
        group_z = convParam.batch;
    }
    else if (conf.shaderType == CONV_SHADER_TYPE_GEMM_4_4_CHN3)
    {
        ASSERT(M % 4 == 0 && conf.blockWidth == 4 && conf.blockHeight == 4 && conf.blockDepth == 1 && conf.localSizeZ == 1);
        group_x = ALIGN(N / 4, conf.localSizeX) / conf.localSizeX;
        group_y = ALIGN(M / 4, conf.localSizeY) / conf.localSizeY;
        group_z = convParam.batch;
    }
    else
    {
        NOT_REACH_HERE;
    }

    if(conf.localSizeX * conf.localSizeY * conf.localSizeZ < max_wg_invocations && 
       conf.localSizeX < max_wg_size_x &&
       conf.localSizeY < max_wg_size_y &&
       conf.localSizeZ < max_wg_size_z &&
       group_x < max_wg_count_x &&
       group_y < max_wg_count_y &&
       group_z < max_wg_count_z)
        return true;
    else
        return false;
}

std::vector<ShaderConfig> genShaderConfigBasic(ConvParam &convParam)
{
    int group_x, group_y, group_z;
    std::vector<ShaderConfig> candidates;

    for (int lx = 1; lx <= 256; lx *= 4)
    {
        for (int ly = 1; ly <= 256; ly *= 4)
        {
            for (int lz = 1; lz <= 32; lz *= 4)
            {
                ShaderConfig conf(CONV_SHADER_TYPE_BASIC, lx, ly, lz, 1, 1, 1);
                if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                    candidates.push_back(conf);
            }
        }
    }
    return candidates;
}

inline bool needImg2Col(ConvParam &convParam)
{
    return !(convParam.padH    == 0 && convParam.padW == 0 &&
             convParam.strideH == 1 && convParam.strideW == 1 &&
             convParam.filterH == 1 && convParam.filterW == 1);
}

std::vector<ShaderConfig> genShaderConfigCandidates(ConvParam &convParam, ConvShaderType type)
{
    int group_x, group_y, group_z;
    std::vector<ShaderConfig> candidates;
    int M = convParam.outH * convParam.outW;
    int K = convParam.filterH * convParam.filterW * convParam.inC;
    int N = convParam.outC;

    if (type == CONV_SHADER_TYPE_GEMM_4_4_GENERIC)
    {
        if (convParam.inC % 4 == 0 && N % 4 == 0)
        {
            ShaderConfig conf;
            conf.localSizeZ  = 1;
            conf.blockWidth  = 4;
            conf.blockHeight = 4;
            conf.blockDepth  = 1;
            conf.shaderType = CONV_SHADER_TYPE_GEMM_4_4_GENERIC;
            for (int lx = 1; lx <= 256; lx *= 4)
            {
                for (int ly = 1; ly <= 256; ly *= 4)
                {
                    conf.localSizeX = lx;
                    conf.localSizeY = ly;
                    if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                        candidates.push_back(conf);
                }
            }
        }
    }
    else if (type == CONV_SHADER_TYPE_GEMM_4_8_GENERIC)
    {
        if (convParam.inC % 4 == 0 && N % 8 == 0)
        {
            ShaderConfig conf;
            conf.localSizeX  = 1;
            conf.localSizeZ  = 1;
            conf.blockWidth  = 8;
            conf.blockHeight = 4;
            conf.blockDepth  = 1;
            conf.shaderType = CONV_SHADER_TYPE_GEMM_4_8_GENERIC;
            for (int ly = 1; ly < 8; ly++)
            {
                conf.localSizeY = ly;
                if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                    candidates.push_back(conf);
            }
            for (int ly = 8; ly <= 256; ly += 8)
            {
                conf.localSizeY = ly;
                if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                    candidates.push_back(conf);
            }
        }
    }
    else if (type == CONV_SHADER_TYPE_GEMM_4_4_CHN3)
    {
        if (convParam.inC % 4 != 0 && M % 4 == 0 && N % 4 == 0)
        {
            ShaderConfig conf;
            conf.localSizeZ  = 1;
            conf.blockWidth  = 4;
            conf.blockHeight = 4;
            conf.blockDepth  = 1;
            conf.shaderType = CONV_SHADER_TYPE_GEMM_4_4_CHN3;
            for (int lx = 1; lx <= 256; lx *= 4)
            {
                for (int ly = 1; ly <= 256; ly *= 4)
                {
                    conf.localSizeX = lx;
                    conf.localSizeY = ly;
                    if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                        candidates.push_back(conf);
                }
            }
        }
    }
    else if (type == CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL)
    {
        if (!needImg2Col(convParam))
        {
            ShaderConfig conf;
            if (K % 4 == 0 && N % 4 == 0)
            {
                conf.localSizeZ  = 1;
                conf.blockWidth  = 4;
                conf.blockHeight = 4;
                conf.blockDepth  = 1;
                conf.shaderType = CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL;
                for (int lx = 1; lx <= 256; lx *= 4)
                {
                    for (int ly = 1; ly <= 256; ly *= 4)
                    {
                        conf.localSizeX = lx;
                        conf.localSizeY = ly;
                        if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                            candidates.push_back(conf);
                    }
                }
            }
        }
    }
    else if (type == CONV_SHADER_TYPE_GEMM1)
    {
        if (!needImg2Col(convParam))
        {
            ShaderConfig conf;
            if (K % 4 == 0)
            {
                for (int lx = 1; lx <= 256; lx *= 4)
                {
                    for (int ly = 1; ly <= 256; ly *= 4)
                    {
                        conf.localSizeX  = lx;
                        conf.localSizeY  = ly;
                        conf.localSizeZ  = 1;
                        conf.blockWidth  = 1;
                        conf.blockHeight = 1;
                        conf.blockDepth  = 1;
                        {
                            conf.shaderType = CONV_SHADER_TYPE_GEMM1;
                        }
                        if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                            candidates.push_back(conf);
                    }
                }
            }
        }
    }
    else if (type == CONV_SHADER_TYPE_BASIC)
    {
        for (int lx = 1; lx <= 256; lx *= 4)
        {
            for (int ly = 1; ly <= 256; ly *= 4)
            {
                for (int lz = 1; lz <= 32; lz *= 4)
                {
                    ShaderConfig conf(CONV_SHADER_TYPE_BASIC, lx, ly, lz, 1, 1, 1);
                    if (computeGroupCount(convParam, conf, group_x, group_y, group_z))
                        candidates.push_back(conf);
                }
            }
        }
    }
    else
    {
        NOT_REACH_HERE;
    }

    return candidates;
}

void createSSBufferObject(GLuint& ssbo, GLuint count)
{
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(float), NULL, GL_STATIC_DRAW);

    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    float* p = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(float), bufMask);
    for (GLuint i = 0; i < count; ++i)
    {
        p[i] = 0.0f;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

bool chn3ToChn4(ConvParam& convParam,
                GlesCsProgramManager& progMgr,
                GLuint inBo,
                GLuint outBo,
                GLuint numItems)
{
    GLuint prog;
    uint32_t localSizeX = 16;
    uint32_t groupCountX;
#define ITEMS_PER_WI 16
    uint32_t totalThreadX = ALIGN(numItems, ITEMS_PER_WI) / ITEMS_PER_WI;
    computeGroupParam(totalThreadX, localSizeX, localSizeX, groupCountX);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inBo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, outBo);

    GlesCsProgramKeyConv key;
    key.activation  = 0;
    key.localSizeX  = localSizeX;
    key.localSizeY  = 1;
    key.localSizeZ  = 1;
    key.blockWidth  = ITEMS_PER_WI;
    key.blockHeight = 1;
    key.blockDepth  = 1;
    key.shaderType  = CONV_SHADER_TYPE_CHN3_TO_CHN4;
    key.convParam   = convParam;

    prog = progMgr.getProgram(&key);
    if (prog == 0)
    {
        LOGE("error: Failed to get program\n");
        return false;
    }
    glUseProgram(prog);

    GLint loc = glGetProgramResourceLocation(prog, GL_UNIFORM, "numItems");
    if (loc != -1)
    {
        glUniform1ui(loc, numItems);
    }
    else
    {
        LOGW("glGetProgramResourceLocation(\"num_threads\") returns -1");
        return false;
    }

    glDispatchCompute(groupCountX, 1, 1);
    CHECK_GL_STATE_RET();
}


bool convolve(ConvParam& convParam,
              ShaderConfig& shaderConfig,
              GlesCsProgramManager& progMgr)
{
    int group_x, group_y, group_z;
    GLuint prog;
    
    GlesCsProgramKeyConv key;
    key.activation  = convParam.activation;
    key.localSizeX  = shaderConfig.localSizeX;
    key.localSizeY  = shaderConfig.localSizeY;
    key.localSizeZ  = shaderConfig.localSizeZ;
    key.blockWidth  = shaderConfig.blockWidth;
    key.blockHeight = shaderConfig.blockHeight;
    key.blockDepth  = shaderConfig.blockDepth;
    key.shaderType  = shaderConfig.shaderType;
    key.convParam   = convParam;

    prog = progMgr.getProgram(&key);
    if (prog == 0)
    {
        LOGE("CONV_2D: failed to get program\n");
        return false;
    }

    glUseProgram(prog);
    computeGroupCount(convParam, shaderConfig, group_x, group_y, group_z);
    glDispatchCompute(group_x, group_y, group_z);
    CHECK_GL_STATE_RET();
}

bool convolveTimed(ConvParam& convParam,
                   ShaderConfig& shaderConfig,
                   GlesCsProgramManager& progMgr,
                   int iter,
                   long& elapsedTime,
                   bool syncPerIter)
{
    bool res = false;
    long t = 0.0;
    
    // warm up run
    if (!convolve(convParam, shaderConfig, progMgr))
        return false;
    glFinish();

    TIMER_START(convTime);
    for (int i = 0; i < iter; i++)
    {
        res = convolve(convParam, shaderConfig, progMgr);
        if (!res)
            break;
        if (syncPerIter)
            glFinish();
    }
    if (!syncPerIter)
        glFinish();
    t = TIMER_STOP(convTime) / iter;

    if (res)
        elapsedTime = t;
    else
        elapsedTime = std::numeric_limits<long>::max();

    return res;
}

void convOneBHWC(
     float* image_data,  int image_offset,
     float* kernel_data, int kernel_offset,
     float* bias,        int bias_offset,
     float* convolved_image,const int convolved_image_offset,
     const int input_width,
     const int input_height,
     const int output_width,
     const int output_height,
     const int padding_left,
     const int padding_top,
     bool has_bias,
     int STRIDE_W, int STRIDE_H,
     int KERNEL_W, int KERNEL_H,
     int DILATION_X, int DILATION_Y,
     int CHANNELS,
     int TOTAL_OUTPUT_DEPTH,
     int OUTPUT_Z,
     int outputX, int outputY, int depth, int activation)
{

    int ZPAR=1;
    if(outputX < output_width && outputY < output_height)
    {
        float sum[ZPAR];
        for(int kern =0; kern < ZPAR; kern++)
        {
            sum[kern] = 0.0f;
        }

        const int org_y = outputY * STRIDE_H - padding_top;
        const int org_x = outputX * STRIDE_W - padding_left;
        const int currentKernelOffset = kernel_offset + depth*KERNEL_H*KERNEL_W*CHANNELS;
        const int biasIndex=bias_offset + (depth % TOTAL_OUTPUT_DEPTH);
        const int local_image_offset = (org_y*input_width + org_x) * CHANNELS;

        float* image_dataPtrFloat = (image_data + (image_offset + local_image_offset));
        float* kernel_dataPtrFloat = (kernel_data + (currentKernelOffset));
        for(int y = 0; y < KERNEL_H; y++)
        {
            for(int x = 0; x < KERNEL_W; x++)
            {
                if(org_y + int(y * DILATION_Y) >= 0 && org_y + int(y * DILATION_Y) < int(input_height) && org_x + int(x * DILATION_X) >= 0 && org_x + int(x * DILATION_X) < int(input_width))
                {
                    for(int c = 0; c < CHANNELS; c++)
                    {
                        for(int outz = 0; outz < ZPAR; outz++)
                        {
                            sum[outz] += image_dataPtrFloat[c] * kernel_dataPtrFloat[outz*KERNEL_H*KERNEL_W*CHANNELS + c];
                        }
                    }
                }
                image_dataPtrFloat += DILATION_X * CHANNELS;
                kernel_dataPtrFloat += CHANNELS;
            }
            image_dataPtrFloat += input_width * DILATION_Y * CHANNELS - DILATION_X * CHANNELS * KERNEL_W;
        }

        if (has_bias)
        {
            for(int kern = 0; kern < ZPAR; kern++)
            {
                if(depth+kern < OUTPUT_Z)
                {
                    int offset = convolved_image_offset + (outputY * output_width  + outputX) * TOTAL_OUTPUT_DEPTH + depth + kern;
                    float out = sum[kern] + bias[biasIndex +kern];
                    if (activation == kRelu)
                    {
                        out = out > 0.f ? out : 0.f; 
                    }
                    else if (activation == kRelu1)
                    {
                        out = out > 1.f ? 1.f : out < -1.f ? -1.f : out; 
                    }
                    else if (activation == kRelu6)
                    {
                        out = out > 6.f ? 6.f : out < 0.f ? 0.f : out; 
                    }
                    convolved_image[offset] = out;
                }
            }
        }
        else
        {
            for(int kern = 0; kern < ZPAR; kern++)
            {
                if(depth+kern < OUTPUT_Z)
                {
                    int offset = convolved_image_offset + (outputY * output_width  + outputX) * TOTAL_OUTPUT_DEPTH + depth + kern;
                    float out = sum[kern];
                    if (activation == kRelu)
                    {
                        out = out > 0.f ? out : 0.f; 
                    }
                    else if (activation == kRelu1)
                    {
                        out = out > 1.f ? 1.f : out < -1.f ? -1.f : out; 
                    }
                    else if (activation == kRelu6)
                    {
                        out = out > 6.f ? 6.f : out < 0.f ? 0.f : out; 
                    }
                    convolved_image[offset] = out;
                }
            }
        }
    }
}

bool convCpuBHWC(float *in_buffer, float *bias_buffer, float *filter_buffer, float *benchmark,
            int batch, int group, int has_bias,
            int input_chn, int input_width, int input_height,
            int output_chn, int output_width, int output_height,
            int filter_width, int filter_height,
            int padding_left, int padding_top,
            int stride_width, int stride_height,
            int dilation_x, int dilation_y, int activation)
{
    int image_offset = 0;
    int bias_offset = 0;
    int kernel_offset = 0;
    int output_image_offset = 0;
    int M_ = output_chn / group;
    int bottom_dim = input_chn * input_width * input_height;
    int top_dim = output_chn * output_width * output_height;

    for (int n = 0; n < batch; ++n) {
        for (int g = 0; g < group; ++g) {
            image_offset  = n * bottom_dim + input_width * input_height * (input_chn / group) * g;
            kernel_offset = filter_height * filter_width* (input_chn / group) * M_ * g;
            bias_offset   = M_ * g;
            output_image_offset = n * top_dim + output_width * output_height * M_ * g;

            for (int depth = 0; depth < M_; ++depth) {
                for (int outputY = 0; outputY < output_height; ++outputY) {
                    for (int outputX = 0; outputX < output_width; ++outputX) {
                        convOneBHWC(
                                in_buffer,  image_offset,
                                filter_buffer, kernel_offset,
                                bias_buffer, bias_offset,
                                benchmark, output_image_offset,
                                input_width,
                                input_height,
                                output_width,
                                output_height,
                                padding_left,
                                padding_top,
                                has_bias,
                                stride_width, stride_height,
                                filter_width, filter_height,
                                dilation_x, dilation_y,
                                input_chn,
                                output_chn,
                                M_,
                                outputX, outputY, depth,
                                activation);
                    }
                }
            }
        }
    }
    return true;
}

void resetOutput(ConvParam &convParam, GLuint output)
{
    glFinish();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, output);
    int output_size = OUTPUT_SIZE(convParam);
    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    float* pOut = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, output_size * sizeof(float), bufMask);
    for(int i = 0; i < output_size; i++)
        pOut[i] = 7.28f;
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

bool verifyResult(ConvParam &convParam, float* in_buffer, float* filter_buffer, float* bias_buffer, float* result_buffer)
{
    int batch = convParam.batch;
    int input_chn = convParam.inC;
    int input_width  = convParam.inW;
    int input_height = convParam.inH;
    int output_chn = convParam.outC;
    int output_width = convParam.outW;
    int output_height = convParam.outH;
    int filter_height = convParam.filterH;
    int filter_width = convParam.filterW;
    int stride_height = convParam.strideH;
    int stride_width = convParam.strideW;
    int padding_top = convParam.padH;
    int padding_left = convParam.padW;
    bool has_bias = convParam.hasBias;
    int activation = convParam.activation;
    int dilation_y = 1;
    int dilation_x = 1;
    int group = 1;

    const GLuint output_size = OUTPUT_SIZE(convParam);
    float *benchmark = new float[output_size];
    convCpuBHWC(in_buffer, bias_buffer, filter_buffer, benchmark,
                batch, group, has_bias,
                input_chn, input_width, input_height,
                output_chn, output_width, output_height,
                filter_width, filter_height,
                padding_left, padding_top,
                stride_width, stride_height,
                dilation_x, dilation_y, activation);

    float* pOut = result_buffer;
    for (int b = 0; b < batch; b++)
    {
        for (int h = 0; h < output_height; h++)
        {
            for (int w = 0; w < output_width; w++)
            {
                for (int c = 0; c < output_chn; c++)
                {
                    int offset = b * (output_chn * output_height * output_width) + h * (output_width * output_chn) + w * output_chn + c;

                    if (fabs(pOut[offset] - benchmark[offset]) > 0.1 * fabs(benchmark[offset]) ||
                        !(fabs(benchmark[offset]) < 1.e-3 && fabs(pOut[offset] - benchmark[offset]) < 1.e-4))
                    {
                        LOGE("CONV_2D: convolution verification failed at (%d, %d, %d, %d), actual: %f, expected: %f\n",
                              b, h, w, c, pOut[offset], benchmark[offset]);
                        delete[] benchmark;
                        return false;
                    }
                }
            }
        }
    }
    delete[] benchmark;
    return true;
}

bool verifyShader(ConvParam &convParam, ShaderConfig &shaderConfig, GlesCsProgramManager& progMgr,
                  GLuint input, GLuint filter, GLuint bias, GLuint output)
{
    bool succeed;
    if (!convolve(convParam, shaderConfig, progMgr))
        return false;
    glFinish();

    float* p;
    int output_size = OUTPUT_SIZE(convParam);
    int input_size =  INPUT_SIZE(convParam);
    int filter_size = FILTER_SIZE(convParam);
    int bias_size = convParam.outC;
    float* out_buffer = new float[output_size];
    float* in_buffer = new float[input_size];
    float* filter_buffer = new float[filter_size];
    float* bias_buffer = new float[bias_size];

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, input);
    p = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, input_size * sizeof(float), GL_MAP_READ_BIT);
    memcpy(in_buffer, p, input_size * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, output);
    p = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, output_size * sizeof(float), GL_MAP_READ_BIT);
    memcpy(out_buffer, p, output_size * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, filter);
    p = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, filter_size * sizeof(float), GL_MAP_READ_BIT);
    memcpy(filter_buffer, p, filter_size * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bias);
    p = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bias_size * sizeof(float), GL_MAP_READ_BIT);
    memcpy(bias_buffer, p, bias_size * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    succeed = verifyResult(convParam, in_buffer, filter_buffer, bias_buffer, out_buffer);

    delete[] in_buffer;
    delete[] filter_buffer;
    delete[] bias_buffer;
    delete[] out_buffer;
    return succeed;
}

bool tryShaderConfig(ConvParam& convParam,
                     ShaderConfig& best,
                     GlesCsProgramManager& progMgr,
                     GLuint input,
                     GLuint filter,
                     GLuint bias,
                     GLuint output,
                     std::vector<ShaderConfig>& configs)
{
    TimedConfig timedConfig;
    std::string name;
    GlesCsProgramKeyConv key;
    bool ret = false;
    
    key.activation  = convParam.activation;
    key.convParam   = convParam;
    for (size_t i = 0; i < configs.size(); i ++)
    {
        long elapsedUS;
        bool ret = convolveTimed(convParam, configs[i], progMgr, 1, elapsedUS, true);
        // delete temporary program in time to save run time memory.
        key.localSizeX  = configs[i].localSizeX;
        key.localSizeY  = configs[i].localSizeY;
        key.localSizeZ  = configs[i].localSizeZ;
        key.blockWidth  = configs[i].blockWidth;
        key.blockHeight = configs[i].blockHeight;
        key.blockDepth  = configs[i].blockDepth;
        key.shaderType  = configs[i].shaderType;
        progMgr.getProgName(&key, name);
        progMgr.deleteProgram(name);
        if (ret)
        {
            NN_GPU_PERF("CONV_2D: %s: tune: %8.3f ms, %s\n", __func__, 1.0 * elapsedUS / 1000, name.c_str());
            std::pair<long, int> entry(elapsedUS, i);
            timedConfig.insert(entry);
        }
    }

    for(TimedConfig::iterator it = timedConfig.begin(); it != timedConfig.end(); ++it)
    {
        ShaderConfig cand = configs[it->second];
        key.localSizeX  = cand.localSizeX;
        key.localSizeY  = cand.localSizeY;
        key.localSizeZ  = cand.localSizeZ;
        key.blockWidth  = cand.blockWidth;
        key.blockHeight = cand.blockHeight;
        key.blockDepth  = cand.blockDepth;
        key.shaderType  = cand.shaderType;
        progMgr.getProgName(&key, name);

        resetOutput(convParam, output);
        if (verifyShader(convParam, cand, progMgr, input, filter, bias, output))
        {
            best = cand;
            ret = true;
            NN_GPU_PERF("CONV_2D: %s: tune: Best shader config: %.2f ms, %s\n",
                    __func__, 1.0 * it->first / 1000, name.c_str());
            break;
        }
        else
        {
            progMgr.deleteProgram(name);
        }
    }
    return ret;
}


void tune(ConvParam& convParam,
          ShaderConfig& conf,
          GlesCsProgramManager& progMgr,
          GLuint input,
          GLuint filter,
          GLuint bias,
          GLuint output)
{
    bool succeed = false;
    std::vector<ShaderConfig> configs;
    std::vector<ShaderConfig> more;

    configs = genShaderConfigCandidates(convParam, CONV_SHADER_TYPE_GEMM_4_8_GENERIC);
    more = genShaderConfigCandidates(convParam, CONV_SHADER_TYPE_GEMM_4_4_CHN3);
    configs.insert(configs.end(), more.begin(), more.end());
    succeed = tryShaderConfig(convParam, conf, progMgr, input, filter, bias, output, configs);

    if (!succeed)
    {
        configs = genShaderConfigCandidates(convParam, CONV_SHADER_TYPE_GEMM_4_4_GENERIC);
        more = genShaderConfigCandidates(convParam, CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL);
        configs.insert(configs.end(), more.begin(), more.end());
        succeed = tryShaderConfig(convParam, conf, progMgr, input, filter, bias, output, configs);
    }

    if (!succeed)
    {
        configs = genShaderConfigCandidates(convParam, CONV_SHADER_TYPE_GEMM1);
        succeed = tryShaderConfig(convParam, conf, progMgr, input, filter, bias, output, configs);
    }

    if (!succeed)
    {
        std::string sig = genConvSignature(convParam);
        NN_GPU_PERF("CONV_2D: %s: %s fallback to basic shader, THIS MAY HAVE POOR PERFORMANCE !\n", __func__, sig.c_str());
        configs = genShaderConfigCandidates(convParam, CONV_SHADER_TYPE_BASIC);
        succeed = tryShaderConfig(convParam, conf, progMgr, input, filter, bias, output, configs);
    }

    ASSERT(succeed);
    return;
}

bool storeConfig(std::string& signature, ShaderConfig& conf)
{
    bool ret;
    std::stringstream ss;

    ss << prop_prefix << signature;
    std::string confString = genShaderConfigString(conf);
    ret = property_set(ss.str().c_str(), confString.c_str()) == 0 ? true : false;
    NN_GPU_PERF("CONV_2D: %s: store shader config %s: %s, %s\n",
          __func__,
          ret ? "succeed" : "failed",
          ss.str().c_str(), confString.c_str());
    return ret;
}

bool loadConfig(std::string signature, ShaderConfig& conf)
{
    bool found = false;
    char prop[PROPERTY_VALUE_MAX];
    std::stringstream ss;

    ss << prop_prefix << signature;
    int ret = property_get(ss.str().c_str(), prop, "0");
    if (ret > 1)
    {
        found = true;
        string2Config(prop, conf);
        NN_GPU_PERF("CONV_2D: %s: %s, %s\n", __func__, ss.str().c_str(), prop);
    }
    return found;
}

void prepareShaderConfig(ConvParam& convParam,
                         ShaderConfig& conf,
                         GlesCsProgramManager& progMgr,
                         GLuint input,
                         GLuint filter,
                         GLuint bias,
                         GLuint output)
{
    std::string sig = genConvSignature(convParam);

    bool tuned = false;
    bool found = false;
    mtx.lock();
    // load default configs and get gl info
    if (!inited)
    {
        int configNum = 0;
        if (sizeof(defaultConfig) > 0)
            configNum = sizeof(defaultConfig) / sizeof(defaultConfig[0]) / 2;
        for (int i = 0; i < configNum; i++)
        {
            std::pair<std::string, std::string> entry(defaultConfig[2 * i], defaultConfig[2 * i + 1]);
            shaderConfigMap.insert(entry);
            NN_GPU_PERF("CONV_2D: %s: load pre-tuned config: %s, %s\n", __func__, defaultConfig[2 * i], defaultConfig[2 * i + 1]);
        }
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_wg_count_x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &max_wg_count_y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &max_wg_count_z);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_wg_size_x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_wg_size_y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_wg_size_z);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_wg_invocations);
        inited = true;
    }

    // search in-memory cache
    ShaderConfigMap::iterator it = shaderConfigMap.find(sig);
    if (it != shaderConfigMap.end())
    {
        NN_GPU_PERF("CONV_2D: %s: found config %s, %s\n", __func__, sig.c_str(), it->second.c_str());
        string2Config(it->second.c_str(), conf);
        mtx.unlock();
        return;
    }

    // load from persistent storage
    found = loadConfig(sig, conf);
    if (!found)
    {
        tune(convParam, conf, progMgr, input, filter, bias, output);
        tuned = true;
    }

    std::pair<std::string, std::string> entry(sig, genShaderConfigString(conf));
    shaderConfigMap.insert(entry);
    NN_GPU_PERF("CONV_2D: %s: cache config in memory: %s, %s\n", __func__, sig.c_str(), genShaderConfigString(conf).c_str());

    if (tuned)
    {
        storeConfig(sig, conf);
    }
    mtx.unlock();

    return;
}

// FIXME:
// Android NN don't set group, dilation, has_bias,
// so make these assumptions: group = 1, dilation = 1, has_bias = 1
bool GlesCsExecutor::doCONV_2D(const Operation& operation, GlesOperationResource& resource)
{
    int32_t input_width, input_height;
    int32_t output_width, output_height;
    int32_t input_chn, output_chn;
    int32_t filter_width, filter_height;
    int32_t padding_left, padding_right, padding_top, padding_bottom;
    int32_t stride_width, stride_height;
    int32_t activation;
    int32_t batch;
    int32_t has_bias = 1;

    ASSERT(operation.type == OperationType::CONV_2D);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;
    const size_t inCount = ins.size();
    ASSERT(inCount == 10 || inCount == 7);

    GlesOperand& input  = operands[ins[0]];
    GlesOperand& filter = operands[ins[1]];
    GlesOperand& bias   = operands[ins[2]];
    GlesOperand& output = operands[outs[0]];

    batch         = input.getDimensionSize(0);
    input_height  = input.getDimensionSize(1);
    input_width   = input.getDimensionSize(2);
    input_chn     = input.getDimensionSize(3);
    output_height = output.getDimensionSize(1);
    output_width  = output.getDimensionSize(2);
    output_chn    = output.getDimensionSize(3);
    filter_height = filter.getDimensionSize(1);
    filter_width  = filter.getDimensionSize(2);

    if (inCount == 10) {
        padding_left     = operands[ins[3]].getScalarData<int32_t>();
        padding_right    = operands[ins[4]].getScalarData<int32_t>();
        padding_top      = operands[ins[5]].getScalarData<int32_t>();
        padding_bottom   = operands[ins[6]].getScalarData<int32_t>();
        stride_width     = operands[ins[7]].getScalarData<int32_t>();
        stride_height    = operands[ins[8]].getScalarData<int32_t>();
        activation       = operands[ins[9]].getScalarData<int32_t>();
    } else {
        int32_t padding_implicit = operands[ins[3]].getScalarData<int32_t>();
        stride_width     = operands[ins[4]].getScalarData<int32_t>();
        stride_height    = operands[ins[5]].getScalarData<int32_t>();
        activation       = operands[ins[6]].getScalarData<int32_t>();
        calculateExplicitPadding(input_width, stride_width,
                                 filter_width, padding_implicit,
                                 &padding_left, &padding_right);
        calculateExplicitPadding(input_height, stride_height,
                                 filter_height, padding_implicit,
                                 &padding_top, &padding_bottom);
    }

    ConvParam convParam(batch, input_height, input_width, input_chn, output_height, output_width, output_chn,
                        filter_height, filter_width, stride_height, stride_width, padding_top, padding_left,
                        activation, has_bias);

    if (input.getType() == OperandType::TENSOR_FLOAT32)
    {
        ShaderConfig shaderConf;
        GLuint inSSbo, filterSSbo, biasSSbo, outSSbo;
        bool needSync = false;

        if (convParam.inC == 3)
        {
            GLuint imageBoChn4, filterBoChn4;
            GLuint imageBoSize, filterBoSize;
            imageBoSize  = INPUT_SIZE(convParam) / 3 * 4;
            filterBoSize = FILTER_SIZE(convParam)/ 3 * 4;

            if (resource.tmpBo.size() == 0)
            {
                createSSBufferObject(imageBoChn4, imageBoSize);
                createSSBufferObject(filterBoChn4, filterBoSize);
                resource.tmpBo.push_back(imageBoChn4);
                resource.tmpBo.push_back(filterBoChn4);
                chn3ToChn4(convParam, progMgr, filter.getSSbo(), filterBoChn4, FILTER_SIZE(convParam));

#if 0
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, filterBoChn4);
                static uint32_t file_no = 0;
                std::string file = std::string("/data/") + std::string("gles_") + std::string("filter")
                                        + std::to_string(file_no) + ".txt";
                FILE* file_ptr = fopen(file.c_str(), "w+");

                file_no++;

                if (file_ptr == nullptr)
                {
                    NN_GPU_DEBUG("create file %s failed", file.c_str());
                }
                else
                {

                    const size_t f_len = 4 * output_chn * filter_height * filter_width;
                    const float* fp = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, f_len, GL_MAP_READ_BIT);
                    int cur_c = 1;

                    NN_GPU_DEBUG("%s: dumpped file length is %d", __func__, f_len);

                    for (size_t i = 0; i < f_len; i++)
                    {
                        fprintf(file_ptr, "%f,", fp[i]);
                        if (cur_c % 4 == 0) {
                            fprintf(file_ptr, "\n");
                        }
                        ++cur_c;
                    }
                    fclose(file_ptr);

                    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
                }
#endif
            }

            imageBoChn4 = resource.tmpBo[0];
            filterBoChn4 = resource.tmpBo[1];
            chn3ToChn4(convParam, progMgr, input.getSSbo(), imageBoChn4, INPUT_SIZE(convParam));
            glFinish();

#if 0
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, imageBoChn4);
            static uint32_t file_no = 0;
            std::string file = std::string("/data/") + std::string("gles_") + std::string("input")
                                        + std::to_string(file_no) + ".txt";
            FILE* file_ptr = fopen(file.c_str(), "w+");
            file_no++;

            if (file_ptr == nullptr)
            {
                NN_GPU_DEBUG("create file %s failed", file.c_str());
            }
            else
            {
                const size_t f_len = input_height * input_width * 4;
                const float* fp = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, f_len, GL_MAP_READ_BIT);

                int cur_c = 1;
                NN_GPU_DEBUG("%s: dumpped file length is %d", __func__, f_len);

                for (size_t i = 0; i < f_len; i++)
                {
                    fprintf(file_ptr, "%f,", fp[i]);
                    if (cur_c % 4 == 0) {
                        fprintf(file_ptr, "\n");
                    }
                    ++cur_c;
                }
                fclose(file_ptr);
                glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            }
#endif

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, imageBoChn4);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, filterBoChn4);
            inSSbo = imageBoChn4;
            filterSSbo = filterBoChn4;
            convParam.inC = 4;
            needSync = true;
        }
        else
        {
            bindOperand(input,  0);
            bindOperand(filter, 2);
            inSSbo = input.getSSbo();
            filterSSbo = filter.getSSbo();
        }
        bindOperand(bias,   1);
        bindOperand(output, 3);
        biasSSbo = bias.getSSbo();
        outSSbo = output.getSSbo();

        prepareShaderConfig(convParam, shaderConf, progMgr, inSSbo, filterSSbo, biasSSbo, outSSbo);

        NN_GPU_DEBUG("convParam batch %d, input_height %d, input_width %d, input_chn %d, output_height %d, output_width %d, "
                "output_chn %d, filter_height %d, filter_width %d, stride_height %d, stride_width %d, padding_height %d, "
                "padding_width %d, activation %d, has_bias %d",
                convParam.batch, convParam.inH, convParam.inW, convParam.inC, convParam.outH, convParam.outW,
                convParam.outC, convParam.filterH, convParam.filterW, convParam.strideH, convParam.strideW,
                convParam.padH, convParam.padW, convParam.activation, convParam.hasBias);

        convolve(convParam, shaderConf, progMgr);
        if (needSync)
            glFinish();

        // output.dumpToFile("out", convParam.outC);
        output.dump();
    }
    else
    {
        NOT_IMPLEMENTED;
    }

    return true;
}

NAME_SPACE_STOP
