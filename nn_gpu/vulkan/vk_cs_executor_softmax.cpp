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

#define LOCAL_SZ_X 8

struct SoftmaxShaderConfig
{
public:
    SoftmaxShaderConfig(int lsx) : local_size_x(lsx) {};

    int local_size_x;
};

#if 0

/* this implementation mainly follow gles solution, but this shader program need change */
struct SoftmaxParam {
    float beta;
};

/* todo: shader is not ready, vkcom softmax shader seems not match with android tensorflow */
bool VkCsExecutor::doSOFTMAX(const Operation& operation)
{
#define BUFFER_NUM 2
    opBase->initVulkanThing(BUFFER_NUM);

    ASSERT(operation.type == OperationType::SOFTMAX);
    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;
    SoftmaxParam param;
    SoftmaxShaderConfig config;

    if (outs.size() != 1 || ins.size() != 2) {
        return false;
    }

    param.beta = operands[ins[1]].getScalarData<float>();

    VkOperand& in = operands[ins[0]];
    VkOperand& output = operands[outs[0]];

    uint32_t numDim = in.getNumberOfDimensions();
    uint32_t total = in.getDimensionSize(0);   // one work item per batch
    uint32_t arraysize = 0;
    if(numDim == 2)
    {
        arraysize = in.getDimensionSize(1);
    }
    else
    {
        ASSERT(numDim == 4);
        arraysize = in.getDimensionSize(1) * in.getDimensionSize(2) * in.getDimensionSize(3);
    }

    /* local_size_y and local_size_z is not set */
    config.local_size_x = 8;

    if (opBase->pipeline == VK_NULL_HANDLE)
    {
        opBase->createShaderModule(softmax_spv, sizeof(softmax_spv));
        opBase->createPipeline(sizeof(SoftmaxParam));
    }

    opBase->bindOperand(in, 0, opBase->descriptor_set);
    opBase->bindOperand(output, 1, opBase->descriptor_set);
    opBase->group_x = alignSize(total, config.local_size_x) / config.local_size_x;
    opBase->group_y = 1;
    opBase->group_z = 1;

    opBase->recordCommandBuffer((void *)&param, sizeof(SoftmaxParam));
    opBase->runCommandBuffer();

    output.dump();

    return true;
}

#else

struct SoftmaxParam {
    int array_size;
    float beta;
    int total;
};

/* todo: shader is not ready, vkcom softmax shader seems not match with android tensorflow */
bool VkCsExecutor::doSOFTMAX(const Operation& operation)
{
    NN_GPU_ENTRY();

#define BUFFER_NUM 2
    opBase->initVulkanThing(BUFFER_NUM);

    ASSERT(operation.type == OperationType::SOFTMAX);

    const hidl_vec<uint32_t>& ins  = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    /* local_size_y and local_size_z is not set */
    SoftmaxShaderConfig config(LOCAL_SZ_X);

    if (outs.size() != 1 || ins.size() != 2)
    {
        LOGE("invalid inputs/outputs size");
        return false;
    }

    VkOperand& input  = operands[ins[0]];
    VkOperand& output = operands[outs[0]];

    int dim_num = input.getNumberOfDimensions();
    ASSERT(dim_num == 2 || dim_num == 4);

    uint32_t arraySize = input.getDimensionSize(1);

    if (dim_num == 4)
    {
        arraySize *= (input.getDimensionSize(2) * input.getDimensionSize(3));
    }

    uint32_t total = input.getDimensionSize(0);   // one work item per batch

    SoftmaxParam param;
    param.array_size = arraySize;
    param.beta       = operands[ins[1]].getScalarData<float>();
    param.total      = total;

    NN_GPU_DEBUG("VkCsExecutor::doSOFTMAX: param array_size is %d, beta is %f, total is %d",
        param.array_size, param.beta, param.total);

    if (opBase->pipeline == VK_NULL_HANDLE)
    {
        NN_GPU_DEBUG("VkCsExecutor::doSOFTMAX: do createShaderModule");
        opBase->createShaderModule(softmax_spv, sizeof(softmax_spv));

        NN_GPU_DEBUG("VkCsExecutor::doSOFTMAX: do createPipeline");
        opBase->createPipeline(sizeof(SoftmaxParam));
    }

    NN_GPU_DEBUG("VkCsExecutor::doSOFTMAX: bind operands");
    opBase->bindOperand(input, 0, opBase->descriptor_set);
    opBase->bindOperand(output, 1, opBase->descriptor_set);

    opBase->group_x = (total + config.local_size_x - 1) / config.local_size_x;
    opBase->group_y = 1;
    opBase->group_z = 1;

    NN_GPU_DEBUG("VkCsExecutor::doSOFTMAX: do recordCommandBuffer");
    opBase->recordCommandBuffer((void *)&param, sizeof(SoftmaxParam));

    NN_GPU_DEBUG("VkCsExecutor::doSOFTMAX: do runCommandBuffer");
    opBase->runCommandBuffer();

    output.dump();
    NN_GPU_EXIT();
 
    return true;
}

#endif

NAME_SPACE_STOP
