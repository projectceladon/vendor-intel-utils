/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "EvsStats.h"

#include "packages/services/Car/cpp/telemetry/proto/evs.pb.h"

#include <aidl/android/frameworks/automotive/telemetry/CarData.h>
#include <aidl/android/frameworks/automotive/telemetry/ICarTelemetry.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <utils/SystemClock.h>

#include <vector>

namespace {

using ::aidl::android::frameworks::automotive::telemetry::CarData;
using ::aidl::android::frameworks::automotive::telemetry::ICarTelemetry;
using ::android::automotive::telemetry::EvsFirstFrameLatency;

// Name of ICarTelemetry service that consumes RVC latency metrics.
constexpr const char kCarTelemetryServiceName[] =
        "android.frameworks.automotive.telemetry.ICarTelemetry/default";

const int kCollectedDataSizeLimit = 200;  // arbitrary chosen
// 10kb limit is imposed by ICarTelemetry, the limit if only for
// CarData.content vector.
const int kCarTelemetryMaxDataSizePerWrite = 10 * 1024;

// Defined in packages/services/Car/cpp/telemetry/proto/CarData.proto
const int kEvsFirstFrameLatencyId = 1;
}  // namespace

// static
EvsStats EvsStats::build() {
    // No need to enable stats if ICarTelemetry is not available.
    bool enabled = false;
    android::sp<android::IServiceManager> mgr = android::defaultServiceManager();
    if (mgr) {
        android::Vector<android::String16> services = mgr->listServices();
        enabled = std::find(services.begin(), services.end(),
                            android::String16(kCarTelemetryServiceName)) != services.end();
    }

    if (!enabled) {
        LOG(DEBUG) << "Telemetry service is not available.";
    }
    return EvsStats(enabled);
}

void EvsStats::startComputingFirstFrameLatency(int64_t startTimeMillis) {
    mFirstFrameLatencyStartTimeMillis = startTimeMillis;
}

void EvsStats::finishComputingFirstFrameLatency(int64_t finishTimeMillis) {
    if (!mEnabled) {
        return;
    }
    if (mFirstFrameLatencyStartTimeMillis == EvsStatsState::NOT_STARTED) {
        LOG(WARNING) << __func__ << "EvsStats received finishComputingFirstFrameLatency, but "
                     << "startComputingFirstFrameLatency was not called before.";
        return;
    }
    auto firstFrameLatencyMillis = finishTimeMillis - mFirstFrameLatencyStartTimeMillis;
    mFirstFrameLatencyStartTimeMillis = EvsStatsState::NOT_STARTED;

    LOG(DEBUG) << __func__ << ": firstFrameLatencyMillis = " << firstFrameLatencyMillis;

    EvsFirstFrameLatency latency;
    latency.set_start_timestamp_millis(mFirstFrameLatencyStartTimeMillis);
    latency.set_latency_millis(firstFrameLatencyMillis);
    std::vector<uint8_t> bytes(latency.ByteSizeLong());
    latency.SerializeToArray(&bytes[0], latency.ByteSizeLong());
    CarData msg;
    msg.id = kEvsFirstFrameLatencyId;
    msg.content = std::move(bytes);

    if (msg.content.size() > kCarTelemetryMaxDataSizePerWrite) {
        LOG(WARNING) << __func__ << "finishComputingFirstFrameLatency is trying to store data "
                     << "with size " << msg.content.size() << ", which is larger than allowed "
                     << kCarTelemetryMaxDataSizePerWrite;
        return;
    }

    mCollectedData.push_back(msg);

    while (mCollectedData.size() > kCollectedDataSizeLimit) {
        mCollectedData.pop_front();
    }

    sendCollectedDataUnsafe(/* waitIfNotReady= */ false);
}

void EvsStats::sendCollectedDataBlocking() {
    if (!mEnabled || mCollectedData.empty()) {
        return;
    }
    sendCollectedDataUnsafe(/* waitIfNotReady= */ true);
}

std::shared_ptr<ICarTelemetry> EvsStats::getCarTelemetry(bool waitIfNotReady) {
    {
        const std::scoped_lock<std::mutex> lock(mMutex);
        if (mCarTelemetry != nullptr) {
            return mCarTelemetry;
        }
    }

    AIBinder* binder;
    if (waitIfNotReady) {
        binder = ::AServiceManager_waitForService(kCarTelemetryServiceName);
    } else {
        binder = ::AServiceManager_checkService(kCarTelemetryServiceName);
    }

    if (binder == nullptr) {
        LOG(WARNING) << __func__ << ": ICarTelemetry is not ready";
        return nullptr;
    }

    const std::scoped_lock<std::mutex> lock(mMutex);  // locks until the end of the method
    mCarTelemetry = ICarTelemetry::fromBinder(ndk::SpAIBinder(binder));
    if (!mCarTelemetry) {
        LOG(WARNING) << "CarTelemetry service is not available.";
        return nullptr;
    }

    auto status = ndk::ScopedAStatus::fromStatus(
            ::AIBinder_linkToDeath(mCarTelemetry->asBinder().get(), mBinderDeathRecipient.get(),
                                   this));
    if (!status.isOk()) {
        LOG(WARNING) << __func__
                     << ": Failed to linkToDeath, continuing anyway: " << status.getMessage();
    }
    return mCarTelemetry;
}

void EvsStats::sendCollectedDataUnsafe(bool waitIfNotReady) {
    std::shared_ptr<ICarTelemetry> telemetry = getCarTelemetry(waitIfNotReady);
    if (telemetry == nullptr) {
        LOG(INFO) << __func__ << ": CarTelemetry is not ready, ignoring";
        return;
    }

    // Send data chunk by chnk, because Binder has transfer data size limit.
    // Adds the oldest elements to `sendingData` and tries to push ICarTelemetryService.
    // If successful, erases then from `mCollectedData`, otherwise leaves them there to try again
    // later.
    while (!mCollectedData.empty()) {
        int sendingDataSizeBytes = 0;
        std::vector<CarData> sendingData;
        auto it = mCollectedData.begin();
        while (it != mCollectedData.end()) {
            sendingDataSizeBytes += it->content.size();
            if (sendingDataSizeBytes > kCarTelemetryMaxDataSizePerWrite) {
                break;
            }
            sendingData.push_back(*it);
            it++;
        }
        ndk::ScopedAStatus status = telemetry->write(sendingData);
        if (!status.isOk()) {
            LOG(WARNING) << __func__
                         << "Failed to write data to ICarTelemetry: " << status.getMessage();
            return;
        }
        mCollectedData.erase(mCollectedData.begin(), it);
    }
}

// Removes the listener if its binder dies.
void EvsStats::telemetryBinderDiedImpl() {
    LOG(WARNING) << __func__ << "ICarTelemetry service died, resetting the state";
    const std::scoped_lock<std::mutex> lock(mMutex);
    mCarTelemetry = nullptr;
}

// static
void EvsStats::telemetryBinderDied(void* cookie) {
    // We expect the pointer to be alive as there is only a single instance of
    // EvsStats and if EvsStats is destructed, then the whole evs_app should be dead too.
    auto thiz = static_cast<EvsStats*>(cookie);
    thiz->telemetryBinderDiedImpl();
}
