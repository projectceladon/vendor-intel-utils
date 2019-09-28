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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_COMMON_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_COMMON_H

#include <vulkan/vulkan.h>
#include "../hal_types.h"
#include "gpu_executor.h"

NAME_SPACE_BEGIN

extern VkPhysicalDevice kPhysicalDevice;
extern VkPhysicalDeviceProperties kDeviceProps;
extern VkDevice kDevice;
extern VkQueue kQueue;
extern VkCommandPool kCmdPool;

/* todo: change to conv, padding top/left is 1/2 padding_size, is it right? */
inline void calculateExplicitPadding(int32_t in_size, int32_t stride,
                                     int32_t filter_size, PaddingScheme padding_mode,
                                     int32_t* padding_size) {
    *padding_size = 0;

    if (padding_mode == kPaddingSame) {
        int32_t out_size = (in_size + stride - 1) / stride;
        int32_t tmp = (out_size - 1) * stride + filter_size;
        if (tmp > in_size)
            *padding_size = (tmp - in_size) / 2;
    }
}

enum ShapeIdx
{
    kShapeIdxBatch = 0,
    kShapeIdxHeight,
    kShapeIdxWidth,
    kShapeIdxChannel,
};

#define alignSize(sz, align) (((sz) + (align) - 1) / (align) * (align))

#define SET_SPEC_CONST_ENTRY(entry, id_, offset_, size_) \
    entry.constantID = id_; \
    entry.offset = offset_; \
    entry.size = size_;

NAME_SPACE_STOP

#endif
