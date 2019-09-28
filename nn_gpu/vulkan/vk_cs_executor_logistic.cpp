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

#define LOCAL_SZ_X 8

struct LogisticShaderConfig
{
public:
    LogisticShaderConfig(int lsx) : local_size_x(lsx) {};
public:
    int local_size_x;
};

struct LogisticParam
{
public:
    LogisticParam(int t) : total(t) {};
public:
    int total;
};


bool VkCsExecutor::doLOGISTIC(const Operation& operation)
{
    NN_GPU_ENTRY();

    ASSERT(operation.type == OperationType::LOGISTIC);

#define BUFFER_NUM 2
    opBase->initVulkanThing(BUFFER_NUM);

    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    VkOperand& input  = operands[ins[0]];
    VkOperand& output = operands[outs[0]];

    int total = input.getElementCount();

    LogisticShaderConfig config(LOCAL_SZ_X);
    LogisticParam param(total);

    if (opBase->pipeline == VK_NULL_HANDLE)
    {
        NN_GPU_DEBUG("VkCsExecutor::doLOGISTIC: run createShaderModule");
        opBase->createShaderModule(logistic_spv, sizeof(logistic_spv));

        NN_GPU_DEBUG("VkCsExecutor::doLOGISTIC: run createPipeline");
        opBase->createPipeline(sizeof(LogisticParam));
    }

    NN_GPU_DEBUG("VkCsExecutor::doLOGISTIC: bind operands");
    opBase->bindOperand(input, 0, opBase->descriptor_set);
    opBase->bindOperand(output, 1, opBase->descriptor_set);

    opBase->group_x = (total + config.local_size_x - 1) / config.local_size_x;
    opBase->group_y = 1;
    opBase->group_z = 1;

    NN_GPU_DEBUG("VkCsExecutor::doLOGISTIC: group_x is %d, group_y is %d, group_z is %d",
        opBase->group_x, opBase->group_y, opBase->group_z);

    NN_GPU_DEBUG("VkCsExecutor::doLOGISTIC: do recordCommandBuffer");
    opBase->recordCommandBuffer((void *)&param, sizeof(LogisticParam));

    NN_GPU_DEBUG("VkCsExecutor::doLOGISTIC: do runCommandBuffer");
    opBase->runCommandBuffer();

    NN_GPU_EXIT();

    return true;
}


NAME_SPACE_STOP
