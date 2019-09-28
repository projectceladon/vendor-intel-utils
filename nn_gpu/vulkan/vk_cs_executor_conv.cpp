/*
 * Copyright @2017 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <math.h>
#include <cutils/properties.h>
#include "gpu_executor.h"
#include "vk_common.h"
#include "vk_cs_executor.h"
#include "shader/spv_shader.h"

NAME_SPACE_BEGIN

// TODO: query group count from vulkan device
#define MAX_GROUP_COUNT_X 65535
#define MAX_GROUP_COUNT_Y 65535
#define MAX_GROUP_COUNT_Z 65535

#define MAX_GROUP_SIZE_X     896
#define MAX_GROUP_SIZE_Y     896
#define MAX_GROUP_SIZE_Z     896
#define MAX_GROUP_INVOCATION 896

#define SPEC_CONST_NUM 21
#define ITEMS_PER_WI 16

#include <sys/time.h>

#define TIMER_START(start)        \
    struct timeval start;         \
    gettimeofday(&start, 0);

#define TIMER_STOP(start)                                                               \
    ({                                                                                  \
     struct timeval stop;                                                               \
     gettimeofday(&stop, 0);                                                            \
     long elapsed_us = (stop.tv_sec-start.tv_sec)*1000000 + stop.tv_usec-start.tv_usec; \
     elapsed_us;                                                                        \
     })

enum ConvShaderType
{
    CONV_SHADER_TYPE_BASIC               = 0,
    CONV_SHADER_TYPE_GEMM1               = 1,
    CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL = 2,
    CONV_SHADER_TYPE_GEMM_4_4_GENERIC    = 3,
    CONV_SHADER_TYPE_GEMM_4_4_CHN3       = 4,
    CONV_SHADER_TYPE_GEMM_4_8_GENERIC    = 5,
    CONV_SHADER_TYPE_CHN3_TO_CHN4        = 6,
    CONV_SHADER_TYPE_NUM                 = 7
};

enum FusedActivationFunctionType { kNone, kRelu, kRelu1, kRelu6 };

struct PushConst {
public:
    PushConst() {};
};


using ShaderConfigPair = std::pair<std::string, std::string>;
using ShaderConfigMap  = std::map<std::string, std::string>;
using TuningTimeMap    = std::map<long, int>;

static std::mutex mtx;
static ShaderConfigMap shaderConfigMap;
static bool is_initialized = false;
static int tmpBoSize = 0;
static int shader_type = CONV_SHADER_TYPE_BASIC;
static bool converted_to_chn4 = false;

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
    "optype3_batch1_in7_7_1024_out7_7_1024_filter1_1_pad0_0_stride1_1_activation3_bias1", "type5_lsz1_24_1_block8_4_1",
    "optype3_batch1_in8_8_8_out6_6_8_filter3_3_pad0_0_stride1_1_activation0_bias1", "type5_lsz1_1_1_block8_4_1",
    "optype3_batch1_in7_7_8_out5_5_8_filter3_3_pad0_0_stride1_1_activation0_bias1", "type5_lsz1_1_1_block8_4_1",
    "optype3_batch1_in8_8_4_out6_6_8_filter3_3_pad0_0_stride1_1_activation0_bias1", "type5_lsz1_256_1_block8_4_1"
#endif
};

static inline bool needImg2Col(const VkConvSpecializedConst& param)
{
    return !(param.pad_h == 0 && param.pad_w == 0 && param.stride_h == 1 && param.stride_w == 1 &&
             param.filter_h == 1 && param.filter_w == 1);
}

static std::string genConvSignature(const VkConvSpecializedConst& param)
{
    std::stringstream sig;
    // Android NN desn't set has_bias, so assume has_bias = 1
    int has_bias = 1;

    sig << "optype"     << (int)OperationType::CONV_2D << "_"
        << "batch"      << param.batch                 << "_"
        << "in"         << param.in_h                  << "_" << param.in_w     << "_" << param.channels << "_"
        << "out"        << param.out_h                 << "_" << param.out_w    << "_" << param.n        << "_"
        << "filter"     << param.filter_h              << "_" << param.filter_w << "_"
        << "pad"        << param.pad_h                 << "_" << param.pad_w    << "_"
        << "stride"     << param.stride_h              << "_" << param.stride_w << "_"
        << "activation" << param.activation            << "_"
        << "bias"       << has_bias;

    return sig.str();
}

static void configToString(const ShaderConfig& conf, std::string& str)
{
    std::stringstream ss;

    ss << "local_size_xyz_(" << conf.local_size_x << ", " << conf.local_size_y << ", " << conf.local_size_z << ")"
       << "_block_size_whd_(" << conf.block_width << ", " << conf.block_height << ", " << conf.block_depth << ")";

    str = ss.str();
}

static bool computeGroupCount(int& gx, int& gy, int& gz, const int type,
                              const VkConvSpecializedConst& param, const ShaderConfig& conf)
{
    if (param.local_sz_x == 0 || param.local_sz_y == 0 || param.local_sz_z == 0)
    {
        return false;
    }

    switch (type)
    {
    case CONV_SHADER_TYPE_BASIC: {
        gx = alignSize(alignSize(param.n, conf.block_width) / conf.block_width, param.local_sz_x) / param.local_sz_x;
        gy = alignSize(alignSize(param.m, conf.block_height) / conf.block_height, param.local_sz_y) / param.local_sz_y;
        gz = alignSize(alignSize(param.batch, conf.block_depth), param.local_sz_z) / param.local_sz_z;
        break;
    }
    case CONV_SHADER_TYPE_GEMM1: {
        ASSERT(conf.block_width == 1 && conf.block_height == 1 && conf.block_depth == 1 && conf.local_size_z == 1);
        gx = alignSize(param.n, conf.local_size_x) / conf.local_size_x;
        gy = alignSize(param.m, conf.local_size_y) / conf.local_size_y;
        gz = param.batch;
        break;
    }
    case CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL: {
        ASSERT(conf.block_width == 4 && conf.block_height == 4 && conf.block_depth == 1 && conf.local_size_z == 1);
        gx = alignSize(param.n / 4, conf.local_size_x) / conf.local_size_x;
        gy = alignSize(alignSize(param.m, 4) / 4, conf.local_size_y) / conf.local_size_y;
        gz = param.batch;
        break;
    }
    case CONV_SHADER_TYPE_GEMM_4_4_GENERIC: {
        ASSERT(conf.block_width == 4 && conf.block_height == 4 && conf.block_depth == 1 && conf.local_size_z == 1);
        gx = alignSize(param.n / 4, conf.local_size_x) / conf.local_size_x;
        gy = alignSize(alignSize(param.m, 4) / 4, conf.local_size_y) / conf.local_size_y;
        gz = param.batch;
        break;
    }
    case CONV_SHADER_TYPE_GEMM_4_8_GENERIC: {
        ASSERT(conf.block_width == 8 && conf.block_height == 4 && conf.block_depth == 1 && conf.local_size_z == 1);
        gx = alignSize(param.n / conf.block_width, conf.local_size_x) / conf.local_size_x;
        gy = alignSize(alignSize(param.m, 4) / 4, conf.local_size_y) / conf.local_size_y;
        gz = param.batch;
        break;
    }
    case CONV_SHADER_TYPE_GEMM_4_4_CHN3: {
        ASSERT(param.m % 4 == 0 && conf.block_width == 4);
        ASSERT(conf.block_height == 4 && conf.block_depth == 1 && conf.local_size_z == 1);
        gx = alignSize(param.n / 4, conf.local_size_x) / conf.local_size_x;
        gy = alignSize(param.m / 4, conf.local_size_y) / conf.local_size_y;
        gz = param.batch;
        break;
    }
    default:
        NOT_REACH_HERE;
        break;
    }

    if (conf.local_size_x * conf.local_size_y * conf.local_size_z < MAX_GROUP_INVOCATION &&
        conf.local_size_x < MAX_GROUP_SIZE_X &&
        conf.local_size_y < MAX_GROUP_SIZE_Y &&
        conf.local_size_z < MAX_GROUP_SIZE_Z &&
        gx < MAX_GROUP_COUNT_X && gy < MAX_GROUP_COUNT_Y && gz < MAX_GROUP_COUNT_Z)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static std::vector<ShaderConfig> genShaderConfigCandidates(VkConvSpecializedConst& param, ConvShaderType type)
{
    int group_x, group_y, group_z;
    std::vector<ShaderConfig> candidates;

    // int M = param.out_h * param.out_w;
    // int N = param.n;
    int K = param.filter_h * param.filter_w;

    ShaderConfig conf;

    if (type == CONV_SHADER_TYPE_GEMM_4_8_GENERIC)
    {
        if (param.channels % 4 != 0 || param.n % 8 != 0)
        {
            return candidates;
        }

        param.local_sz_x  = 1;
        param.local_sz_z  = 1;
        conf.local_size_x = 1;
        conf.local_size_z = 1;
        conf.block_width  = 8;
        conf.block_height = 4;
        conf.block_depth  = 1;
        shader_type       = CONV_SHADER_TYPE_GEMM_4_8_GENERIC;

        for (int ly = 1; ly < 8; ++ly)
        {
            conf.local_size_y = ly;
            param.local_sz_y  = ly;

            if (computeGroupCount(group_x, group_y, group_z, CONV_SHADER_TYPE_GEMM_4_8_GENERIC, param, conf))
            {
                candidates.push_back(conf);
            }
        }

        for (int ly = 8; ly <= 256; ly += 8)
        {
            conf.local_size_y = ly;
            param.local_sz_y  = ly;

            if (computeGroupCount(group_x, group_y, group_z, CONV_SHADER_TYPE_GEMM_4_8_GENERIC, param, conf))
            {
                candidates.push_back(conf);
            }
        }
    }
    else if (type == CONV_SHADER_TYPE_GEMM1)
    {
        if (needImg2Col(param) || K % 4 != 0)
        {
            return candidates;
        }

        param.local_sz_z  = 1;
        conf.block_width  = 1;
        conf.block_height = 1;
        conf.block_depth  = 1;
        shader_type       = CONV_SHADER_TYPE_GEMM1;

        for (int lx = 1; lx <= 256; lx *= 4)
        {
            for (int ly = 1; ly <= 256; ly *= 4)
            {
                conf.local_size_x = lx;
                conf.local_size_y = ly;

                param.local_sz_x = lx;
                param.local_sz_y = ly;

                if (computeGroupCount(group_x, group_y, group_z, CONV_SHADER_TYPE_GEMM1, param, conf))
                {
                    candidates.push_back(conf);
                }
            }
        }
    }
    else if (type == CONV_SHADER_TYPE_BASIC)
    {
        shader_type = CONV_SHADER_TYPE_BASIC;

        conf.block_width  = 1;
        conf.block_height = 1;
        conf.block_depth  = 1;

        for (int lx = 1; lx <= 256; lx *= 4)
        {
            for (int ly = 1; ly <= 256; ly *= 4)
            {
                for (int lz = 1; lz <= 32; lz *= 4)
                {
                    conf.local_size_x = lx;
                    conf.local_size_y = ly;
                    conf.local_size_z = lz;

                    param.local_sz_x  = lx;
                    param.local_sz_y  = ly;
                    param.local_sz_z  = lz;

                    if (computeGroupCount(group_x, group_y, group_z, CONV_SHADER_TYPE_BASIC, param, conf))
                    {
                        candidates.push_back(conf);
                    }
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

static void setSpecInfo(VkSpecializationMapEntry* entry,
                        VkSpecializationInfo& spec_info,
                        const VkConvSpecializedConst& spec_const,
                        const int entry_size)
{
    SET_SPEC_CONST_ENTRY(entry[0], 0, offsetof(VkConvSpecializedConst, local_sz_x), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[1], 1, offsetof(VkConvSpecializedConst, local_sz_y), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[2], 2, offsetof(VkConvSpecializedConst, local_sz_z), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[3], 3, offsetof(VkConvSpecializedConst, in_h), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[4], 4, offsetof(VkConvSpecializedConst, in_w), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[5], 5, offsetof(VkConvSpecializedConst, out_h), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[6], 6, offsetof(VkConvSpecializedConst, out_w), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[7], 7, offsetof(VkConvSpecializedConst, stride_h), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[8], 8, offsetof(VkConvSpecializedConst, stride_w), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[9], 9, offsetof(VkConvSpecializedConst, pad_h), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[10], 10, offsetof(VkConvSpecializedConst, pad_w), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[11], 11, offsetof(VkConvSpecializedConst, filter_h), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[12], 12, offsetof(VkConvSpecializedConst, filter_w), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[13], 13, offsetof(VkConvSpecializedConst, channels), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[14], 14, offsetof(VkConvSpecializedConst, batch), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[15], 15, offsetof(VkConvSpecializedConst, m), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[16], 16, offsetof(VkConvSpecializedConst, k), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[17], 17, offsetof(VkConvSpecializedConst, n), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[18], 18, offsetof(VkConvSpecializedConst, activation), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[19], 19, offsetof(VkConvSpecializedConst, num_items), sizeof(int));
    SET_SPEC_CONST_ENTRY(entry[20], 20, offsetof(VkConvSpecializedConst, tail_m), sizeof(int));

    spec_info.mapEntryCount = entry_size;
    spec_info.pMapEntries   = entry;
    spec_info.dataSize      = sizeof(spec_const);
    spec_info.pData         = &spec_const;

    return;
}

static void convOneBhwc(float* in_data, int in_offset, float* filter_data, int filter_offset,
                        float* bias, int bias_offset, float* convolved_data, int out_offset,
                        int in_w, int in_h, int out_w, int out_h,
                        int pad_left, int pad_top, int has_bias, int stride_w, int stride_h,
                        int filter_w, int filter_h, int dilation_x, int dilation_y,
                        int in_c, int out_c, int out_z, int out_x, int out_y, int depth, int activation)
{
    const int ZPAR = 1;

    if (out_x < out_w && out_y < out_h)
    {
        float sum[ZPAR];

        for(int kern = 0; kern < ZPAR; kern++)
        {
            sum[kern] = 0.0f;
        }

        const int org_y                 = out_y * stride_h - pad_top;
        const int org_x                 = out_x * stride_w - pad_left;
        const int current_filter_offset = filter_offset + depth * filter_h * filter_w * in_c;
        const int bias_index            = bias_offset + (depth % out_c);
        const int local_in_offset       = (org_y * in_w + org_x) * in_c;

        float* in_ptr     = in_data + (in_offset + local_in_offset);
        float* filter_ptr = filter_data + (current_filter_offset);

        for(int y = 0; y < filter_h; y++)
        {
            for(int x = 0; x < filter_w; x++)
            {
                if(org_y + int(y * dilation_y) >= 0 && org_y + int(y * dilation_y) < int(in_h) &&
                   org_x + int(x * dilation_x) >= 0 && org_x + int(x * dilation_x) < int(in_w))
                {
                    for(int c = 0; c < in_c; c++)
                    {
                        for(int outz = 0; outz < ZPAR; outz++)
                        {
                            sum[outz] += in_ptr[c] * filter_ptr[outz * filter_h * filter_w * in_c + c];
                        }
                    }
                }

                in_ptr += dilation_x * in_c;
                filter_ptr += in_c;
            }

            in_ptr += in_w * dilation_y * in_c - dilation_x * in_c * filter_w;
        }

        if (has_bias)
        {
            for (int kern = 0; kern < ZPAR; kern++)
            {
                if (depth + kern < out_z)
                {
                    int offset = out_offset + (out_y * out_w  + out_x) * out_c + depth + kern;
                    float out = sum[kern] + bias[bias_index +kern];
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
                    convolved_data[offset] = out;
                }
            }
        }
        else
        {
            for (int kern = 0; kern < ZPAR; kern++)
            {
                if (depth + kern < out_z)
                {
                    int offset = out_offset + (out_y * out_w  + out_x) * out_c + depth + kern;
                    float out  = sum[kern];

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

                    convolved_data[offset] = out;
                }
            }
        }
    }
}

static void convCpuBhwc(float* in_buffer, float* bias_buffer, float* filter_buffer, float* benchmark,
                        int batch, int group, int has_bias, int in_c, int in_w, int in_h,
                        int out_c, int out_w, int out_h, int filter_w, int filter_h,
                        int padding_left, int padding_top, int stride_w, int stride_h,
                        int dilation_x, int dilation_y, int activation)
{
    int in_offset     = 0;
    int bias_offset   = 0;
    int filter_offset = 0;
    int out_offset    = 0;
    int m             = out_c / group;
    int bottom_dim    = in_c * in_w * in_h;
    int top_dim       = out_c * out_w * out_h;

    for (int n = 0; n < batch; ++n)
    {
        for (int g = 0; g < group; ++g)
        {
            in_offset     = n * bottom_dim + in_w * in_h * (in_c / group) * g;
            filter_offset = filter_h * filter_w * (in_c / group) * m * g;
            bias_offset   = m * g;
            out_offset    = n * top_dim + out_w * out_h * m * g;

            for (int depth = 0; depth < m; ++depth)
            {
                for (int out_y = 0; out_y < out_h; ++out_y)
                {
                    for (int out_x = 0; out_x < out_w; ++out_x)
                    {
                        convOneBhwc(in_buffer, in_offset, filter_buffer, filter_offset, bias_buffer, bias_offset,
                                    benchmark, out_offset, in_w, in_h, out_w, out_h, padding_left, padding_top,
                                    has_bias, stride_w, stride_h, filter_w, filter_h, dilation_x, dilation_y,
                                    in_c, out_c, m, out_x, out_y, depth, activation);
                    }
                }
            }
        }
    }

    return;
}

static bool fake_loadConfig()
{
    return false;
}

static void fake_storeConfig()
{
    return;
}

bool VkCsExecutor::verifyResult(VkConvSpecializedConst& param,
                                float* in_buffer, float* filter_buffer, float* bias_buffer, float* result_buffer)
{
    int batch      = param.batch;
    int in_c       = param.channels;
    int in_w       = param.in_w;
    int in_h       = param.in_h;
    int out_c      = param.n;
    int out_w      = param.out_w;
    int out_h      = param.out_h;
    int filter_h   = param.filter_h;
    int filter_w   = param.filter_w;
    int stride_h   = param.stride_h;
    int stride_w   = param.stride_w;
    int pad_h      = param.pad_h;
    int pad_w      = param.pad_w;
    int has_bias   = 1;
    int activation = param.activation;
    int dilation_x = 1;
    int dilation_y = 1;
    int group      = 1;

    const int out_size = batch * out_h * out_w * out_c;
    float* benchmark   = new float[out_size];
    float* p_out       = result_buffer;

    convCpuBhwc(in_buffer, bias_buffer, filter_buffer, benchmark, batch, group, has_bias,
                in_c, in_w, in_h, out_c, out_w, out_h, filter_w, filter_h, pad_w, pad_h,
                stride_w, stride_h, dilation_x, dilation_y, activation);

    for (int b = 0; b < batch; ++b)
    {
        for (int h = 0; h < out_h; ++h)
        {
            for (int w = 0; w < out_w; ++w)
            {
                for (int c = 0; c < out_c; c++)
                {
                    int offset = b * (out_c * out_h * out_w) + h * (out_w * out_c) + w * out_c + c;

                    if ((fabs(p_out[offset] - benchmark[offset]) > 0.1 * fabs(benchmark[offset])) ||
                        !((fabs(benchmark[offset]) < 1.e-3) && (fabs(p_out[offset] - benchmark[offset]) < 1.e-4)))
                    {
                        NN_GPU_DEBUG("CONV_2D: convolution verification failed at (%d, %d, %d, %d), actual: %f, expected: %f\n",
                                b, h, w, c, p_out[offset], benchmark[offset]);

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

bool VkCsExecutor::verifyShader(VkConvSpecializedConst& param, ShaderConfig& conf,
                                VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out)
{
    std::string conf_str;

    configToString(conf, conf_str);
    NN_GPU_DEBUG("VkCsExecutor::verifyShader with %s", conf_str.c_str());

    bool ret = false;

    if (!tuning_convolve(param, conf, in, filter, bias, out))
    {
        LOG(ERROR) << "VkCsExecutor::verifyShader tuning_convolve failed.";
        return ret;
    }

    const int input_size  = param.batch * param.in_h * param.out_w * param.channels;
    const int output_size = param.batch * param.out_h * param.out_w * param.n;
    const int filter_size = param.n * param.filter_h * param.filter_w * param.channels;
    const int bias_size   = param.n;

    float* in_buffer      = new float[input_size];
    float* out_buffer     = new float[output_size];
    float* filter_buffer  = new float[filter_size];
    float* bias_buffer    = new float[bias_size];

    in.copyToBuffer(in_buffer, input_size);
    out.copyToBuffer(out_buffer, output_size);
    filter.copyToBuffer(filter_buffer, filter_size);
    bias.copyToBuffer(bias_buffer, bias_size);

    ret = verifyResult(param, in_buffer, filter_buffer, bias_buffer, out_buffer);

    delete[] in_buffer;
    delete[] out_buffer;
    delete[] filter_buffer;
    delete[] bias_buffer;

    return ret;
}

bool VkCsExecutor::tuning_convolve(VkConvSpecializedConst& param, const ShaderConfig& conf,
                                   VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out)
{
    VkSpecializationInfo spec_info;
    VkSpecializationMapEntry entry[SPEC_CONST_NUM];
    setSpecInfo(entry, spec_info, param, SPEC_CONST_NUM);

    switch (shader_type)
    {
    case CONV_SHADER_TYPE_GEMM_4_8_GENERIC: {
        opBase->createShaderModule(conv_gemmShader4_8_spv, sizeof(conv_gemmShader4_8_spv));
        opBase->createPipeline(sizeof(PushConst), &spec_info);
        break;
    }
    case CONV_SHADER_TYPE_GEMM1: {
        opBase->createShaderModule(conv_gemm1_spv, sizeof(conv_gemm1_spv));
        opBase->createPipeline(sizeof(PushConst), &spec_info);
        break;
    }
    case CONV_SHADER_TYPE_BASIC: {
        // todo: shaders of gemm_4_4, gemm_no_mig2col and gemm_4_4_chn3 are not added yet
        opBase->createShaderModule(conv_spv, sizeof(conv_spv));
        opBase->createPipeline(sizeof(PushConst), &spec_info);
        break;
    }
    case CONV_SHADER_TYPE_CHN3_TO_CHN4:
    case CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL:
    case CONV_SHADER_TYPE_GEMM_4_4_GENERIC:
    case CONV_SHADER_TYPE_GEMM_4_4_CHN3:
    default: {
        NOT_REACH_HERE;
    }
    }

    opBase->bindOperand(in, 0, opBase->descriptor_set);
    opBase->bindOperand(filter, 1, opBase->descriptor_set);
    opBase->bindOperand(bias, 2, opBase->descriptor_set);
    opBase->bindOperand(out, 3, opBase->descriptor_set);

    if (!computeGroupCount(opBase->group_x, opBase->group_y, opBase->group_z, shader_type, param, conf))
    {
        return false;
    }

    if (shader_type == CONV_SHADER_TYPE_BASIC)
    {
        int partition_num = (int)ceil(1.0 * param.n / opBase->group_y);
        for (int b = 0; b < param.batch; b++)
        {
            for (int n = 0; n < partition_num; n++)
            {
                opBase->recordCommandBuffer((void*)&param, sizeof(PushConst));
                opBase->runCommandBuffer();
            }
        }
    }
    else
    {
        opBase->recordCommandBuffer((void*)&param, sizeof(PushConst));
        opBase->runCommandBuffer();
    }

    return true;
}

bool VkCsExecutor::tryShaderConfig(VkConvSpecializedConst& param,
                                   ShaderConfig& best,
                                   const std::vector<ShaderConfig>& configs,
                                   VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out)
{
    NN_GPU_PERF("CONV_2D: %s: try shader type: %d\n", __func__, shader_type);

    bool ret = false;
    ShaderConfig conf;
    std::string conf_str;
    TuningTimeMap time_map;

    for (size_t i = 0; i < configs.size(); i ++)
    {
        long elapsed_us, t;

        param.local_sz_x   = configs[i].local_size_x;
        param.local_sz_y   = configs[i].local_size_y;
        param.local_sz_z   = configs[i].local_size_z;

        conf.local_size_x = configs[i].local_size_x;
        conf.local_size_y = configs[i].local_size_y;
        conf.local_size_z = configs[i].local_size_z;

        conf.block_width   = configs[i].block_width;
        conf.block_height  = configs[i].block_height;
        conf.block_depth   = configs[i].block_depth;

        // warm up
        ret = tuning_convolve(param, conf, in, filter, bias, out);
        if (ret == false)
        {
            continue;
        }

        TIMER_START(conv_time);
        ret = tuning_convolve(param, conf, in, filter, bias, out);
        t = TIMER_STOP(conv_time);

        if (ret)
        {
            elapsed_us = t;

            configToString(configs[i], conf_str);
            NN_GPU_PERF("CONV_2D: %s: tune: %8.3f ms, %s\n", __func__, 1.0 * elapsed_us / 1000, conf_str.c_str());

            std::pair<long, int> entry(elapsed_us, i);
            time_map.insert(entry);
        }
        else
        {
            elapsed_us = std::numeric_limits<long>::max();
        }
    }

    for (TuningTimeMap::iterator it = time_map.begin(); it != time_map.end(); ++it)
    {
        ShaderConfig candidate = configs[it->second];

        param.local_sz_x   = candidate.local_size_x;
        param.local_sz_y   = candidate.local_size_y;
        param.local_sz_z   = candidate.local_size_z;

        conf.block_width   = candidate.block_width;
        conf.block_height  = candidate.block_height;
        conf.block_depth   = candidate.block_depth;

        configToString(candidate, conf_str);
        out.resetForTune();

        if (verifyShader(param, candidate, in, filter, bias, out))
        {
            best = candidate;
            ret = true;
        }
    }

    return ret;
}

void VkCsExecutor::tune(VkConvSpecializedConst& param, ShaderConfig& conf,
                        VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out)
{
    bool succeed = false;
    std::vector<ShaderConfig> configs;

    if (!succeed)
    {
        configs = genShaderConfigCandidates(param, CONV_SHADER_TYPE_GEMM_4_8_GENERIC);
        succeed = tryShaderConfig(param, conf, configs, in, filter, bias, out);
    }

    if (!succeed)
    {
        configs = genShaderConfigCandidates(param, CONV_SHADER_TYPE_GEMM1);
        succeed = tryShaderConfig(param, conf, configs, in, filter, bias, out);
    }

    if (!succeed)
    {
        std::string sig = genConvSignature(param);
        NN_GPU_PERF("CONV_2D: %s: %s fallback to basic shader, THIS MAY HAVE POOR PERFORMANCE !\n", __func__, sig.c_str());
        configs = genShaderConfigCandidates(param, CONV_SHADER_TYPE_BASIC);
        succeed = tryShaderConfig(param, conf, configs, in, filter, bias, out);
    }

    ASSERT(succeed);
    return;
}

static bool chn3ToChn4(VkConvSpecializedConst& param, ShaderConfig& config)
{
    param.local_sz_x   = 16;
    param.local_sz_y   = 1;
    param.local_sz_z   = 1;

    config.block_width  = ITEMS_PER_WI;
    config.block_height = 1;
    config.block_depth  = 1;

    return true;
}

void computeConvOutputShapeAndPadding(const PaddingScheme& padding_mode,
                                      const uint32_t& in_h, const uint32_t& in_w,
                                      const uint32_t& filter_h, const uint32_t& filter_w,
                                      const uint32_t& dilation_h, const uint32_t& dilation_w,
                                      const uint32_t& stride_h, const uint32_t& stride_w,
                                      uint32_t& out_h, uint32_t& out_w)
{
    if (padding_mode == kPaddingValid)
    {
        out_h = ceil((in_h - (filter_h - 1) * dilation_h) / stride_h);
        out_w = ceil((in_w - (filter_w - 1) * dilation_w) / stride_w);
    }
    else if (padding_mode == kPaddingSame)
    {
        out_h = ceil(in_h / stride_h);
        out_w = ceil(in_w / stride_w);
    }
    else
    {
        LOGE("Invalid padding mode:%d", padding_mode);
    }
}

static void prepareConfig(const Operation& operation, ShaderConfig& config)
{
    //tune();
    (void)(operation);
    (void)(config);
}

static void string2Config(const char* confString, ShaderConfig &conf)
{
    sscanf(confString, "type%d_lsz%d_%d_%d_block%d_%d_%d",
           &shader_type, &conf.local_size_x,  &conf.local_size_y, &conf.local_size_z,
           &conf.block_width, &conf.block_height, &conf.block_depth);

    NN_GPU_DEBUG("CONV_2D: string2Config shader type is %d, local_size_x %d, local_size_y %d, local_size_z %d, "
                 "block_width %d, block_height %d, block_depth %d",
                 shader_type, conf.local_size_x, conf.local_size_y,
                 conf.local_size_z, conf.block_width, conf.block_height, conf.block_depth);
}

void VkCsExecutor::prepareShaderConfig(VkConvSpecializedConst& param, ShaderConfig& conf,
                                       VkOperand& in, VkOperand& filter, VkOperand& bias, VkOperand& out)
{
    const std::string sig = genConvSignature(param);

    bool tuned = false;
    bool found = false;

    mtx.lock();

    // load default configs and get vulkan info
    if (!is_initialized)
    {
        NN_GPU_DEBUG("prepareShaderConfig: init shaderConfigMap for vulkan backend shader");

        int configNum = 0;
        if (sizeof(defaultConfig) > 0)
        {
            configNum = sizeof(defaultConfig) / sizeof(defaultConfig[0]) / 2;
        }
        for (int i = 0; i < configNum; i++)
        {
            ShaderConfigPair entry(defaultConfig[2 * i], defaultConfig[2 * i + 1]);
            shaderConfigMap.insert(entry);
            NN_GPU_PERF("CONV_2D: %s: load pre-tuned config: %s, %s\n", __func__, defaultConfig[2 * i], defaultConfig[2 * i + 1]);
        }
        NN_GPU_DEBUG("prepareShaderConfig: shaderConfigMap is initialized");
        is_initialized = true;
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

    NN_GPU_PERF("CONV_2D: %s: config cannot be found from in-memory cache", __func__);

    // todo: load from persistent config
    found = fake_loadConfig();
    if (!found)
    {
        tune(param, conf, in, filter, bias, out);
        tuned = true;
    }

    // todo: store persistent config
    if (tuned)
    {
        fake_storeConfig();
    }

    mtx.unlock();
}

bool VkCsExecutor::convolve(const Operation& operation, ShaderConfig& config)
{
#define BUFFER_NUM 4
    opBase->initVulkanThing(BUFFER_NUM);

    const hidl_vec<uint32_t>& ins  = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    const size_t inCount = ins.size();
    ASSERT(inCount == 10 || inCount == 7);

    VkOperand& in     = operands[ins[0]];
    VkOperand& filter = operands[ins[1]];
    VkOperand& bias   = operands[ins[2]];
    VkOperand& out    = operands[outs[0]];

    Shape in_shape     = in.getShape();
    Shape out_shape    = out.getShape();
    Shape filter_shape = filter.getShape();
    Shape bias_shape   = bias.getShape();

    int M      = out_shape[kShapeIdxHeight] * out_shape[kShapeIdxWidth];
    int N      = out_shape[kShapeIdxChannel];
    int K      = in_shape[kShapeIdxChannel] * filter_shape[kShapeIdxHeight] * filter_shape[kShapeIdxWidth];
    int tail_m = M % 4;

    PaddingScheme padding_mode;

    VkConvSpecializedConst spec_const(in_shape[kShapeIdxHeight], in_shape[kShapeIdxWidth],
                                      out_shape[kShapeIdxHeight], out_shape[kShapeIdxWidth],
                                      filter_shape[kShapeIdxHeight], filter_shape[kShapeIdxWidth],
                                      in_shape[kShapeIdxChannel], in_shape[kShapeIdxBatch], M, K, N, tail_m);

    PushConst push_const;

    if (opBase->pipeline == VK_NULL_HANDLE)
    {
        if (inCount == 10)
        {
            uint32_t padding_left   = operands[ins[3]].getScalarData<uint32_t>();
            uint32_t padding_top    = operands[ins[5]].getScalarData<uint32_t>();

            // uint32_t padding_right  = operands[ins[4]].getScalarData<uint32_t>();
            // uint32_t padding_bottom = operands[ins[6]].getScalarData<uint32_t>();

            spec_const.pad_w        = padding_left;
            spec_const.pad_h        = padding_top;
            spec_const.stride_w     = operands[ins[7]].getScalarData<uint32_t>();
            spec_const.stride_h     = operands[ins[8]].getScalarData<uint32_t>();
            spec_const.activation   = operands[ins[9]].getScalarData<uint32_t>();

            if (padding_left == 0 && padding_top == 0)
            {
                padding_mode = kPaddingValid;
            }
            else
            {
                padding_mode = kPaddingSame;
            }
        }
        else
        {
            padding_mode          = static_cast<PaddingScheme>(operands[ins[3]].getScalarData<uint32_t>());
            spec_const.stride_w   = operands[ins[4]].getScalarData<uint32_t>();
            spec_const.stride_h   = operands[ins[5]].getScalarData<uint32_t>();
            spec_const.activation = operands[ins[6]].getScalarData<uint32_t>();

            calculateExplicitPadding(spec_const.in_w, spec_const.stride_w, spec_const.filter_w,
                                     padding_mode, &spec_const.pad_w);
            calculateExplicitPadding(spec_const.in_h, spec_const.stride_h, spec_const.filter_h,
                                     padding_mode, &spec_const.pad_h);
        }

        // for chn3_to_chn4 convertion
        VkOperand chn4_filter = filter;
        VkOperand chn4_in     = in;

        if (spec_const.channels == 3)
        {
            uint32_t filter_size    = spec_const.channels * spec_const.filter_w * spec_const.filter_h * spec_const.n;
            uint32_t input_size     = spec_const.channels * spec_const.in_w * spec_const.in_h * spec_const.batch;
            uint32_t total_thread_x = 0;

            const int new_channel_num = 4;

            opBase->rebindVkBuffer(chn4_filter, spec_const.n, spec_const.filter_w, spec_const.filter_h, new_channel_num);
            opBase->rebindVkBuffer(chn4_in, spec_const.batch, spec_const.in_w, spec_const.in_h, new_channel_num);

            // first off, convert filter to 4 channels
            if (tmpBoSize == 0)
            {
                total_thread_x = alignSize(filter_size, ITEMS_PER_WI) / ITEMS_PER_WI;
                chn3ToChn4(spec_const, config);

                int group_x = opBase->computeGroupCountX(total_thread_x, spec_const.local_sz_x, spec_const.local_sz_x);
                opBase->setGroupSize(group_x, 1, 1);

                ++tmpBoSize;

                VkSpecializationMapEntry entry[SPEC_CONST_NUM];
                VkSpecializationInfo spec_info;
                spec_const.num_items = filter_size;

                setSpecInfo(entry, spec_info, spec_const, SPEC_CONST_NUM);

                opBase->createShaderModule(conv_chn3to4_spv, sizeof(conv_chn3to4_spv));
                opBase->createPipeline(sizeof(PushConst), &spec_info);

                opBase->bindOperand(filter, 0, opBase->descriptor_set);
                opBase->bindOperand(chn4_filter, 1, opBase->descriptor_set);

                int partition_num = (int)ceil(1.0 * N / opBase->group_y);


                uint32_t num = partition_num * 3;
                for (uint32_t i = 0; i < num; i++)
                {
                    opBase->recordCommandBuffer((void*)&push_const, sizeof(PushConst));
                    opBase->runCommandBuffer();
                }
                // chn4_filter.dumpToFile("filter", 4);
            }

            // then, convert input
            total_thread_x = alignSize(input_size, ITEMS_PER_WI) / ITEMS_PER_WI;
            chn3ToChn4(spec_const, config);

            int group_x = opBase->computeGroupCountX(total_thread_x, spec_const.local_sz_x, spec_const.local_sz_x);
            opBase->setGroupSize(group_x, 1, 1);
            --tmpBoSize;

            VkSpecializationMapEntry entry[SPEC_CONST_NUM];
            VkSpecializationInfo spec_info;

            spec_const.num_items = input_size;
            setSpecInfo(entry, spec_info, spec_const, SPEC_CONST_NUM);

            opBase->createShaderModule(conv_chn3to4_spv, sizeof(conv_chn3to4_spv));
            opBase->createPipeline(sizeof(PushConst), &spec_info);

            opBase->bindOperand(in, 0, opBase->descriptor_set);
            opBase->bindOperand(chn4_in, 1, opBase->descriptor_set);

            int partition_num = (int)ceil(1.0 * N / opBase->group_y);

            if (shader_type == CONV_SHADER_TYPE_BASIC)
            {
                for (uint32_t b = 0; b < filter_shape[kShapeIdxBatch]; b++)
                {
                    for (int n = 0; n < partition_num; n++)
                    {
                        opBase->recordCommandBuffer((void*)&push_const, sizeof(PushConst));
                        opBase->runCommandBuffer();
                    }
                }
            }
            else
            {
                opBase->recordCommandBuffer((void*)&push_const, sizeof(PushConst));
                opBase->runCommandBuffer();
            }

            spec_const.k = spec_const.k / 3 * 4;
            spec_const.channels = 4;
            converted_to_chn4 = true;

            // chn4_in.dumpToFile("in", 4);
        }

        // prepare shader config
        if (converted_to_chn4)
        {
            prepareShaderConfig(spec_const, config, chn4_in, chn4_filter, bias, out);
        }
        else
        {
            prepareShaderConfig(spec_const, config, in, filter, bias, out);
        }

        spec_const.local_sz_x = config.local_size_x;
        spec_const.local_sz_y = config.local_size_y;
        spec_const.local_sz_z = config.local_size_z;

        VkSpecializationInfo spec_info;
        VkSpecializationMapEntry entry[SPEC_CONST_NUM];
        setSpecInfo(entry, spec_info, spec_const, SPEC_CONST_NUM);

        switch (shader_type)
        {
        case CONV_SHADER_TYPE_GEMM_4_8_GENERIC: {
            opBase->createShaderModule(conv_gemmShader4_8_spv, sizeof(conv_gemmShader4_8_spv));
            opBase->createPipeline(sizeof(PushConst), &spec_info);
            break;
        }
        case CONV_SHADER_TYPE_GEMM1: {
            opBase->createShaderModule(conv_gemm1_spv, sizeof(conv_gemm1_spv));
            opBase->createPipeline(sizeof(PushConst), &spec_info);
            break;
        }
        case CONV_SHADER_TYPE_BASIC:
        case CONV_SHADER_TYPE_GEMM_4_4_NO_IMG2COL:
        case CONV_SHADER_TYPE_GEMM_4_4_GENERIC:
        case CONV_SHADER_TYPE_GEMM_4_4_CHN3: {
            // todo: shaders of gemm_4_4, gemm_no_mig2col and gemm_4_4_chn3 are not added yet
            opBase->createShaderModule(conv_spv, sizeof(conv_spv));
            opBase->createPipeline(sizeof(PushConst), &spec_info);
            break;
        }
        case CONV_SHADER_TYPE_CHN3_TO_CHN4:
        default:
            NOT_REACH_HERE;
            break;
        }

        if (spec_const.channels == 4 && converted_to_chn4)
        {
            // bind the converted channel 4 operands
            opBase->bindOperand(chn4_in, 0, opBase->descriptor_set);
            opBase->bindOperand(chn4_filter, 1, opBase->descriptor_set);
            converted_to_chn4 = false;
        }
        else
        {
            // bind the original input & filter
            opBase->bindOperand(in, 0, opBase->descriptor_set);
            opBase->bindOperand(filter, 1, opBase->descriptor_set);
        }

        // chn3ToChn4 is just for input & filter, no need to convert bias & output
        opBase->bindOperand(bias, 2, opBase->descriptor_set);
        opBase->bindOperand(out, 3, opBase->descriptor_set);
    }

    // todo: should be moved to opBase
    if (spec_const.local_sz_x == 0 || spec_const.local_sz_y == 0 || spec_const.local_sz_z == 0)
    {
        NOT_REACH_HERE;
    }
    else if (false == computeGroupCount(opBase->group_x, opBase->group_y, opBase->group_z, shader_type, spec_const, config))
    {
        NN_GPU_DEBUG("VkCsExecutor::doCONV_2D: computeGroupCount failed");
        return false;
    }

    // todo: duplicated, remove it
    opBase->setGroupSize(opBase->group_x, opBase->group_y, opBase->group_z);

    NN_GPU_DEBUG("VkCsExecutor::doCONV_2D: lsx %d, lsy %d, lsz %d, group_x %d, group_y %d, group_z %d, "
                 "in_w %d, in_h %d, out_h %d, out_w %d, stride_h %d, stride_w %d, pad_h %d, pad_w %d, "
                 "filter_h %d, filter_w %d, channels %d, batch %d, m %d, k %d, n %d, activation %d",
                 spec_const.local_sz_x, spec_const.local_sz_y, spec_const.local_sz_z, opBase->group_x,
                 opBase->group_y, opBase->group_z, spec_const.in_w, spec_const.in_h, spec_const.out_h,
                 spec_const.out_w, spec_const.stride_h, spec_const.stride_w, spec_const.pad_h,
                 spec_const.pad_w, spec_const.filter_h, spec_const.filter_w, spec_const.channels,
                 spec_const.batch, spec_const.m, spec_const.k, spec_const.n, spec_const.activation);

    int partition_num = (int)ceil(1.0 * N / opBase->group_y);

    if (shader_type == CONV_SHADER_TYPE_BASIC)
    {
        for (uint32_t b = 0; b < in_shape[kShapeIdxBatch]; b++)
        {
            for (int n = 0; n < partition_num; n++)
            {
                opBase->recordCommandBuffer((void*)&push_const, sizeof(PushConst));
                opBase->runCommandBuffer();
            }
        }
    }
    else
    {
        opBase->recordCommandBuffer((void*)&push_const, sizeof(PushConst));
        opBase->runCommandBuffer();
    }

    // out.dumpToFile("out", spec_const.n);
    return true;
}

// FIXME:
// Android NN don't set group, dilation, has_bias,
// so make these assumptions: group = 1, dilation = 1, has_bias = 1
bool VkCsExecutor::doCONV_2D(const Operation& operation)
{
    NN_GPU_ENTRY();

    ASSERT(operation.type == OperationType::CONV_2D);
    bool ret = false;

    ShaderConfig config = {1, 16, 1, 1, 1, 1};
    prepareConfig(operation, config);
    ret = convolve(operation, config);
    if (!ret)
        LOGE("failed to call convolve function");

    NN_GPU_EXIT();
    return ret;
}

NAME_SPACE_STOP
