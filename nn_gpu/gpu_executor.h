#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GPU_EXECUTOR_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_GPU_EXECUTOR_H

#include "base_executor.h"

NAME_SPACE_BEGIN

enum PaddingScheme
{
    kPaddingUnknown = 0,
    kPaddingSame = 1,
    kPaddingValid = 2,
};

inline void calculateExplicitPadding(int32_t in_size, int32_t stride,
                                     int32_t filter_size, int32_t padding_implicit,
                                     int32_t* padding_head, int32_t* padding_tail) {
    *padding_head = 0;
    *padding_tail = 0;

    if (padding_implicit == kPaddingSame) {
        int32_t out_size = (in_size + stride - 1) / stride;
        int32_t tmp = (out_size - 1) * stride + filter_size;
        if (tmp > in_size) {
            *padding_head = (tmp - in_size) / 2;
            *padding_tail = (tmp - in_size) - *padding_head;
        }
    }
}

class GpuExecutor : public BaseExecutor
{
public:
    GpuExecutor(const Model& model) : BaseExecutor(model) {}
    ~GpuExecutor() override {}
};

NAME_SPACE_STOP

#endif
