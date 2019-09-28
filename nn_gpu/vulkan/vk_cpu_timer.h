#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_CPU_TIMER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_VK_CPU_TIMER_H

#include <sys/time.h>
#include <iostream>
#include <cutils/properties.h>

#include "operation_cpu_timer.h"

NAME_SPACE_BEGIN

class VkCpuTimer
{
public:
    VkCpuTimer(const std::string& n) : start(0), name(n), opTimer(nullptr) { init(); }
    VkCpuTimer(OperationCpuTimer* t) : start(0), opTimer(t) { init(); }
    ~VkCpuTimer() { deinit(); }
private:
    VkCpuTimer();
    void init()
    {
        char prop[PROPERTY_VALUE_MAX] = "0";
        property_get("nn.gpgpu.timer", prop, "0");
        // the property only effects operation statistics,
        // assume other cases are temp and always need the data
        if (prop[0] != '1' && opTimer != nullptr)
        {
            return;
        }

        struct timeval val;
        gettimeofday(&val, NULL);
        //usec is microsecond (1e-6 second), while ms is millisecond (1e-3 second)
        start = val.tv_sec*1000000 + val.tv_usec;
    }
    void deinit()
    {
        if (start == 0)
        {
            return;
        }

        //glFinish();

        struct timeval val;
        gettimeofday(&val, NULL);
        long end = val.tv_sec*1000000 + val.tv_usec;
        long diff = end - start;

        if (name.size() > 0)
        {
            std::string msg = name + ": " + getTime(diff);
            NN_GPU_PERF("%s", msg.c_str());
        }
        else
        {
            ASSERT(opTimer != nullptr);
            opTimer->add(diff);
        }
    }
    static std::string getTime(long diff)
    {
        if (diff > 1000000)
        {
            return std::to_string(diff/1000000.) + " s";
        }
        else
        {
            return std::to_string(diff/1000.) + " ms";
        }
    }
    long start;
    std::string name;
    OperationCpuTimer* opTimer;
};

NAME_SPACE_STOP

#endif
