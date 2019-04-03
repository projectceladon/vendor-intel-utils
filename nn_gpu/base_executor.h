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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_BASE_EXECUTOR_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_BASE_EXECUTOR_H

#include <algorithm>
#include <memory.h>
#include <string.h>

#include <utils/RefBase.h>
#include <android/log.h>
#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

#include "hal_types.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

#define UNUSED(x) (void)(x);

#define ASSERT(v)                                                               \
    do                                                                          \
    {                                                                           \
        if (!(v))                                                               \
        {                                                                       \
            ALOGE("ASSERT failed at %s:%d - %s", __FILE__, __LINE__, #v);       \
            abort();                                                            \
        }                                                                       \
    } while(0)

#define NOT_REACH_HERE ASSERT(!"should not reach here");
#define NOT_IMPLEMENTED ASSERT(!"not implemented");

#define ALIGN(size_, align) (((size_) + (align) - 1) / (align) * (align))
// Macro to check if the input parameters for operation are valid or not.
#define NN_CHECK(v)                                                     \
  do {                                                                  \
    if (!(v)) {                                                         \
      LOG(ERROR) << "NN_CHECK failed: "  << #v << ", " << __FILE__ << ":" << __LINE__ << "\n";                \
      return false;                                                     \
    }                                                                   \
  } while(0);

#define NN_CHECK_EQ(actual, expected)           \
  NN_CHECK((actual) == (expected))

#define NN_OPS_CHECK NN_CHECK


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

    virtual bool run(const Request& request) { NOT_REACH_HERE; UNUSED(request); return true;}
    static std::string getOpName(const Operation& op);
protected:
    const Model& model;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
