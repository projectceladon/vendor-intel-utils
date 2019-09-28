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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_OP_BASE_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_OP_BASE_H

#include "vk_common.h"
#include "vk_operand.h"

NAME_SPACE_BEGIN

class VkOpBase
{
public:
    VkOpBase();
    virtual ~VkOpBase();

protected:
    void initVulkanThing(int buffer_num);
    void resetPipeline();
    void bindOperand(VkOperand& operand, int binding, VkDescriptorSet descriptor_set);
    void createDescriptorSetLayout(int buffer_num);
    void createDescriptorSet(int buffer_num);
    void createShaderModule(const uint32_t* spv, size_t sz);
    void createPipeline(size_t push_constants_size = 0, VkSpecializationInfo* specialization_info = 0);
    void createCommandBuffer();
    void recordCommandBuffer(void* push_constants = NULL, size_t push_constants_size = 0);
    void runCommandBuffer();
    bool computeGroupCountX(uint32_t totalThreadX, int preferLocalSizeX, int& localSizeX);
    void setGroupSize(const int gx, const int gy, const int gz);
    void rebindVkBuffer(VkOperand& operand, const int b, const int w, const int h, const int c);

    VkPipeline pipeline;
    VkCommandBuffer cmd_buffer;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    VkDevice device;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkShaderModule module;
    int group_x;
    int group_y;
    int group_z;
    std::string type;
    friend class VkCsExecutor;

private:
    bool checkGroupParam(uint32_t* localSize, uint32_t* groupCount);
};

NAME_SPACE_STOP

#endif
