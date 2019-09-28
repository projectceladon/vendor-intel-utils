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

#include "vk_common.h"
#include "vk_wrapper.h"
#include "vk_op_base.h"

NAME_SPACE_BEGIN

VkOpBase::VkOpBase(): group_x(0), group_y(0), group_z(0)
{
    NN_GPU_CALL();
    device = kDevice;
    pipeline = VK_NULL_HANDLE;
    cmd_buffer = VK_NULL_HANDLE;
    descriptor_pool = VK_NULL_HANDLE;
    descriptor_set = VK_NULL_HANDLE;
    descriptor_set_layout = VK_NULL_HANDLE;
    pipeline_layout = VK_NULL_HANDLE;
    module = VK_NULL_HANDLE;
}

VkOpBase::~VkOpBase()
{
    NN_GPU_CALL();
    vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
    resetPipeline();
}

void VkOpBase::resetPipeline()
{
    NN_GPU_ENTRY();
    if (module != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, module, NULL);
        module = VK_NULL_HANDLE;
    }
    if (pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, pipeline, NULL);
        pipeline = VK_NULL_HANDLE;
    }
    if (pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipeline_layout, NULL);
        pipeline_layout = VK_NULL_HANDLE;
    }
    NN_GPU_EXIT();
}

void VkOpBase::initVulkanThing(int buffer_num)
{
    NN_GPU_ENTRY();
    createDescriptorSetLayout(buffer_num);
    createDescriptorSet(buffer_num);
    createCommandBuffer();
    NN_GPU_EXIT();
}

void VkOpBase::bindOperand(VkOperand& operand, int binding, VkDescriptorSet descriptor_set)
{
    NN_GPU_ENTRY();
    VkDescriptorBufferInfo desc_buffer_info = {};
    desc_buffer_info.buffer = operand.getVkBuffer();
    desc_buffer_info.offset = 0;
    desc_buffer_info.range = operand.size();

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = descriptor_set;
    write_descriptor_set.dstBinding = binding;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set.pBufferInfo = &desc_buffer_info;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
    NN_GPU_EXIT();
}

void VkOpBase::createDescriptorSetLayout(int buffer_num)
{
    NN_GPU_ENTRY();
    std::unique_ptr<VkDescriptorSetLayoutBinding[]> bindings(new VkDescriptorSetLayoutBinding[buffer_num]);
    for (int i = 0; i < buffer_num; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = buffer_num;
    info.pBindings = buffer_num ? bindings.get() : nullptr;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &info, NULL, &descriptor_set_layout));
    NN_GPU_EXIT();
}

void VkOpBase::createDescriptorSet(int buffer_num)
{
    NN_GPU_ENTRY();
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = buffer_num;

    VkDescriptorPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets = 1;
    info.poolSizeCount = 1;
    info.pPoolSizes = &pool_size;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &info, NULL, &descriptor_pool));

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &descriptor_set_layout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocate_info, &descriptor_set));
    NN_GPU_EXIT();
}

void VkOpBase::createShaderModule(const uint32_t* spv, size_t sz)
{
    NN_GPU_ENTRY();
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    if (spv)
    {
        create_info.pCode = spv;
        create_info.codeSize = sz;
    }
    VK_CHECK_RESULT(vkCreateShaderModule(device, &create_info, NULL, &module));
    NN_GPU_EXIT();
}

void VkOpBase::createPipeline(size_t push_constants_size, VkSpecializationInfo* specialization_info)
{
    NN_GPU_ENTRY();
    // create pipeline
    VkPipelineShaderStageCreateInfo stage_create_info = {};
    stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module = module;
    stage_create_info.pName = "main";
	stage_create_info.pSpecializationInfo = specialization_info;
    VkPushConstantRange push_constant_ranges[1] = {};
    push_constant_ranges[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant_ranges[0].offset = 0;
    push_constant_ranges[0].size = push_constants_size;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (push_constants_size != 0)
    {
        pipeline_layout_create_info.pushConstantRangeCount = 1;
        pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges;
    }
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipeline_layout_create_info,
                                           NULL, &pipeline_layout));

    VkComputePipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = stage_create_info;
    pipeline_create_info.layout = pipeline_layout;
    VK_CHECK_RESULT(vkCreateComputePipelines(device, VK_NULL_HANDLE,
                                             1, &pipeline_create_info,
                                             NULL, &pipeline));
    NN_GPU_EXIT();
}

void VkOpBase::createCommandBuffer()
{
    NN_GPU_ENTRY();
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = kCmdPool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &info, &cmd_buffer));
    NN_GPU_EXIT();
}

void VkOpBase::recordCommandBuffer(void* push_constants, size_t push_constants_size)
{
    NN_GPU_ENTRY();
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_RESULT(vkBeginCommandBuffer(cmd_buffer, &beginInfo));
    if (push_constants)
        vkCmdPushConstants(cmd_buffer, pipeline_layout,
                           VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           push_constants_size, push_constants);
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    vkCmdDispatch(cmd_buffer, group_x, group_y, group_z);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmd_buffer));
    NN_GPU_EXIT();
}

void VkOpBase::runCommandBuffer()
{
    NN_GPU_ENTRY();
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;

    VkFence fence;
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = 0;

    VK_CHECK_RESULT(vkCreateFence(device, &fence_create_info, NULL, &fence));
    {
        VK_CHECK_RESULT(vkQueueSubmit(kQueue, 1, &submit_info, fence));
    }
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000));
    vkDestroyFence(device, fence, NULL);
    NN_GPU_EXIT();
}

bool VkOpBase::checkGroupParam(uint32_t* localSize, uint32_t* groupCount)
{
    NN_GPU_CALL();
    NN_GPU_DEBUG("limit invocations is %d, group size limit is %d, %d, %d, group count is %d, %d, %d",
        kDeviceProps.limits.maxComputeWorkGroupInvocations,
        kDeviceProps.limits.maxComputeWorkGroupSize[0],
        kDeviceProps.limits.maxComputeWorkGroupSize[1],
        kDeviceProps.limits.maxComputeWorkGroupSize[2],
        kDeviceProps.limits.maxComputeWorkGroupCount[0],
        kDeviceProps.limits.maxComputeWorkGroupCount[1],
        kDeviceProps.limits.maxComputeWorkGroupCount[2]); 
    return (localSize[0] * localSize[1] * localSize[2] < kDeviceProps.limits.maxComputeWorkGroupInvocations &&
            localSize[0] < kDeviceProps.limits.maxComputeWorkGroupSize[0] &&
            localSize[1] < kDeviceProps.limits.maxComputeWorkGroupSize[1] &&
            localSize[2] < kDeviceProps.limits.maxComputeWorkGroupSize[2] &&
            groupCount[0] < kDeviceProps.limits.maxComputeWorkGroupCount[0] &&
            groupCount[1] < kDeviceProps.limits.maxComputeWorkGroupCount[1] &&
            groupCount[2] < kDeviceProps.limits.maxComputeWorkGroupCount[2]);
}

bool VkOpBase::computeGroupCountX(uint32_t totalThreadX, int preferLocalSizeX, int& localSizeX)
{
    NN_GPU_ENTRY();
    uint32_t lsz[3] = {1, 1, 1};
    uint32_t groupCount[3] = {1, 1, 1};

    lsz[0] = preferLocalSizeX;
    groupCount[0] = (totalThreadX + lsz[0] - 1) / lsz[0];
    if (checkGroupParam(lsz, groupCount))
    {
        localSizeX = lsz[0];
        group_x = groupCount[0];
        NN_GPU_DEBUG("return true for group_x %d", group_x);
        return true;
    }

    for (uint32_t localX = 1; localX < kDeviceProps.limits.maxComputeWorkGroupSize[0]; localX *= 2)
    {
        lsz[0] = localX;
        groupCount[0] = (totalThreadX + lsz[0] - 1) / lsz[0];
        if (checkGroupParam(lsz, groupCount))
        {
            localSizeX = lsz[0];
            group_x = groupCount[0];
            return true;
        }
    }
    assert(0);

    return false;
}

void VkOpBase::setGroupSize(const int gx, const int gy, const int gz)
{
    group_x = gx;
    group_y = gy;
    group_z = gz;
}

void VkOpBase::rebindVkBuffer(VkOperand& operand, const int b, const int w, const int h, const int c)
{
    operand.reset(b, w, h, c);
    return;
}

NAME_SPACE_STOP
