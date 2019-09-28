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

struct SpecializationConst {
    int lsz_x;
    int activation;
    int broadcast;
    int type; 
};

struct PushConst{
    uint32_t total_thread;
    int round;
};

enum OpElewiseType { kElewiseTypeAdd, kElewiseTypeMul, kElewiseTypeNum };

bool VkCsExecutor::doADD(const Operation& operation)
{
    NN_GPU_CALL();
	return doEleWise(operation, kElewiseTypeAdd);
}

bool VkCsExecutor::doMUL(const Operation& operation)
{
    NN_GPU_CALL();
	return doEleWise(operation, kElewiseTypeMul);
}

bool VkCsExecutor::doEleWise(const Operation& operation, const int type)
{
    NN_GPU_ENTRY();

#define BUFFER_NUM 3
    opBase->initVulkanThing(BUFFER_NUM);

    const hidl_vec<uint32_t>& ins = operation.inputs;
    const hidl_vec<uint32_t>& outs = operation.outputs;
    const size_t inCount = ins.size();
	ASSERT(inCount == 3);

    VkOperand& in0 = operands[ins[0]];
    VkOperand& in1 = operands[ins[1]];
    VkOperand& in2 = operands[ins[2]];
    VkOperand& out = operands[outs[0]];

	Shape in0_shape = in0.getShape();
	Shape in1_shape = in1.getShape();
	Shape out_shape = out.getShape();

     // broadcast
    int broadcast = 1;
    int in0_bind  = 0;
    int in1_bind  = 1;

    if (in0.getElementCount() == in1.getElementCount())
    {
        broadcast = 0;
    }

    if (in0.getElementCount() < in1.getElementCount())
    {
        in0_bind = 1;
        in1_bind = 0;
    }

	uint32_t total_thread = in0.getElementCount();
	int local_size_x = 8;
	opBase->computeGroupCountX(total_thread, local_size_x, local_size_x);
	opBase->group_y = 1;
	opBase->group_z = 1;

	int activation		 = in2.getScalarData<int>();

    NN_GPU_DEBUG("VkCsExecutor::doEleWise: operation type is %d, operands index of in0, in1 and in2 is %d, %d, %d,"
        "index of out is %d, activation is %d, group_x is %d, group_y is %d, group_z is %d, total_thread is %d, broadcast is %d",
        type, ins[0], ins[1], ins[2], outs[0],
        activation, opBase->group_x, opBase->group_y, opBase->group_z, total_thread, broadcast);

	if (opBase->pipeline == VK_NULL_HANDLE)
	{
		opBase->createShaderModule(elewise_spv, sizeof(elewise_spv));

		SpecializationConst spec_const = {
			local_size_x,
			activation,
			broadcast,
			type
		};
#define SPECIALIZATION_CONST_NUM 4
		VkSpecializationMapEntry entry[SPECIALIZATION_CONST_NUM];
		SET_SPEC_CONST_ENTRY(entry[0], 0, offsetof(SpecializationConst, lsz_x), sizeof(int));
		SET_SPEC_CONST_ENTRY(entry[1], 1, offsetof(SpecializationConst, activation), sizeof(int));
		SET_SPEC_CONST_ENTRY(entry[2], 2, offsetof(SpecializationConst, broadcast), sizeof(int));
		SET_SPEC_CONST_ENTRY(entry[3], 3, offsetof(SpecializationConst, type), sizeof(int));

		VkSpecializationInfo spec_info;
		spec_info.mapEntryCount = SPECIALIZATION_CONST_NUM;
		spec_info.pMapEntries = entry;
		spec_info.dataSize = sizeof(spec_const);
		spec_info.pData = &spec_const;

		opBase->createPipeline(sizeof(PushConst), &spec_info);
	}

	opBase->bindOperand(in0, in0_bind, opBase->descriptor_set);
	opBase->bindOperand(in1, in1_bind, opBase->descriptor_set);
	opBase->bindOperand(out, 2, opBase->descriptor_set);

    PushConst push_const = {total_thread, std::min(in0.getElementCount(), in1.getElementCount())};

    NN_GPU_DEBUG("VkCsExecutor::doEleWise: do recordCommandBuffer");
    opBase->recordCommandBuffer((void *)&push_const, sizeof(PushConst));

    NN_GPU_DEBUG("VkCsExecutor::doEleWise: do runCommandBuffer");
	opBase->runCommandBuffer();

    out.dump();
    NN_GPU_EXIT();
	return true;
}

NAME_SPACE_STOP
