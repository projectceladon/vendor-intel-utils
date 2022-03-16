/*
 * Copyright (c) 2020 Intel Corporation
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
 *
 */

#include "iioClient.h"

iioClient *iioClient::iioc = NULL;
/**
 * sensor_map, holds list of sensor information to map during iio initalization.
 * name@sensor_map should be matched with iio sensor name and returns sensor-id.
 * Where stuct iio_sensor_map -> {name, id};
 */
struct iio_sensor_map sensor_map[10] = {
    {"unknown", 0},
    {"accel_3d", 1},
    {"gyro_3d", 2},
    {"magn_3d", 3},
    {"als", 4},
    {"gravity", 5},
    {"dev_rotation", 6},
    {"geomagnetic_orientation", 7},
    {"relative_orientation", 8},
    {"incli_3d", 9},
};

/**
 * get_android_sensor_id_by_name is an helper function to map
 * iio sensor with android sensor list. return -1 when sensor
 * not found otherwise return sensor handle.
 */
int iioClient::get_sensorid_by_name(const char *name) {
    for (int index = 1; index <=  MAX_SENSOR; index++) {
        if (!strcmp(name, sensor_map[index].name)) {
            return sensor_map[index].id;
        }
    }

    /* in this case appropriate sensor not found */
    return -1;
}

void iioClient::iioThread(struct iioclient_device *devlist) {
    /**
     * Incase sensor not initialized initialize
     * once before use
     */
    iioClient *iioc = iioClient::get_iioClient();
    while (!iioc->is_iioc_initialized && !iioc->iioInit())
        sleep(5);

    /* post initialization of iio-devices to align with
     * android sensor fwk.*/
    for (int id = 1; id <= MAX_SENSOR; id++) {
        if (!devlist[id].is_initialized)
            continue;

        if (iioc->devlist[id].sampling_period_ns)
            iioc->batch(id, iioc->devlist[id].sampling_period_ns);

        if (iioc->devlist[id].is_enabled)
            iioc->activate(id, iioc->devlist[id].is_enabled);
    }

    /**
     * read_sensor_data thread is a deffered call to read sensor data.
     * this api is implemented to collect active sensor data from iiod.
     */
    while (iioc) {
        for (int id = 1; id <= MAX_SENSOR; id++) {
            if (!(devlist[id].is_initialized && devlist[id].is_enabled))
                continue;

            for (int index = 0; index < devlist[id].raw_channel_count; index++) {
                struct iio_channel *channel = devlist[id].channel_raw[index];
                char buf[1024] = {0};
                if (iio_channel_attr_read(channel, "raw", buf, sizeof(buf)) > 0)
                    devlist[id].data[index] = strtof(buf, NULL) * devlist[id].scale;
            }
#if 0  // data probing point for debug
            sleep(1);
            ALOGI("%s -> data[%d](%f, %f, %f)",
                  devlist[id].name, devlist[id].raw_channel_count,
                  devlist[id].data[0], devlist[id].data[1], devlist[id].data[2]);
#endif
        }
        usleep(1000);  // 1millisec
    }

    ALOGE("Error: exit iiClient::iioThread");
}

/**
 * iioClient:iioInit, establish nw bakend connection with iiod server
 * and initialize raw chanel, freq channel & read sensorscale.
 * return true on success else fail
 */
bool iioClient::iioInit(void) {
    sensorCount = 0;
    ctx = NULL;
    active_sensor_count = 0;
    is_iioc_initialized = false;

    /* Read IP address from vendor property */
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("vendor.intel.ipaddr", value, "invalid_ip_addr");

    /* Create IIO context */
    ctx = iio_create_network_context(value);
    if (!ctx) {
        ALOGW("Retrying: Initialize IIO Client@%s with N/W backend.", value);
        return false;
    }

    unsigned int nb_iio_devices = iio_context_get_devices_count(ctx);
    for (int i = 0; i < nb_iio_devices; i++) {
        const struct iio_device *device = iio_context_get_device(ctx, i);
        /* Skip device with null pointer */
        if (!device)
            continue;

        const char *sensor_name = iio_device_get_name(device);
        int handle = get_sensorid_by_name(sensor_name);
        /* Skip device with invalid id/handle */
        if (handle < 0)
            continue;

        bool scale_found = false;
        unsigned int nb_channels = iio_device_get_channels_count(device);
        /* Skip device with no channles */
        if (!nb_channels)
            continue;

        for (int ch_index = 0; ch_index < nb_channels; ch_index++) {
            struct iio_channel *channel = iio_device_get_channel(device, ch_index);

            if (!iio_channel_is_output(channel)) {
                unsigned int attrs_count = iio_channel_get_attrs_count(channel);
                for (int attr_index = 0; attr_index < attrs_count; attr_index++) {
                    const char* attr_name = iio_channel_get_attr(channel, attr_index);
                    if (attr_name && !strcmp(attr_name, "raw")) {
                        /* get raw data channels */
                        devlist[handle].raw_channel_count = ch_index + 1;
                        devlist[handle].channel_raw[ch_index] = channel;
                    } else if (!scale_found && attr_name && !strcmp(attr_name, "scale")) {
                        char buf[1024] = {0};
                        /* reading scale */
                        if (iio_channel_attr_read(channel, "scale", buf, sizeof(buf)) > 0) {
                            devlist[handle].scale = strtod(buf, NULL);
                            scale_found = true;
                        }
                    } else if (!scale_found && attr_name && !strcmp(attr_name, "sampling_frequency")) {
                        /**
                         * channle name might be "sampling_frequency" or "frequency"
                         * find existing channel fir gressful operations
                         */
                        devlist[handle].channel_frequency = channel;
                   }
                }
            }
        }

        /* Initialize & Map IIO sensor devices with Android sensor devices */
        devlist[handle].dev = device;
        devlist[handle].name = sensor_name;
        devlist[handle].nb_channels = nb_channels;
        devlist[handle].is_initialized = true;

        sensorCount++;
        if (sensorCount > MAX_SENSOR)
            break;
    }

    /* Destroy iio context if sensor count = zero */
    if (!sensorCount) {
        if (ctx)
            iio_context_destroy(ctx);
        return false;
    }

    ALOGI("Success: Initialized IIO Client@%s with N/W backend. sensor_count(%u)", value, sensorCount);
    is_iioc_initialized = true;
    return true;
}


/**
 * Activate/de-activate one sensor.
 * sensor_handle is the handle of the sensor to change.
 * enabled set to 1 to enable, or 0 to disable the sensor.
 */
int iioClient::activate(int handle, bool enabled) {
    if ((handle < 0) || (handle > MAX_SENSOR)) {
        ALOGE("ERROR: activate(%d) Sensor hadle(%d) is out of range", enabled, handle);
        return 0;
    }

    const char *sensor_name = sensor_map[handle].name;

    /* store the state*/
    devlist[handle].is_enabled = enabled;

    /* skip, if device not initialized*/
    if (!devlist[handle].is_initialized)
        return 0;

    active_sensor_count = 0;
    for (int id = 1; id <= MAX_SENSOR; id++)
        if (devlist[id].is_initialized && devlist[id].is_enabled)
            active_sensor_count++;

    ALOGI("Success: activate ->  Sensor(%s): %s -> active_sensor_count(%d)",
           sensor_name, enabled?"enabled":"disabled", active_sensor_count);

    /* Activate or Deactivate all sensor */
    for (int index = 0; index < devlist[handle].nb_channels; index++) {
        const struct iio_device *device = devlist[handle].dev;
        struct iio_channel *channel = iio_device_get_channel(device, index);

        /* skip output channels */
        if (iio_channel_is_output(channel))
            continue;

        /* enable/disable input channels only */
        if (enabled)
            iio_channel_enable(channel);
        else
            iio_channel_disable(channel);

        ALOGI("%s channel(%d)", enabled?"Activated":"Deactivated", index);
    }

    return 0;
}

/**
 * Sets a sensor's parameters, including sampling frequency and maximum
 * report latency. This function can be called while the sensor is
 * activated.
 */
int iioClient::batch(int handle, int32_t sampling_period_ns) {
    if ((handle < 0) || (handle > MAX_SENSOR)) {
        ALOGE("Warning: batch invalid handle sampling_time(%d) sensor hadle(%d) is out of range",
              sampling_period_ns, handle);
        return 0;
    }

    /* store the sample period*/
    devlist[handle].sampling_period_ns = sampling_period_ns;

    /* Skip, if device not initialized */
    if (!devlist[handle].is_initialized)
        return 0;

    const char *sensor_name = sensor_map[handle].name;
    struct iio_channel *channel = devlist[handle].channel_frequency;

    /* Calculate frequency from sampling time(ns) */
    double write_freq = static_cast<double> (1000000000.00/sampling_period_ns);
    if (iio_channel_attr_write_double(channel, "sampling_frequency", write_freq)) {
        ALOGD("iio-write error: batch -> Sensor(%s) sampling_period_ns(%d) freq(%f)",
              sensor_name, sampling_period_ns, write_freq);
    }

    /* Read confirmation of frequency value */
    double read_freq = 0;
    if (iio_channel_attr_read_double(channel, "sampling_frequency", &read_freq)) {
        ALOGD("iio-read error: batch -> Sensor(%s) sampling_period_ns(%d) freq(%f)",
              sensor_name, sampling_period_ns, read_freq);
    }

    ALOGD("Success: batch -> Sensor(%s), sampling_period_ns(%d) freq(%f %f)",
           sensor_name, sampling_period_ns, write_freq, read_freq);

    return 0;
}
