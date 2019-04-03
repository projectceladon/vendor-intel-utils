#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_PREPARE_MODEL_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_PREPARE_MODEL_H

#include "hal_types.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

class BaseExecutor;

class PreparedModel : public IPreparedModel
{
public:
    PreparedModel(const Model& model);
    ~PreparedModel() override;
    bool initialize();
    Return<ErrorStatus> execute(const Request& request,
                                const sp<IExecutionCallback>& callback) override;

private:
    void asyncExecute(const Request& request, const sp<IExecutionCallback>& callback);

    Model mModel;
    sp<BaseExecutor> exec;
    std::vector<std::thread> execThreads;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif
