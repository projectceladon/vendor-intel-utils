#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_PREPARE_MODEL_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_PREPARE_MODEL_H

#include "hal_types.h"

NAME_SPACE_BEGIN

using time_point = std::chrono::steady_clock::time_point;

class BaseExecutor;

class PreparedModel : public IPreparedModel
{
public:
    PreparedModel(const Model& model);
    ~PreparedModel() override;
    bool initialize();
    Return<ErrorStatus> execute(const Request& request,
                                const sp<V1_0::IExecutionCallback>& callback) override;
    Return<ErrorStatus> execute_1_2(const Request& request,
                                    V1_2::MeasureTiming measure,
                                    const sp<V1_2::IExecutionCallback>& cb) override;
    Return<void> executeSynchronously(const Request& request,
                                      V1_2::MeasureTiming measure,
                                      executeSynchronously_cb cb) override;
    Return<void> configureExecutionBurst(const sp<V1_2::IBurstCallback>& callback,
                                         const MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
                                         const MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
                                         configureExecutionBurst_cb cb) override;

private:
    void asyncExecute(const Request& request, const sp<V1_0::IExecutionCallback>& callback);
    void asyncExecute_1_2(const Request& request, const sp<V1_2::IExecutionCallback>& callback);

    Model mModel;
    sp<BaseExecutor> exec;
    std::vector<std::thread> execThreads;
};

NAME_SPACE_STOP

#endif
