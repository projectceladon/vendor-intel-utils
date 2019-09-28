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

bool VkCsExecutor::doRESHAPE(const Operation& operation)
{
    NN_GPU_ENTRY();

    ASSERT(operation.type == OperationType::RESHAPE);

#define BUFFER_NUM 2
    opBase->initVulkanThing(BUFFER_NUM);

    const hidl_vec<uint32_t>& ins  = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;

    VkOperand& input  = operands[ins[0]];
    VkOperand& output = operands[outs[0]];

    output.shareGpuStorage(input);

    Shape in_shape  = input.getShape();
    Shape out_shape = output.getShape();

    NN_GPU_DEBUG("VkCsExecutor::doRESHAPE: in_h is %d, in_w is %d, out_h is %d, out_w is %d",
        in_shape[kShapeIdxHeight], in_shape[kShapeIdxWidth], out_shape[kShapeIdxHeight], out_shape[kShapeIdxWidth]);

    NN_GPU_EXIT();

    return true;
}

NAME_SPACE_STOP
