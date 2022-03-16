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

#ifndef SENSORS_2_0_IIOHAL_MEDIATION_V2_0_IIO_CLIENT_H_
#define SENSORS_2_0_IIOHAL_MEDIATION_V2_0_IIO_CLIENT_H_

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>
#include <utils/Atomic.h>

#include <sys/types.h>
#include <sys/cdefs.h>
#include <linux/limits.h>
#include <cutils/properties.h>
#include <log/log.h>

#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "iio.h"

#define MAX_SENSOR 9
#define MAX_CHANNEL 3

struct iio_sensor_map {
    const char *name;
    int id;
};

struct iioclient_device {
    const char *name;
    const struct iio_device *dev;
    double scale;
    int raw_channel_count;
    struct iio_channel *channel_raw[10];
    struct iio_channel *channel_frequency;;
    float data[16];

    unsigned int nb_channels;
    const char *frequency_channel;

    bool is_initialized;
    bool is_enabled;
    int32_t sampling_period_ns;
};

class iioClient {
 private:
    static iioClient *iioc;
    /**
     * controctor called when iioclient object created.
     * initialize member variables to default state.
     */
    iioClient() {
        sensorCount = 0;
        ctx = NULL;
        is_iioc_initialized = false;
        static std::thread thread_object(iioThread, devlist);
    }

 public:
    static iioClient *get_iioClient() {
        if (!iioc)
            iioc = new iioClient;

        return iioc;
    }
    /**
     * distructor called when iioclient object deleted.
     * and destroy iio client if already initialized.
     */
    ~iioClient() {
        if (ctx)
            iio_context_destroy(ctx);
        iioc = NULL;
    }

    /* member veriables*/
    volatile int sensorCount;
    bool is_iioc_initialized;
    int active_sensor_count;
    struct iio_context *ctx;
    struct iioclient_device devlist[MAX_SENSOR];

    /* member funcions*/
    static void iioThread(struct iioclient_device *devlist);
    bool iioInit(void);
    int get_sensorid_by_name(const char *name);
    int activate(int handle, bool enabled);
    int batch(int handle, int32_t sampling_period_ns);
};
#endif  /*SENSORS_2_0_IIOHAL_MEDIATION_V2_0_IIO_CLIENT_H_*/
