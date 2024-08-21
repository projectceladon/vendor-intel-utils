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

#include "ConfigManager.h"
#include "EvsStateControl.h"
#include "EvsVehicleListener.h"

#include <aidl/android/hardware/automotive/evs/IEvsDisplay.h>
#include <aidl/android/hardware/automotive/evs/IEvsEnumerator.h>
#include <aidl/android/hardware/automotive/vehicle/SubscribeOptions.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleGear.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/StrongPointer.h>

#include <IVhalClient.h>
#include <signal.h>
#include <stdio.h>

namespace {

using aidl::android::hardware::automotive::evs::IEvsDisplay;
using aidl::android::hardware::automotive::evs::IEvsEnumerator;
using aidl::android::hardware::automotive::vehicle::VehicleGear;
using aidl::android::hardware::automotive::vehicle::VehicleProperty;
using android::base::EqualsIgnoreCase;
using android::frameworks::automotive::vhal::ISubscriptionClient;
using android::frameworks::automotive::vhal::IVhalClient;

const char CONFIG_DEFAULT_PATH[] = "/system/etc/automotive/evs/config_intel.json";
const char CONFIG_OVERRIDE_PATH[] = "/system/etc/automotive/evs/config_override.json";

std::shared_ptr<IEvsEnumerator> pEvsService;
std::shared_ptr<IEvsDisplay> pDisplay;
EvsStateControl* pStateController;

// Helper to subscribe to Vhal notifications
bool subscribeToVHal(ISubscriptionClient* client, VehicleProperty propertyId) {
    assert(pVnet != nullptr);
    assert(listener != nullptr);

    // Register for vehicle state change callbacks we care about
    // Changes in these values are what will trigger a reconfiguration of the EVS pipeline
    std::vector<aidl::android::hardware::automotive::vehicle::SubscribeOptions> options = {
            {
                    .propId = static_cast<int32_t>(propertyId),
                    .areaIds = {},
            },
    };
    if (auto result = client->subscribe(options); !result.ok()) {
        LOG(WARNING) << "VHAL subscription for property " << static_cast<int32_t>(propertyId)
                     << " failed with error " << result.error().message();
        return false;
    }

    return true;
}

bool convertStringToFormat(const char* str, android_pixel_format_t* output) {
    bool result = true;
    if (EqualsIgnoreCase(str, "RGBA8888")) {
        *output = HAL_PIXEL_FORMAT_RGBA_8888;
    } else if (EqualsIgnoreCase(str, "YV12")) {
        *output = HAL_PIXEL_FORMAT_YV12;
    } else if (EqualsIgnoreCase(str, "NV21")) {
        *output = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    } else if (EqualsIgnoreCase(str, "YUYV")) {
        *output = HAL_PIXEL_FORMAT_YCBCR_422_I;
    } else {
        result = false;
    }

    return result;
}

}  // namespace

// Main entry point
int main(int argc, char** argv) {
    LOG(INFO) << "EVS app starting";

    // Set up default behavior, then check for command line options
    bool useVehicleHal = true;
    bool printHelp = false;
    const char* evsServiceName = "default";
    int displayId = -1;
    bool useExternalMemory = false;
    android_pixel_format_t extMemoryFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    int32_t mockGearSignal = static_cast<int32_t>(VehicleGear::GEAR_REVERSE);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            useVehicleHal = false;
        } else if (strcmp(argv[i], "--hw") == 0) {
            evsServiceName = "EvsEnumeratorHw";
        } else if (strcmp(argv[i], "--mock") == 0) {
            evsServiceName = "EvsEnumeratorHw-Mock";
        } else if (strcmp(argv[i], "--help") == 0) {
            printHelp = true;
        } else if (strcmp(argv[i], "--display") == 0) {
            displayId = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "--extmem") == 0) {
            useExternalMemory = true;
            if (i + 1 >= argc) {
                // use RGBA8888 by default
                LOG(INFO) << "External buffer format is not set.  "
                          << "RGBA8888 will be used.";
            } else {
                if (!convertStringToFormat(argv[i + 1], &extMemoryFormat)) {
                    LOG(WARNING) << "Color format string " << argv[i + 1]
                                 << " is unknown or not supported.  RGBA8888 will be used.";
                } else {
                    // move the index
                    ++i;
                }
            }
        } else if (strcmp(argv[i], "--gear") == 0) {
            // Gear signal to simulate
            if (i + 1 >= argc) {
                LOG(INFO) << "Gear signal is not set.  "
                          << "Reverse signal will be used.";
                continue;
            }
            i += 1;  // increase an index to next argument
            if (strcasecmp(argv[i], "Park") == 0) {
                mockGearSignal = static_cast<int32_t>(VehicleGear::GEAR_PARK);
            } else if (strcasecmp(argv[i], "Reverse") != 0) {
                LOG(WARNING) << "Unknown gear signal, " << argv[i] << ", is ignored "
                             << "and the reverse signal will be used instead";
            }
        } else {
            printf("Ignoring unrecognized command line arg '%s'\n", argv[i]);
            printHelp = true;
        }
    }
    if (printHelp) {
        printf("Options include:\n");
        printf("  --test\n\tDo not talk to Vehicle Hal, "
               "but simulate a given mock gear signal instead\n");
        printf("  --gear\n\tMock gear signal for the test mode.");
        printf("  Available options are Reverse and Park (case insensitive)\n");
        printf("  --hw\n\tBypass EvsManager by connecting directly to EvsEnumeratorHw\n");
        printf("  --mock\n\tConnect directly to EvsEnumeratorHw-Mock\n");
        printf("  --display\n\tSpecify the display to use.  If this is not set, the first"
               "display in config.json's list will be used.\n");
        printf("  --extmem  <format>\n\t"
               "Application allocates buffers to capture camera frames.  "
               "Available format strings are (case insensitive):\n");
        printf("\t\tRGBA8888: 4x8-bit RGBA format.  This is the default format to be used "
               "when no format is specified.\n");
        printf("\t\tYV12: YUV420 planar format with a full resolution Y plane "
               "followed by a V values, with U values last.\n");
        printf("\t\tNV21: A biplanar format with a full resolution Y plane "
               "followed by a single chrome plane with weaved V and U values.\n");
        printf("\t\tYUYV: Packed format with a half horizontal chrome resolution.  "
               "Known as YUV4:2:2.\n");

        return EXIT_FAILURE;
    }

    // Load our configuration information
    ConfigManager config;
    if (!config.initialize(CONFIG_OVERRIDE_PATH)) {
        if (!config.initialize(CONFIG_DEFAULT_PATH)) {
            LOG(ERROR) << "Missing or improper configuration for the EVS application.  Exiting.";
            return EXIT_FAILURE;
        }
    }

    // Set thread pool size to one to avoid concurrent events from the HAL.
    // This pool will handle the EvsCameraStream callbacks.
    // Note:  This _will_ run in parallel with the EvsListener run() loop below which
    // runs the application logic that reacts to the async events.
    if (!ABinderProcess_setThreadPoolMaxThreadCount(/* numThreads= */ 1)) {
        LOG(ERROR) << "Failed to confgiure the binder thread pool.";
        return EXIT_FAILURE;
    }
    ABinderProcess_startThreadPool();

    // Construct our async helper object
    std::shared_ptr<EvsVehicleListener> pEvsListener = std::make_shared<EvsVehicleListener>();

    // Get the EVS manager service
    LOG(INFO) << "Acquiring EVS Enumerator";
    std::string serviceName =
            std::string(IEvsEnumerator::descriptor) + "/" + std::string(evsServiceName);
    if (!AServiceManager_isDeclared(serviceName.c_str())) {
        LOG(ERROR) << serviceName << " is not declared. Exiting.";
        return EXIT_FAILURE;
    }

    pEvsService = IEvsEnumerator::fromBinder(
            ndk::SpAIBinder(AServiceManager_checkService(serviceName.c_str())));
    if (!pEvsService) {
        LOG(ERROR) << "Failed to get " << serviceName << ". Exiting.";
        return EXIT_FAILURE;
    }

    // Request exclusive access to the EVS display
    LOG(INFO) << "Acquiring EVS Display";

    // We'll use an available display device.
    displayId = config.setActiveDisplayId(displayId);
    if (displayId < 0) {
        PLOG(ERROR) << "EVS Display is unknown.  Exiting.";
        return EXIT_FAILURE;
    }

    if (auto status = pEvsService->openDisplay(displayId, &pDisplay); !status.isOk()) {
        LOG(ERROR) << "EVS Display unavailable.  Exiting.";
        return EXIT_FAILURE;
    }

    config.useExternalMemory(useExternalMemory);
    config.setExternalMemoryFormat(extMemoryFormat);

    // Set a mock gear signal for the test mode
    config.setMockGearSignal(mockGearSignal);

    // Connect to the Vehicle HAL so we can monitor state
    std::shared_ptr<IVhalClient> pVnet;
    if (useVehicleHal) {
        LOG(INFO) << "Connecting to Vehicle HAL";
        pVnet = IVhalClient::create();
        if (pVnet == nullptr) {
            LOG(ERROR) << "Vehicle HAL getService returned NULL.  Exiting.";
            return EXIT_FAILURE;
        } else {
            auto subscriptionClient = pVnet->getSubscriptionClient(pEvsListener);
            // Register for vehicle state change callbacks we care about
            // Changes in these values are what will trigger a reconfiguration of the EVS pipeline
            if (!subscribeToVHal(subscriptionClient.get(), VehicleProperty::GEAR_SELECTION)) {
                LOG(ERROR) << "Without gear notification, we can't support EVS.  Exiting.";
                return EXIT_FAILURE;
            }
            if (!subscribeToVHal(subscriptionClient.get(), VehicleProperty::TURN_SIGNAL_STATE)) {
                LOG(WARNING) << "Didn't get turn signal notifications, so we'll ignore those.";
            }
        }
    } else {
        LOG(WARNING) << "Test mode selected, so not talking to Vehicle HAL";
    }

    // Configure ourselves for the current vehicle state at startup
    LOG(INFO) << "Constructing state controller";
    pStateController = new EvsStateControl(pVnet, pEvsService, pDisplay, config);
    if (!pStateController->startUpdateLoop()) {
        LOG(ERROR) << "Initial configuration failed.  Exiting.";
        return EXIT_FAILURE;
    }

    // Run forever, reacting to events as necessary
    LOG(INFO) << "Entering running state";
    pEvsListener->run(pStateController);

    // In normal operation, we expect to run forever, but in some error conditions we'll quit.
    // One known example is if another process preempts our registration for our service name.
    LOG(ERROR) << "EVS Listener stopped.  Exiting.";

    return EXIT_SUCCESS;
}
