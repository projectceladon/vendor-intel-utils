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

#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <string>
#include <unistd.h>
#include "SensorHandler.h"
#include "nlohmann/json.hpp"
#include "ga-common.h"
#include "ga-conf.h"

#define TAG "SensorHandler"

using json = nlohmann::json;

// Proto-type for command to be send to client app
std::string SensorHandler::sensorStartMsg =
    "{ \"key\" : \"sensor-start\" , \"type\" : \"0\" \"}";
std::string SensorHandler::sensorStopMsg =
    "{ \"key\" : \"sensor-stop\" , \"type\" : \"0\" \"}";

SensorHandler::SensorHandler(int instanceId, CommandHandler cmdHandler)
{
    mInstanceId = instanceId;
    mCmdHandler = cmdHandler;
    std::string socketDir = ga_conf_readstr("aic-workdir");

    if (ga_conf_readbool("k8s", 0) == 0) {
        socketDir += "/ipc"; // Docker environment
    }

    ga_logger(Severity::WARNING, "[sensor] Creating SensorHandler %s : %d \n", socketDir.c_str(), instanceId);
    vhal::client::UnixConnectionInfo conn_info = { socketDir, instanceId };
    auto callback = [&](const SensorInterface::CtrlPacket& ctrlPkt) {
        processVHALCtrlMsg(ctrlPkt);
    };

    int width = ga_conf_readint("video-res-width");
    int height = ga_conf_readint("video-res-height");
    if (width < height) {
        mOrientation = portraitOrientation;
    } else {
        mOrientation = landscapeOrientation;
    }
    int32_t userId = -1;
    if (ga_conf_readbool("enable-multi-user", 0) != 0) {
        userId = ga_conf_readint("user");
    }
    sensorHALIface = std::make_unique<SensorInterface>(conn_info, callback, userId);
}

void SensorHandler::processVHALCtrlMsg(const SensorInterface::CtrlPacket& ctrlPkt)
{
    ga_logger(Severity::INFO,
    "sensor_config: sensor type= %d, enabled = %d, sample_period = %d ms\n",
    ctrlPkt.type, ctrlPkt.enabled, ctrlPkt.samplingPeriod_ms);

    std::unique_lock<std::mutex> lck(sensorMapMutex);
    // Insert/Update sensor configuration in map
    mSensorMap[ctrlPkt.type] = {ctrlPkt.enabled, ctrlPkt.samplingPeriod_ms};
    // Check if client app has requested for sensors
    if (getClientRequestFlag() == true) {
        sendCmdToClient(ctrlPkt.type, ctrlPkt.enabled, ctrlPkt.samplingPeriod_ms);
    }
}

void SensorHandler::processClientMsg(const std::string &json_message)
{
    SensorInterface::SensorDataPacket event;
    json j = json::parse(json_message);
    if (!j["data"]["parameters"].is_string()) {
        json event_param = j["data"]["parameters"];
        if (event_param["type"].is_number()) {
            std::vector<float> data =  event_param["data"];
            if (data.size() > MAX_SENSOR_DATA_VAL_CNT)
                return;
            for (size_t j = 0; j < data.size(); j++) {
                event.fdata[j] = data[j];
            }
            event.type = event_param["type"];
            event.timestamp_ns = now_ns();
            if (mOrientation == landscapeOrientation) {
                updateCoordinates(event);
            }
            sensorHALIface->SendDataPacket(&event);
        }
    }
}

int64_t SensorHandler::now_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

// Call this function upon sensor check request from client.
void SensorHandler::configureClientSensors()
{
    std::unique_lock<std::mutex> lck(sensorMapMutex);
    for (auto it : mSensorMap)
       sendCmdToClient(it.first, it.second.first, it.second.second);
}

void SensorHandler::sendCmdToClient(int32_t sensorType, bool enabled, int32_t samplingPeriod_ms)
{
    if (enabled) {
        sensorStartMsg = "{ \"key\" : \"sensor-start\"";
        sensorStartMsg.append(", \"type\" : \"" + std::to_string(sensorType) + "\"");
        sensorStartMsg.append(", \"samplingPeriod_ms\" : \"" + std::to_string(samplingPeriod_ms) + "\"");
        sensorStartMsg.append("}");
        mCmdHandler(static_cast<uint32_t>(Command::kSensorStart));
    } else {
        sensorStopMsg = "{ \"key\" : \"sensor-stop\" , \"type\" :";
        sensorStopMsg.append("\"" + std::to_string(sensorType) + "\"}");
        mCmdHandler(static_cast<uint32_t>(Command::kSensorStop));
    }
}

void SensorHandler::updateCoordinates(SensorInterface::SensorDataPacket &event)
{
    float tmp;
    switch (event.type) {
        case SENSOR_TYPE_ACCELEROMETER:
        case SENSOR_TYPE_MAGNETIC_FIELD:
        case SENSOR_TYPE_GYROSCOPE:
            tmp = event.fdata[0];
            event.fdata[0] = -event.fdata[1];
            event.fdata[1] = tmp;
            break;

        case SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED:
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
            tmp = event.fdata[0];
            event.fdata[0] = -event.fdata[1];
            event.fdata[1] = tmp;

            tmp = event.fdata[3];
            event.fdata[3] = -event.fdata[4];
            event.fdata[4] = tmp;
            break;

        default:
            break;
    }
}
