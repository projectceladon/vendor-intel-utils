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

#include "sensors-impl/Sensor.h"

#include <log/log.h>
#include "utils/SystemClock.h"

#include <cmath>
#include <vector>
#undef DEBUG

using ::ndk::ScopedAStatus;

namespace aidl {
namespace android {
namespace hardware {
namespace sensors {

static constexpr float kDefaultMaxDelayUs = 10 * 1000 * 1000;

Sensor::Sensor(ISensorsEventCallback* callback)
    : mIsEnabled(false),
      mSamplingPeriodNs(0),
      mLastSampleTimeNs(0),
      mCallback(callback),
      mMode(OperationMode::NORMAL) {
      iioc = iioClient::get_iioClient();
    mRunThread = std::thread(startThread, this);
}

Sensor::~Sensor() {
    std::unique_lock<std::mutex> lock(mRunMutex);
    mStopThread = true;
    mIsEnabled = false;
    mWaitCV.notify_all();
    lock.release();
    mRunThread.join();
}

const SensorInfo& Sensor::getSensorInfo() const {
    return mSensorInfo;
}

void Sensor::batch(int32_t samplingPeriodNs) {
    if (samplingPeriodNs < mSensorInfo.minDelayUs * 1000) {
        samplingPeriodNs = mSensorInfo.minDelayUs * 1000;
    } else if (samplingPeriodNs > mSensorInfo.maxDelayUs * 1000) {
        samplingPeriodNs = mSensorInfo.maxDelayUs * 1000;
    }

    if (mSamplingPeriodNs != samplingPeriodNs) {
        mSamplingPeriodNs = samplingPeriodNs;
        // Wake up the 'run' thread to check if a new event should be generated now
	
	if (iioc)
	   iioc->batch(mSensorInfo.sensorHandle, mSamplingPeriodNs);
        mWaitCV.notify_all();
    }
}

void Sensor::activate(bool enable) {
    if (mIsEnabled != enable) {
        std::unique_lock<std::mutex> lock(mRunMutex);
        mIsEnabled = enable;
	
	if (iioc)
            iioc->activate(mSensorInfo.sensorHandle, enable);
        mWaitCV.notify_all();
    }
}

ScopedAStatus Sensor::flush() {
    // Only generate a flush complete event if the sensor is enabled and if the sensor is not a
    // one-shot sensor.
    if (!mIsEnabled ||
        (mSensorInfo.flags & static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ONE_SHOT_MODE))) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(BnSensors::ERROR_BAD_VALUE));
    }

    // Note: If a sensor supports batching, write all of the currently batched events for the sensor
    // to the Event FMQ prior to writing the flush complete event.
    Event ev{};
    ev.sensorHandle = mSensorInfo.sensorHandle;
    ev.sensorType = SensorType::META_DATA;
    EventPayload::MetaData meta = {
            .what = MetaDataEventType::META_DATA_FLUSH_COMPLETE,
    };
    ev.payload.set<EventPayload::Tag::meta>(meta);
    std::vector<Event> evs{ev};
    mCallback->postEvents(evs, isWakeUpSensor());

    return ScopedAStatus::ok();
}

void Sensor::startThread(Sensor* sensor) {
    sensor->run();
}

void Sensor::run() {
    std::unique_lock<std::mutex> runLock(mRunMutex);
    constexpr int64_t kNanosecondsInSeconds = 1000 * 1000 * 1000;

    while (!mStopThread) {
        if (!mIsEnabled || mMode == OperationMode::DATA_INJECTION) {
            mWaitCV.wait(runLock, [&] {
                return ((mIsEnabled && mMode == OperationMode::NORMAL) || mStopThread);
            });
        } else {
            timespec curTime;
            clock_gettime(CLOCK_REALTIME, &curTime);
            int64_t now = (curTime.tv_sec * kNanosecondsInSeconds) + curTime.tv_nsec;
            int64_t nextSampleTime = mLastSampleTimeNs + mSamplingPeriodNs;

            if (now >= nextSampleTime) {
                mLastSampleTimeNs = now;
                nextSampleTime = mLastSampleTimeNs + mSamplingPeriodNs;
                mCallback->postEvents(readEvents(), isWakeUpSensor());
            }

            mWaitCV.wait_for(runLock, std::chrono::nanoseconds(nextSampleTime - now));
        }
    }
}

bool Sensor::isWakeUpSensor() {
    return mSensorInfo.flags & static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_WAKE_UP);
}

std::vector<Event> Sensor::readEvents() {
    std::vector<Event> events;
    Event event;
    event.sensorHandle = mSensorInfo.sensorHandle;
    event.sensorType = mSensorInfo.type;
    event.timestamp = ::android::elapsedRealtimeNano();
    memset(&event.payload, 0, sizeof(event.payload));
    EventPayload::Vec3 vec3;
    EventPayload::Vec4 vec4;
    EventPayload::Uncal uncal;
    EventPayload::Data data;

    if (iioc && iioc->is_iioc_initialized) {
        switch (event.sensorType) {
            case SensorType::ACCELEROMETER:
            case SensorType::MAGNETIC_FIELD:
            case SensorType::ORIENTATION:
            case SensorType::GYROSCOPE:
            case SensorType::GRAVITY:
            case SensorType::LINEAR_ACCELERATION:{
                vec3 = {
                    .x = iioc->devlist[mSensorInfo.sensorHandle].data[0],
                    .y = iioc->devlist[mSensorInfo.sensorHandle].data[1],
                    .z = iioc->devlist[mSensorInfo.sensorHandle].data[2],
                    .status = SensorStatus::ACCURACY_HIGH,};
                event.payload.set<EventPayload::Tag::vec3>(vec3);
                break;
            }
            case SensorType::GAME_ROTATION_VECTOR: {
                vec4 = {
                    .x = iioc->devlist[mSensorInfo.sensorHandle].data[0],
                    .y = iioc->devlist[mSensorInfo.sensorHandle].data[1],
                    .z = iioc->devlist[mSensorInfo.sensorHandle].data[2],
                    .w = iioc->devlist[mSensorInfo.sensorHandle].data[3],};
                event.payload.set<EventPayload::Tag::vec4>(vec4);
                break;
            }
            case SensorType::ROTATION_VECTOR:
            case SensorType::GEOMAGNETIC_ROTATION_VECTOR: {
                memcpy(data.values.data(), iioc->devlist[mSensorInfo.sensorHandle].data, 5 * sizeof(float));
                event.payload.set<EventPayload::Tag::data>(data);
                break;
            }
            case SensorType::GYROSCOPE_UNCALIBRATED: {
                uncal = {
                    .x = iioc->devlist[mSensorInfo.sensorHandle].data[0],
                    .y = iioc->devlist[mSensorInfo.sensorHandle].data[1],
                    .z = iioc->devlist[mSensorInfo.sensorHandle].data[2],
                    .xBias = iioc->devlist[mSensorInfo.sensorHandle].data[3],
                    .yBias = iioc->devlist[mSensorInfo.sensorHandle].data[4],
                    .zBias = iioc->devlist[mSensorInfo.sensorHandle].data[5],};
                event.payload.set<EventPayload::Tag::uncal>(uncal);
                break;
            }
            case SensorType::LIGHT:
            case SensorType::DEVICE_ORIENTATION:
            case SensorType::PRESSURE:
            case SensorType::PROXIMITY:
            case SensorType::AMBIENT_TEMPERATURE:
            case SensorType::RELATIVE_HUMIDITY:
            case SensorType::HINGE_ANGLE:{
                event.payload.set<EventPayload::Tag::scalar>((float)iioc->devlist[mSensorInfo.sensorHandle].data[0]);
                break;
            }
            default :{
                vec3 = {
                    .x = iioc->devlist[mSensorInfo.sensorHandle].data[0],
                    .y = iioc->devlist[mSensorInfo.sensorHandle].data[1],
                    .z = iioc->devlist[mSensorInfo.sensorHandle].data[2],
                    .status = SensorStatus::ACCURACY_HIGH,};
            event.payload.set<EventPayload::Tag::vec3>(vec3);
            }
        }
    } else {
        switch (event.sensorType) {
            case SensorType::ACCELEROMETER:{
                vec3 = {
                    .x = 0,
                    .y = 0,
                    .z = -9.8,
                    .status = SensorStatus::ACCURACY_HIGH,
                    };
                event.payload.set<EventPayload::Tag::vec3>(vec3);
                break;
            }
            case SensorType::MAGNETIC_FIELD:{
                vec3 = {
                    .x = 100.0,
                    .y = 0,
                    .z = 50.0,
                    .status = SensorStatus::ACCURACY_HIGH,};
                event.payload.set<EventPayload::Tag::vec3>(vec3);
                break;
            }
            case SensorType::ORIENTATION:
            case SensorType::GYROSCOPE:
            case SensorType::GRAVITY:
            case SensorType::LINEAR_ACCELERATION: {
                event.payload.set<EventPayload::Tag::vec3>(vec3);
                break;
            }
            case SensorType::GAME_ROTATION_VECTOR: {
                event.payload.set<EventPayload::Tag::vec4>(vec4);
                break;
            }
            case SensorType::ROTATION_VECTOR:
            case SensorType::GEOMAGNETIC_ROTATION_VECTOR: {
                event.payload.set<EventPayload::Tag::data>(data);
                break;
            }
            case SensorType::GYROSCOPE_UNCALIBRATED: {
                event.payload.set<EventPayload::Tag::uncal>(uncal);
                break;
            }
            case SensorType::LIGHT:{
                event.payload.set<EventPayload::Tag::scalar>(80.0f);
                break;
            }
            case SensorType::DEVICE_ORIENTATION:{
                event.payload.set<EventPayload::Tag::scalar>(0.0f);
                break;
            }
            case SensorType::PRESSURE:{
                event.payload.set<EventPayload::Tag::scalar>(1013.25f);
                break;
            }
            case SensorType::PROXIMITY:{
                event.payload.set<EventPayload::Tag::scalar>(2.5f);
                break;
            }
            case SensorType::AMBIENT_TEMPERATURE:{
                event.payload.set<EventPayload::Tag::scalar>(40.0f);
                break;
            }
            case SensorType::RELATIVE_HUMIDITY:{
                event.payload.set<EventPayload::Tag::scalar>(50.0f);
                break;
            }
            case SensorType::HINGE_ANGLE: {
                event.payload.set<EventPayload::Tag::scalar>(180.0f);
                break;
            }
            default :{
                vec3 = {
                    .x = 0,
                    .y = 0,
                    .z = 0,
                    .status = SensorStatus::ACCURACY_HIGH,};
                event.payload.set<EventPayload::Tag::vec3>(vec3);
            }
        }
    }
    events.push_back(event);
#ifdef DEBUG   // Probing data to debug
    ALOGD("readEvents: handle(%d) name -> %s data[%f, %f, %f]",
           mSensorInfo.sensorHandle, mSensorInfo.name.c_str(),
           vec3.x, vec3.y, vec3.z);
#endif
    return events;
}

void Sensor::setOperationMode(OperationMode mode) {
    if (mMode != mode) {
        std::unique_lock<std::mutex> lock(mRunMutex);
        mMode = mode;
        mWaitCV.notify_all();
    }
}

bool Sensor::supportsDataInjection() const {
    return mSensorInfo.flags & static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_DATA_INJECTION);
}

ScopedAStatus Sensor::injectEvent(const Event& event) {
    if (event.sensorType == SensorType::ADDITIONAL_INFO) {
        return ScopedAStatus::ok();
        // When in OperationMode::NORMAL, SensorType::ADDITIONAL_INFO is used to push operation
        // environment data into the device.
    }

    if (!supportsDataInjection()) {
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    if (mMode == OperationMode::DATA_INJECTION) {
        mCallback->postEvents(std::vector<Event>{event}, isWakeUpSensor());
        return ScopedAStatus::ok();
    }

    return ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(BnSensors::ERROR_BAD_VALUE));
}

OnChangeSensor::OnChangeSensor(ISensorsEventCallback* callback)
    : Sensor(callback), mPreviousEventSet(false) {}

void OnChangeSensor::activate(bool enable) {
    Sensor::activate(enable);
    if (!enable) {
        mPreviousEventSet = false;
    }
}

std::vector<Event> OnChangeSensor::readEvents() {
    std::vector<Event> events = Sensor::readEvents();
    std::vector<Event> outputEvents;

    for (auto iter = events.begin(); iter != events.end(); ++iter) {
        Event ev = *iter;
        if (!mPreviousEventSet ||
              memcmp(&mPreviousEvent.payload, &ev.payload, sizeof(ev.payload)) != 0) {
            outputEvents.push_back(ev);
            mPreviousEvent = ev;
            mPreviousEventSet = true;
        }
    }
    return outputEvents;
}

AccelSensor::AccelSensor(int32_t sensorHandle, ISensorsEventCallback* callback) : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Accel Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::ACCELEROMETER;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 78.4f;  // +/- 8g
    mSensorInfo.resolution = 1.52e-5;
    mSensorInfo.power = 0.001f;        // mA
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_DATA_INJECTION);

}

PressureSensor::PressureSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Pressure Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::PRESSURE;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1100.0f;     // hPa
    mSensorInfo.resolution = 0.005f;    // hPa
    mSensorInfo.power = 0.001f;         // mA
    mSensorInfo.minDelayUs = 100 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 100 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

MagnetometerSensor::MagnetometerSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Magnetic Field Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::MAGNETIC_FIELD;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1300.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;        // mA
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

LightSensor::LightSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : OnChangeSensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Light Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::LIGHT;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 43000.0f;
    mSensorInfo.resolution = 10.0f;
    mSensorInfo.power = 0.001f;         // mA
    mSensorInfo.minDelayUs = 200 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 200 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE);
}

ProximitySensor::ProximitySensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : OnChangeSensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Proximity Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::PROXIMITY;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 5.0f;
    mSensorInfo.resolution = 1.0f;
    mSensorInfo.power = 0.012f;         // mA
    mSensorInfo.minDelayUs = 200 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 200 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE |
                                              SensorInfo::SENSOR_FLAG_BITS_WAKE_UP);
}

GyroSensor::GyroSensor(int32_t sensorHandle, ISensorsEventCallback* callback) : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Gyro Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::GYROSCOPE_UNCALIBRATED;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1000.0f * M_PI / 180.0f;
    mSensorInfo.resolution = 1000.0f * M_PI / (180.0f * 32768.0f);
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

AmbientTempSensor::AmbientTempSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : OnChangeSensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Ambient Temp Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::AMBIENT_TEMPERATURE;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 80.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 40 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 40 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE);
}

DeviceTempSensor::DeviceTempSensor(int32_t sensorHandle,
        ISensorsEventCallback* callback)
    : OnChangeSensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Device Temp Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::AMBIENT_TEMPERATURE;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 80.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 40 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 40 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE);
}

RelativeHumiditySensor::RelativeHumiditySensor(int32_t sensorHandle,
                                               ISensorsEventCallback* callback)
    : OnChangeSensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Relative Humidity Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::RELATIVE_HUMIDITY;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 100.0f;
    mSensorInfo.resolution = 0.1f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 40 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 40 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE);
}

GravitySensor::GravitySensor(int32_t sensorHandle,
        ISensorsEventCallback* callback) : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Gravity Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::GRAVITY;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1300.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

RotationVector::RotationVector(int32_t sensorHandle,
        ISensorsEventCallback* callback) : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Rotation Vector";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::ROTATION_VECTOR;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1300.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

GeomagnaticRotationVector::GeomagnaticRotationVector(int32_t sensorHandle,
        ISensorsEventCallback* callback) : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Geomagnatic Rotation Vector";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::GEOMAGNETIC_ROTATION_VECTOR;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1300.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

OrientationSensor::OrientationSensor(int32_t sensorHandle,
        ISensorsEventCallback* callback) : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Orientation Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::ORIENTATION;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1300.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

InclinometerSensor::InclinometerSensor(int32_t sensorHandle,
        ISensorsEventCallback* callback)
    : Sensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Inclinometer 3D Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::ORIENTATION;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 1300.0f;
    mSensorInfo.resolution = 0.01f;
    mSensorInfo.power = 0.001f;        // mA
    mSensorInfo.minDelayUs = 10 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = 10 * 1000 * 10;  // microseconds
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = 0;
}

HingeAngleSensor::HingeAngleSensor(int32_t sensorHandle, ISensorsEventCallback* callback)
    : OnChangeSensor(callback) {
    mSensorInfo.sensorHandle = sensorHandle;
    mSensorInfo.name = "Hinge Angle Sensor";
    mSensorInfo.vendor = "Intel";
    mSensorInfo.version = 1;
    mSensorInfo.type = SensorType::HINGE_ANGLE;
    mSensorInfo.typeAsString = "";
    mSensorInfo.maxRange = 360.0f;
    mSensorInfo.resolution = 1.0f;
    mSensorInfo.power = 0.001f;
    mSensorInfo.minDelayUs = 40 * 1000;  // microseconds
    mSensorInfo.maxDelayUs = kDefaultMaxDelayUs;
    mSensorInfo.fifoReservedEventCount = 0;
    mSensorInfo.fifoMaxEventCount = 0;
    mSensorInfo.requiredPermission = "";
    mSensorInfo.flags = static_cast<uint32_t>(SensorInfo::SENSOR_FLAG_BITS_ON_CHANGE_MODE |
                                              SensorInfo::SENSOR_FLAG_BITS_WAKE_UP |
                                              SensorInfo::SENSOR_FLAG_BITS_DATA_INJECTION);
}


}  // namespace sensors
}  // namespace hardware
}  // namespace android
}  // namespace aidl
