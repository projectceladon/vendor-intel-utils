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
 * Author: Guo Yejun <yejun.guo@intel.com>
 */

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_BASE_EXECUTOR_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_BASE_EXECUTOR_H

#include <algorithm>
#include <memory.h>
#include <string.h>

#include <utils/RefBase.h>
#include <hidl/LegacySupport.h>
#include <thread>

#include "hal_types.h"

NAME_SPACE_BEGIN

#define UNUSED(x) (void)(x);

#define NOT_REACH_HERE ASSERT(!"should not reach here");
#define NOT_IMPLEMENTED ASSERT(!"not implemented");

#define ALIGN(size_, align) (((size_) + (align) - 1) / (align) * (align))

inline size_t getSizeFromInts(int lower, int higher) {
    return (uint32_t)(lower) + ((uint64_t)(uint32_t)(higher) << 32);
}

class BaseExecutor : public RefBase
{
public:
    BaseExecutor(const Model& m) : model(m) { UNUSED(model); }
    virtual ~BaseExecutor() {}

    virtual bool initPerModel() { NOT_REACH_HERE; return true; }
    virtual void deinitPerModel() { NOT_REACH_HERE; }

    // why not merge these two functions into function run?
    // just for a clear logic for cleanup(deinit) in case something wrong within run
    virtual bool initPerExecThread() { NOT_REACH_HERE; return true; }
    virtual void deinitPerExecThread() { NOT_REACH_HERE; }

    virtual bool run(const Request& request) { UNUSED(request); NOT_REACH_HERE; return true; }
    virtual std::string getOpName(const Operation& op);
protected:
    const Model& model;
};

NAME_SPACE_STOP

#endif
