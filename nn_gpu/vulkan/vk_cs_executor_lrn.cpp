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

#define DEFAULT_LOCAL_SZ 256
// TODO: query group count from vulkan device
#define MAX_GROUP_COUNT_X 65535
#define MAX_GROUP_COUNT_Y 65535
#define MAX_GROUP_COUNT_Z 65535

struct LRNParam {
    int thread_num;
    int channels;
    int height;
    int width;
    int filter_len;
    int radius;
    float alpha;
    float bias;
    float negative_beta;
};

static void prepareConfig(const Operation& operation, ShaderConfig& config)
{
    //tune();
    (void)(operation);
    (void)(config);
}

bool VkCsExecutor::doLOCAL_RESPONSE_NORMALIZATION(const Operation& operation)
{
    NN_GPU_ENTRY();
#define BUFFER_NUM 2
    opBase->initVulkanThing(BUFFER_NUM);

    ASSERT(operation.type == OperationType::LOCAL_RESPONSE_NORMALIZATION);

    ShaderConfig config = {DEFAULT_LOCAL_SZ, 1, 1, 1, 1, 1};
    prepareConfig(operation, config);

    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;
    const size_t inCount = ins.size();
    ASSERT(inCount == 5);

    VkOperand& in       = operands[ins[0]];
    VkOperand& radius   = operands[ins[1]];
    VkOperand& bias     = operands[ins[2]];
    VkOperand& alpha    = operands[ins[3]];
    VkOperand& beta     = operands[ins[4]];
    VkOperand& out      = operands[outs[0]];

    Shape in_shape  = in.getShape();
    Shape out_shape = out.getShape();

    LRNParam param;
    param.thread_num    = in_shape[kShapeIdxHeight] * in_shape[kShapeIdxWidth] * in_shape[kShapeIdxBatch];
    param.channels      = in_shape[kShapeIdxChannel];
    param.height        = in_shape[kShapeIdxHeight];
    param.width         = in_shape[kShapeIdxWidth];
    param.radius        = static_cast<PaddingScheme>(radius.getScalarData<int32_t>());
    param.filter_len    = param.radius * 2 + 1;
    //todo: do we need normalize alpha value with filter_len?
    param.alpha         = alpha.getScalarData<float>();
    param.bias          = bias.getScalarData<float>();
    param.negative_beta = -1.0 * beta.getScalarData<float>();

    NN_GPU_DEBUG("VkCsExecutor::doLOCAL_RESPONSE_NORMALIZATION: param thread_num %d, channels %d, "
        "height %d, width %d, filter_len %d, radius %d, alpha %f, bias %f, negative_beta %f",
        param.thread_num, param.channels, param.height, param.width,
        param.filter_len, param.radius, param.alpha, param.bias, param.negative_beta);

    if (opBase->pipeline == VK_NULL_HANDLE)
    {
        opBase->createShaderModule(lrn_spv, sizeof(lrn_spv));
        opBase->createPipeline(sizeof(LRNParam));
        opBase->group_x = alignSize(param.thread_num, config.local_size_x) / config.local_size_x;
        opBase->group_y = 1;
        opBase->group_z = 1;
    }

    NN_GPU_DEBUG("VkCsExecutor::doLOCAL_RESPONSE_NORMALIZATION: bind operands");
    opBase->bindOperand(in, 0, opBase->descriptor_set);
    opBase->bindOperand(out, 1, opBase->descriptor_set);

    NN_GPU_DEBUG("VkCsExecutor::doLOCAL_RESPONSE_NORMALIZATION: do recordCommandBuffer");
    opBase->recordCommandBuffer((void *)&param, sizeof(LRNParam));

    NN_GPU_DEBUG("VkCsExecutor::doLOCAL_RESPONSE_NORMALIZATION: run runCommandBuffer");
    opBase->runCommandBuffer();

    NN_GPU_EXIT();

    return true;
}

NAME_SPACE_STOP
