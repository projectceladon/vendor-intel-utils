/*
 * Copyright @2019 Intel Corporation
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

// todo: query group count from vulkan device
#define MAX_GROUP_COUNT_X 65535
#define MAX_GROUP_COUNT_Y 65535
#define MAX_GROUP_COUNT_Z 65535

#define DEFAULT_DILATION_H 1
#define DEFAULT_DILATION_W 1
#define HAS_BIAS 1

struct PushConst {
public:
    PushConst() {};
};

struct SpecializaitonConst {
public:
    SpecializaitonConst(int ih, int iw, int oh, int ow, int dh, int dw,
                        int fh, int fw, int chn, int bias, int M, int K, int N):
        local_sz_x(0), local_sz_y(0), local_sz_z(0),
        in_h(ih), in_w(iw), out_h(oh), out_w(ow), stride_h(0), stride_w(0), dilation_h(dh), dilation_w(dw),
        pad_h(0), pad_w(0), filter_h(fh), filter_w(fw), channels(chn), has_bias(bias),
        m(M), k(K), n(N), depth_multiplier(0), activation(0)
    {};

    int local_sz_x;
    int local_sz_y;
    int local_sz_z;
    int in_h;
    int in_w;
    int out_h;
    int out_w;
    int stride_h;
    int stride_w;
    int dilation_h;
    int dilation_w;
    int pad_h;
    int pad_w;
    int filter_h;
    int filter_w;
    int channels;
    int has_bias;
    int m;
    int k;
    int n;
    int depth_multiplier;
    int activation;
};

static void prepareConfig(const Operation& operation, ShaderConfig& config)
{
    //tune();
    (void)(operation);
    (void)(config);
}

bool VkCsExecutor::depthConvolve(const Operation& operation, ShaderConfig& config)
{
#define BUFFER_NUM 4
    opBase->initVulkanThing(BUFFER_NUM);

    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    ASSERT(ins.size() == 11 || ins.size() == 8);

    VkOperand& in     = operands[ins[0]];
    VkOperand& filter = operands[ins[1]];
    VkOperand& bias   = operands[ins[2]];
    VkOperand& out    = operands[outs[0]];

    Shape in_shape     = in.getShape();
    Shape out_shape    = out.getShape();
    Shape filter_shape = filter.getShape();
    Shape bias_shape   = bias.getShape();

    uint32_t M = out_shape[kShapeIdxHeight] * out_shape[kShapeIdxWidth];
    uint32_t N = out_shape[kShapeIdxChannel];
    uint32_t K = in_shape[kShapeIdxChannel] * filter_shape[kShapeIdxHeight] * filter_shape[kShapeIdxWidth];

    PaddingScheme padding_mode;
    PushConst push_const;
    SpecializaitonConst spec_const(in_shape[kShapeIdxHeight], in_shape[kShapeIdxWidth],
                                   out_shape[kShapeIdxHeight], out_shape[kShapeIdxWidth],
                                   DEFAULT_DILATION_H, DEFAULT_DILATION_W,
                                   filter_shape[kShapeIdxHeight], filter_shape[kShapeIdxWidth],
                                   in_shape[kShapeIdxChannel], HAS_BIAS, M, K, N);

    // todo: tuning
    spec_const.local_sz_x = config.local_size_x;
    spec_const.local_sz_y = config.local_size_y;
    spec_const.local_sz_z = config.local_size_z;

    if (opBase->pipeline == VK_NULL_HANDLE)
    {
        if (ins.size() == 11)
        {
            uint32_t padding_left       = operands[ins[3]].getScalarData<uint32_t>();
            uint32_t padding_right      = operands[ins[4]].getScalarData<uint32_t>();
            uint32_t padding_top        = operands[ins[5]].getScalarData<uint32_t>();
            uint32_t padding_bottom     = operands[ins[6]].getScalarData<uint32_t>();

            spec_const.pad_w            = padding_right;
            spec_const.pad_h            = padding_bottom;
            spec_const.stride_w         = operands[ins[7]].getScalarData<uint32_t>();
            spec_const.stride_h         = operands[ins[8]].getScalarData<uint32_t>();
            spec_const.depth_multiplier = operands[ins[9]].getScalarData<uint32_t>();
            spec_const.activation       = operands[ins[10]].getScalarData<uint32_t>();

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
            padding_mode                = static_cast<PaddingScheme>(operands[ins[3]].getScalarData<uint32_t>());
            spec_const.stride_w         = operands[ins[4]].getScalarData<uint32_t>();
            spec_const.stride_h         = operands[ins[5]].getScalarData<uint32_t>();
            spec_const.depth_multiplier = operands[ins[6]].getScalarData<uint32_t>();
            spec_const.activation       = operands[ins[7]].getScalarData<uint32_t>();

            calculateExplicitPadding(spec_const.in_w,
                    spec_const.stride_w, spec_const.filter_w, padding_mode, &spec_const.pad_w);
            calculateExplicitPadding(spec_const.in_h,
                    spec_const.stride_h, spec_const.filter_h, padding_mode, &spec_const.pad_h);
        }

#define SPEC_CONST_NUM 22
        VkSpecializationMapEntry entry[SPEC_CONST_NUM];

        SET_SPEC_CONST_ENTRY(entry[0], 0, offsetof(SpecializaitonConst, local_sz_x), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[1], 1, offsetof(SpecializaitonConst, local_sz_y), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[2], 2, offsetof(SpecializaitonConst, local_sz_z), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[3], 3, offsetof(SpecializaitonConst, in_h), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[4], 4, offsetof(SpecializaitonConst, in_w), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[5], 5, offsetof(SpecializaitonConst, out_h), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[6], 6, offsetof(SpecializaitonConst, out_w), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[7], 7, offsetof(SpecializaitonConst, stride_h), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[8], 8, offsetof(SpecializaitonConst, stride_w), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[9], 9, offsetof(SpecializaitonConst, dilation_h), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[10], 10, offsetof(SpecializaitonConst, dilation_w), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[11], 11, offsetof(SpecializaitonConst, pad_h), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[12], 12, offsetof(SpecializaitonConst, pad_w), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[13], 13, offsetof(SpecializaitonConst, filter_h), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[14], 14, offsetof(SpecializaitonConst, filter_w), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[15], 15, offsetof(SpecializaitonConst, channels), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[16], 16, offsetof(SpecializaitonConst, has_bias), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[17], 17, offsetof(SpecializaitonConst, m), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[18], 18, offsetof(SpecializaitonConst, k), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[19], 19, offsetof(SpecializaitonConst, n), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[20], 20, offsetof(SpecializaitonConst, depth_multiplier), sizeof(int));
        SET_SPEC_CONST_ENTRY(entry[21], 21, offsetof(SpecializaitonConst, activation), sizeof(int));

        VkSpecializationInfo spec_info;

        spec_info.mapEntryCount = SPEC_CONST_NUM;
        spec_info.pMapEntries   = entry;
        spec_info.dataSize      = sizeof(spec_const);
        spec_info.pData         = &spec_const;

        NN_GPU_DEBUG("VkCsExecutor::doDEPTHWISE_CONV_2D: run createShaderModule");
        opBase->createShaderModule(dw_conv_spv, sizeof(dw_conv_spv));

        NN_GPU_DEBUG("VkCsExecutor::doDEPTHWISE_CONV_2D: run createPipeline");
        opBase->createPipeline(sizeof(PushConst), &spec_info);
    }

    if (spec_const.local_sz_x != 0 && spec_const.local_sz_y != 0 && spec_const.local_sz_z != 0)
    {
        opBase->group_x = ceil(static_cast<float>(spec_const.out_w) / spec_const.local_sz_x);
        opBase->group_y = ceil(static_cast<float>(spec_const.out_h) / spec_const.local_sz_y);
        opBase->group_z = ceil(static_cast<float>
            ((ceil(static_cast<float>(N) * in_shape[kShapeIdxBatch] / spec_const.depth_multiplier))) / spec_const.local_sz_z);
    }
    else
    {
        NOT_REACH_HERE;
    }

    NN_GPU_DEBUG("VkCsExecutor::doDEPTHWISE_CONV_2D: lsx %d, lsy %d, lsz %d, group_x %d, group_y %d, group_z %d, "
        "in_h %d, in_w %d, out_h %d, out_w %d, stride_h %d, stride_w %d, dilation_h %d, dilation_w %d, pad_h %d, pad_w %d"
        "filter_h %d, filter_w %d, channels %d, has_bias %d, m %d, k %d, n %d, depth_multiplier %d, activation %d",
        spec_const.local_sz_x, spec_const.local_sz_y, spec_const.local_sz_z, opBase->group_x, opBase->group_y, opBase->group_z,
        spec_const.in_h, spec_const.in_w, spec_const.out_h, spec_const.out_w, spec_const.stride_h, spec_const.stride_w,
        spec_const.dilation_h, spec_const.dilation_w, spec_const.pad_h, spec_const.pad_w, spec_const.filter_h,
        spec_const.filter_w, spec_const.channels, spec_const.has_bias, spec_const.m, spec_const.k, spec_const.n,
        spec_const.depth_multiplier, spec_const.activation);

    NN_GPU_DEBUG("VkCsExecutor::doDEPTHWISE_CONV_2D: bind operands");
    opBase->bindOperand(in, 0, opBase->descriptor_set);
    opBase->bindOperand(filter, 1, opBase->descriptor_set);
    opBase->bindOperand(bias, 2, opBase->descriptor_set);
    opBase->bindOperand(out, 3, opBase->descriptor_set);

    int partition_num = 1;
    partition_num = (int)ceil(1.0 * N / opBase->group_y);

    for (uint32_t b = 0;  b < in_shape[kShapeIdxBatch]; b++)
    {
        for (int n = 0;  n < partition_num; n++)
        {
            opBase->recordCommandBuffer((void*)&push_const, sizeof(PushConst));
            opBase->runCommandBuffer();
        }
    }

    return true;
}

bool VkCsExecutor::doDEPTHWISE_CONV_2D(const Operation& operation)
{
    NN_GPU_ENTRY();

    ASSERT(operation.type == OperationType::DEPTHWISE_CONV_2D);

    bool ret = false;

    // config: auto config.
    ShaderConfig config = {1, 1, 16, 1, 1, 1};
    prepareConfig(operation, config);

    ret = depthConvolve(operation, config);

    NN_GPU_EXIT();

    return ret;
}

NAME_SPACE_STOP
