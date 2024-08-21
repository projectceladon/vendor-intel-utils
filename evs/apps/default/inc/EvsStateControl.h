/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef CAR_EVS_APP_EVSSTATECONTROL_H
#define CAR_EVS_APP_EVSSTATECONTROL_H

#include "ConfigManager.h"
#include "EvsStats.h"
#include "RenderBase.h"
#include "StreamHandler.h"

#include <aidl/android/hardware/automotive/evs/CameraDesc.h>
#include <aidl/android/hardware/automotive/evs/IEvsCamera.h>
#include <aidl/android/hardware/automotive/evs/IEvsDisplay.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropValues.h>

#include <IVhalClient.h>

#include <thread>

/*
 * This class runs the main update loop for the EVS application.  It will sleep when it has
 * nothing to do.  It provides a thread safe way for other threads to wake it and pass commands
 * to it.
 */
class EvsStateControl final {
public:
    EvsStateControl(
            std::shared_ptr<android::frameworks::automotive::vhal::IVhalClient> pVnet,
            std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> pEvs,
            const std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsDisplay>& pDisplay,
            const ConfigManager& config);

    enum State {
        OFF = 0,
        REVERSE,
        LEFT,
        RIGHT,
        PARKING,
        NUM_STATES  // Must come last
    };

    enum class Op {
        EXIT,
        CHECK_VEHICLE_STATE,
        TOUCH_EVENT,
    };

    struct Command {
        Op operation;
        uint32_t arg1;
        uint32_t arg2;
    };

    // This spawns a new thread that is expected to run continuously
    bool startUpdateLoop();

    // This stops a rendering thread
    void terminateUpdateLoop();

    // Safe to be called from other threads
    void postCommand(const Command& cmd, bool clear = false);

private:
    void updateLoop();
    android::frameworks::automotive::vhal::ErrorCode invokeGet(
            aidl::android::hardware::automotive::vehicle::VehiclePropValue* pRequestedPropValue);
    bool selectStateForCurrentConditions();
    bool configureEvsPipeline(State desiredState);  // Only call from one thread!

    std::shared_ptr<android::frameworks::automotive::vhal::IVhalClient> mVehicle;
    std::shared_ptr<aidl::android::hardware::automotive::evs::IEvsEnumerator> mEvs;
    std::weak_ptr<aidl::android::hardware::automotive::evs::IEvsDisplay> mDisplay;
    const ConfigManager& mConfig;

    aidl::android::hardware::automotive::vehicle::VehiclePropValue mGearValue;
    aidl::android::hardware::automotive::vehicle::VehiclePropValue mTurnSignalValue;

    State mCurrentState = OFF;

    // mCameraList is a redundant storage for camera device info, which is also
    // stored in mCameraDescList and, however, not removed for backward
    // compatibility.
    std::vector<ConfigManager::CameraInfo> mCameraList[NUM_STATES];
    std::unique_ptr<RenderBase> mCurrentRenderer;
    std::unique_ptr<RenderBase> mDesiredRenderer;
    std::vector<aidl::android::hardware::automotive::evs::CameraDesc> mCameraDescList[NUM_STATES];

    std::thread mRenderThread;  // The thread that runs the main rendering loop

    // Other threads may want to spur us into action, so we provide a thread safe way to do that
    std::mutex mLock;
    std::condition_variable mWakeSignal;
    std::queue<Command> mCommandQueue;

    EvsStats mEvsStats;  // Not thread-safe

    // True if the first frame displayed on the mCurrentRenderer. Resets to false when
    // mCurrentRenderer changes.
    bool mFirstFrameIsDisplayed;
};

#endif  // CAR_EVS_APP_EVSSTATECONTROL_H
