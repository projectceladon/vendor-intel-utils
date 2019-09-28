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

struct PoolParam {
      int channels;
      int in_height;
      int in_width;
      int out_height;
      int out_width;
      int padding_top;
      int padding_left;
      int filter_h;
      int filter_w;
      int stride_h;
      int stride_w;
      int total;
      int mask_or_padded_area;
};

enum OpPoolType { kPoolTypeAvg, kPoolTypeMax, kPoolTypeNum };

static void prepareConfig(const Operation& operation, ShaderConfig& config)
{
    //tune();
    (void)(operation);
    (void)(config);
}

bool VkCsExecutor::doPool(const Operation& operation, ShaderConfig& config, const int type)
{
#define BUFFER_NUM 2
    opBase->initVulkanThing(BUFFER_NUM);

    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;
    const size_t inCount = ins.size();
    int activation;
    ASSERT(inCount == 10 || inCount == 7);

    VkOperand& in  = operands[ins[0]];
    VkOperand& out = operands[outs[0]];

	Shape in_shape = in.getShape();
	Shape out_shape = out.getShape();

	PaddingScheme padding_mode;

	if (opBase->pipeline == VK_NULL_HANDLE)
	{
        if (type == kPoolTypeAvg)
        {
		    opBase->createShaderModule(avg_pool_spv, sizeof(avg_pool_spv));
        }
        else if (type == kPoolTypeMax)
        {
            opBase->createShaderModule(max_pool_spv, sizeof(max_pool_spv));
        }

		opBase->createPipeline(sizeof(PoolParam));
	}

	opBase->bindOperand(in, 0, opBase->descriptor_set);
	opBase->bindOperand(out, 1, opBase->descriptor_set);

    PoolParam param;
    param.channels   = in_shape[kShapeIdxChannel];
    param.in_height  = in_shape[kShapeIdxHeight];
    param.in_width   = in_shape[kShapeIdxWidth];
    param.out_height = out_shape[kShapeIdxHeight];
    param.out_width  = out_shape[kShapeIdxWidth];
    param.total      = out.getElementCount();

    if (inCount == 10) {
        param.padding_left   = operands[ins[1]].getScalarData<uint32_t>();
        param.padding_top    = operands[ins[3]].getScalarData<uint32_t>();
        param.stride_w     = operands[ins[5]].getScalarData<uint32_t>();
        param.stride_h     = operands[ins[6]].getScalarData<uint32_t>();
        param.filter_w = operands[ins[7]].getScalarData<uint32_t>();
        param.filter_h = operands[ins[8]].getScalarData<uint32_t>();
        //it is not used in compute shader, only support case of activtion as 0
        activation   = operands[ins[9]].getScalarData<uint32_t>();
        assert(activation == 0);
    } else {
        padding_mode = static_cast<PaddingScheme>(operands[ins[1]].getScalarData<uint32_t>());
        param.stride_w     = operands[ins[2]].getScalarData<uint32_t>();
        param.stride_h     = operands[ins[3]].getScalarData<uint32_t>();
        param.filter_w = operands[ins[4]].getScalarData<uint32_t>();
        param.filter_h = operands[ins[5]].getScalarData<uint32_t>();
        activation   = operands[ins[6]].getScalarData<uint32_t>();
        assert(activation == 0);
        calculateExplicitPadding(param.in_width, param.stride_w,
                                 param.filter_w, padding_mode,
                                 &param.padding_left);
        calculateExplicitPadding(param.in_height, param.stride_h,
                                 param.filter_h, padding_mode,
                                 &param.padding_top);
    }

    int32_t lsz_x  = config.local_size_x;
    int32_t lsz_y  = config.local_size_y;
    int32_t lsz_z  = config.local_size_z;
    int32_t item_z = 1;

    opBase->group_x = ceil(static_cast<float>(param.out_width) / lsz_x);
    opBase->group_y = ceil(static_cast<float>(param.out_height) / lsz_y);
    opBase->group_z = ceil(static_cast<float>((ceil(static_cast<float>(param.channels) / item_z))) / lsz_z);

    /* for average pool, following member is used for padded_area, hard coded as true in vkcom,
     * for max pool, it is used to indicate if mask tensor exist, we didn't support currently
     */
    if (type == kPoolTypeAvg)
    {
        param.mask_or_padded_area = 1;
    }
    else
    {
        param.mask_or_padded_area = 0;
    }

    NN_GPU_DEBUG("VkCsExecutor::doPool: param channels is %d, in height is %d, in width is %d, out height is %d, "
        "out width is %d, total is %d, stride w is %d, stride h is %d, filter w is %d, filter h is %d, padded area is %d",
        param.channels, param.in_height, param.in_width, param.out_height, param.out_width, param.total, param.stride_w,
        param.stride_h, param.filter_w, param.filter_h, param.mask_or_padded_area);

    opBase->recordCommandBuffer((void *)&param, sizeof(PoolParam));
    opBase->runCommandBuffer();

	return true;
}


bool VkCsExecutor::doAVERAGE_POOL_2D(const Operation& operation)
{
    NN_GPU_ENTRY();

    ASSERT(operation.type == OperationType::AVERAGE_POOL_2D);
    bool ret = false;

    ShaderConfig config = {8, 8, 1, 1, 1, 1};
    prepareConfig(operation, config);
    ret = doPool(operation, config, kPoolTypeAvg);

    NN_GPU_EXIT();
    return ret;
}

bool VkCsExecutor::doMAX_POOL_2D(const Operation& operation)
{
    NN_GPU_ENTRY();

    ASSERT(operation.type == OperationType::MAX_POOL_2D);
    bool ret = false;

    ShaderConfig config = {8, 8, 1, 1, 1, 1};
    prepareConfig(operation, config);
    ret = doPool(operation, config, kPoolTypeMax);

    NN_GPU_EXIT();
    return ret;
}

NAME_SPACE_STOP
