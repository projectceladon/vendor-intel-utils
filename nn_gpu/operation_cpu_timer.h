#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_OPERATION_CPU_TIMER_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_OPERATION_CPU_TIMER_H

#include <vector>
#include <string>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class OperationCpuTimer
{
public:
    OperationCpuTimer() : avg(0) {}
    ~OperationCpuTimer() {}
    void add(long diff) { times.push_back(diff); }
    void set(uint32_t index, const std::string& name)
    {
        opIndex = index;
        opName = name;
        size_t s = opName.length();
        while (s < 20)
        {
            s++;
            opName += " ";
        }
    }

    long getAvg()
    {
        if (times.size() == 0)
        {
            avg = 0;
        }
        else if (times.size() == 1)
        {
            avg = times[0];
        }
        else
        {
            long sum = 0;
            for (size_t i = 1; i < times.size(); ++i)
            {
                sum += times[i];
            }
            avg = sum / (times.size()-1);
        }
        return avg;
    }
    long show(long acc, float sum)
    {
        ALOGI("op %02d (%s): \t%s per run (%.2f%%, %.2f%%)",
               opIndex, opName.c_str(),
               getTimeString(avg).c_str(),
               avg*100/sum,(avg+acc)*100/sum);
        return avg + acc;
    }

    static bool sort(const OperationCpuTimer& lhs, const OperationCpuTimer& rhs)
    {
        return (lhs.avg > rhs.avg);
    }

private:
    static std::string getTimeString(long t)
    {
        if (t > 1000000)
        {
            return std::to_string(t/1000000.) + " s";
        }
        else
        {
            return std::to_string(t/1000.) + " ms";
        }
    }

    std::vector<long> times;  //holds time in us (1e-6 second)
    long avg;
    uint32_t opIndex;
    std::string opName;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
