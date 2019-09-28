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

#define LOCAL_SZ_X 256

struct ConcatParam {
    int out_concat_axis;
    int accumulated_concat_axis;
    int concat_size;
    int total_concat_size;
    int thread_num;
};

struct ConcatShaderConfig
{
    int local_size_x;
    int local_size_y;
    int local_size_z;
    int block_height;
    int block_width;
    int block_depth;
};

bool VkCsExecutor::doCONCATENATION(const Operation& operation)
{
    NN_GPU_ENTRY();

#define BUFFER_NUM 2
    opBase->initVulkanThing(BUFFER_NUM);

    ASSERT(operation.type == OperationType::CONCATENATION);
    const hidl_vec<uint32_t>& ins  = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    if (outs.size() != 1 || ins.size() < 2) {
        return false;
    }

    int32_t numInputTensors = ins.size() - 1;
    int32_t axis            = operands[ins[numInputTensors]].getScalarData<int32_t>();
    VkOperand& firstInput   = operands[ins[0]];
    uint32_t numDims        = firstInput.getNumberOfDimensions();

    NN_OPS_CHECK(axis >= 0);
    NN_OPS_CHECK(axis < (int32_t)numDims);

    int32_t concatSize;
    bool lastaxis = (axis == (int32_t)numDims - 1);
    if (lastaxis)
    {
        concatSize = 1;
    }
    else
    {
        concatSize = firstInput.getElementCount(axis + 1);
    }

    int sum_axis = firstInput.getDimensionSize(axis);
    VkOperand& output = operands[outs[0]];

    for (int i = 1; i < numInputTensors; ++i)
    {
        VkOperand& operand = operands[ins[i]];
        assert(operand.dimNum() == numDims);
        for (int32_t d = 0; d < (int32_t)numDims; ++d)
        {
            if (d == axis)
            {
                sum_axis += operand.getDimensionSize(axis);;
            }
            else
            {
                assert(firstInput.getDimensionSize(d) == operand.getDimensionSize(d));
            }
        }
    }

    assert(output.getDimensionSize(axis) == sum_axis);
    for (int32_t d = 0; d < (int32_t)numDims; ++d)
    {
        if (d != axis)
        {
            assert(output.getDimensionSize(d) == firstInput.getDimensionSize(d));
        }
    }
    ConcatParam param;
    ConcatShaderConfig config;
    /* local_size_y and local_size_z is not set */
    config.local_size_x = LOCAL_SZ_X;
    config.block_height = 1;
    config.block_width  = 1;
    config.block_depth  = 1;

    param.out_concat_axis = sum_axis;
    param.concat_size = output.getElementCount(axis + 1);

    if (opBase->pipeline == VK_NULL_HANDLE)
    {
        opBase->createShaderModule(concat_spv, sizeof(concat_spv));
        opBase->createPipeline(sizeof(ConcatParam));
    }

    NN_GPU_DEBUG("VkCsExecutor::doCONCATENATION: param out_concat_axis is %d, concat_size is %d",
        param.out_concat_axis, param.concat_size);

    param.accumulated_concat_axis = 0;

    for (int i = 0; i < numInputTensors; i++)
    {
        NN_GPU_DEBUG("VkCsExecutor::doCONCATENATION: bind operands");

        opBase->bindOperand(operands[ins[i]], 0, opBase->descriptor_set);
        opBase->bindOperand(output, 1, opBase->descriptor_set);

        param.total_concat_size = operands[ins[i]].getElementCount(axis);
        param.thread_num = operands[ins[i]].getElementCount();

        opBase->group_x = alignSize(param.thread_num, config.local_size_x) / config.local_size_x;
        opBase->group_y = 1;
        opBase->group_z = 1;

        NN_GPU_DEBUG("VkCsExecutor::doCONCATENATION: do recordCommandBuffer");
        opBase->recordCommandBuffer((void *)&param, sizeof(ConcatParam));

        NN_GPU_DEBUG("VkCsExecutor::doCONCATENATION: do runCommandBuffer");
        opBase->runCommandBuffer();

        param.accumulated_concat_axis += operands[ins[i]].getDimensionSize(axis);
    }

    output.dump();
    NN_GPU_EXIT();
 
	return true;
}

NAME_SPACE_STOP
