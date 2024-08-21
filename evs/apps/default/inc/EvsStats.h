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

#ifndef CAR_EVS_APP_EVSSTATS_H
#define CAR_EVS_APP_EVSSTATS_H

#include <aidl/android/frameworks/automotive/telemetry/CarData.h>
#include <aidl/android/frameworks/automotive/telemetry/ICarTelemetry.h>
#include <android/binder_status.h>
#include <utils/Mutex.h>

#include <deque>
#include <memory>

// Performs metric computations, sends to `ICarTelemetry`.
//
// Not thread-safe. Methods `startComputingFirstFrameLatency`, `finishComputingFirstFrameLatency`
// and `sendCollectedDataBlocking` must be called from the same thread.
class EvsStats final {
public:
    // Instantiates EvsStats.
    static EvsStats build();

    // Starts computing end-2-end first frame latency: from the time of the event that starts the
    // camera to the time of the display of the very first frame.
    // Call this method when an event that enables the camera occurred, e.g. gear shift to REAR.
    // Param `startTimeMillis` should be `android::uptimeMillis()`.
    void startComputingFirstFrameLatency(int64_t startTimeMillis);

    // Computes the latency and sends the data to `ICarTelemetry` if the receiving service is up.
    // Call this method when the first camera frame is displayed on the screen and don't call
    // after that unless a new computation is started.
    // Param `finishTimeMillis` should be `android::uptimeMillis()`.
    void finishComputingFirstFrameLatency(int64_t finishTimeMillis);

    // Sends the collected data to `ICarTelemetry`. Blocks for short amount of time if the service
    // is unavailable.
    void sendCollectedDataBlocking();

private:
    enum EvsStatsState {
        NOT_STARTED = -1,
    };

private:
    EvsStats(bool enabled) :
          mEnabled(enabled),
          mBinderDeathRecipient(::AIBinder_DeathRecipient_new(EvsStats::telemetryBinderDied)) {}

    // Death recipient callback that is called when ICarTelemetry dies.
    // The cookie is a pointer to a EvsStats object.
    static void telemetryBinderDied(void* cookie);

    void telemetryBinderDiedImpl();

    // Tries sending data if the receiving service is up.
    // Must be called when both `mEnabled` is true and `mCollectedData` is not empty.
    //
    // \param waitIfNotReady - if true, it can block briefly until ICarTelemetry is ready.
    void sendCollectedDataUnsafe(bool waitIfNotReady);

    // Returns the instance of ICarTelemetry.
    //
    // \param waitIfNotReady - if true, it can block briefly until ICarTelemetry is ready.
    std::shared_ptr<aidl::android::frameworks::automotive::telemetry::ICarTelemetry>
    getCarTelemetry(bool waitIfNotReady);

    std::mutex mMutex;
    bool mEnabled;
    int64_t mFirstFrameLatencyStartTimeMillis = EvsStatsState::NOT_STARTED;
    // This is a ring buffer
    std::deque<aidl::android::frameworks::automotive::telemetry::CarData> mCollectedData;
    std::shared_ptr<aidl::android::frameworks::automotive::telemetry::ICarTelemetry> mCarTelemetry;
    ndk::ScopedAIBinder_DeathRecipient mBinderDeathRecipient;
};

#endif  // CAR_EVS_APP_EVSSTATS_H
