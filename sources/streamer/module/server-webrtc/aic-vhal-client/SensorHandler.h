// Copyright (C) 2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <iostream>
#include <iterator>
#include <thread>
#include <map>
#include <mutex>
#include <libvhal/sensor_interface.h>

#define MAX_SENSOR_DATA_VAL_CNT 6

using namespace vhal::client;

class SensorHandler {
public:
    using CommandHandler = std::function<void(uint32_t cmd)>;
    SensorHandler() {}
    ~SensorHandler() {};
    SensorHandler(int instanceId, CommandHandler cmdHandler);

    /**
     * @brief Parse the sensor data received from client app and
     *        send to Server VHAL.
     * @param jsonMessage Json data received from client
     */
    void processClientMsg(const std::string &jsonMessage);

    /**
     * @brief Configure sensors in client based on data available in mSensorMap.
     */

    void configureClientSensors();

    bool getClientRequestFlag() {return mIsClientRequested;}

    void setClientRequestFlag(bool flag) {mIsClientRequested = flag;}

    enum Command {kSensorStart = 31, kSensorStop = 32};

    enum Orientation {landscapeOrientation = 0, portraitOrientation = 1} mOrientation;

    static std::string sensorStartMsg;

    static std::string sensorStopMsg;

private:
    /**
     * @brief processVHALCtrlMsg Callback function registers to libvhal-client
     *        to receive all the sensor control packets from AIC sensor VHAL server.
     * @param ctrlPkt contains sensor tyoe and ssmapling period and
     *        enbale/disable flag
     */
    void processVHALCtrlMsg(const SensorInterface::CtrlPacket& ctrlPkt);

    /**
     * @brief Send sensor's start/stop command to client app
     * @param sensorType sensor type defined in vhal::client::sensor_type_t
     * @enabled True, If sensor is enabled.
     *          False, If sensor is disabled.
     * @samplingPeriod_ms, The delay(milli seconds) at which sensor data samples are expected.
     */
    void sendCmdToClient(int32_t sensorType, bool enabled, int32_t samplingPeriod_ms);

    /**
     * @brief update x and y motion sensor axis to make the device rotation works for tablet mode.
     * @param event Sensor event defined in SensorInterface::SensorDataPacket
     */
    void updateCoordinates(SensorInterface::SensorDataPacket &event);

    /**
     * Get current time in nano secnds
     */
    int64_t now_ns();

    /**
     *  Client instance ID
     */
    int mInstanceId;

    /**
     * Flag to check if any sensor request message received from Client app.
     */
    bool mIsClientRequested;

    /**
     * Map to store each sensor enable/disable status
     * <sensor type, <enable/disable flag, sensor sampling period> >
     */
    std::map<int32_t, std::pair<bool, int32_t>> mSensorMap;

    /**
     * Lock to guard mSensorMap
     */
    std::mutex sensorMapMutex;

    /**
     * CommandHandler instance to send sensor control messages to client.
     */
    CommandHandler mCmdHandler;

    /**
     * SensorInterface LibVHAL client's sensor interface to communicate
     *                 with AIC Sensor server VHAL.
     */
    std::unique_ptr<SensorInterface> sensorHALIface;
};
