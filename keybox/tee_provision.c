/*
 * Copyright 2020 Intel Corporation
 * Copyright (C) 2020 The Android Open Source Project
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
#include <errno.h>
#include <getopt.h>
#include <log/log.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "keybox.h"

#define TIPC_DEV "/dev/trusty-ipc-dev0"
#define KEYBOX_PATH  "/dev/block/by-name/teedata"

static const char *trusty_devname;
static const char *keybox_path;

static const char *_sopts = "hd:p:";
static const struct option _lopts[] = {
    {"help",        no_argument,       NULL, 'h'},
    {"trusty_dev",  optional_argument, NULL, 'd'},
    {"keybox_path", optional_argument, NULL, 'p'},
    {0, 0, 0, 0}
};

static void show_usage_and_exit(int code)
{
    printf("usage: provisiond -d <trusty_dev> -p <keybox_path>\n");
    exit(code);
}

static void parse_args(int argc, char *argv[])
{
    int opt;
    int oidx = 0;

    while ((opt = getopt_long(argc, argv, _sopts, _lopts, &oidx)) != -1) {
        switch (opt) {

        case 'd':
            trusty_devname = strdup(optarg);
            break;

        case 'p':
            keybox_path = strdup(optarg);
            break;

        default:
            ALOGE("unrecognized option (%c):\n", opt);
            show_usage_and_exit(EXIT_FAILURE);
        }
    }

    if (trusty_devname == NULL) {
        ALOGE("Not specify argument(s): trusty_dev, use default value.\n");
        trusty_devname = TIPC_DEV;
    }

    if (keybox_path == NULL) {
        ALOGE("Not specify argument(s): keybox path, use default value.\n");
        keybox_path = KEYBOX_PATH;
    }

    ALOGI("keybox starting provisiond.\n");
    ALOGI("keybox trusty dev: %s\n", trusty_devname);
    ALOGI("keybox keybox path: %s\n", keybox_path);
}

int main(int argc, char *argv[])
{
    parse_args(argc, argv);

    /* Provision Keybox */
    ALOGI("\n----- Start Keybox provision -----\n");

    if (!provision_keybox(trusty_devname, keybox_path)) {
        ALOGE("----- Keybox provision failed -----\n\n");
        return EXIT_FAILURE;
    }
    ALOGI("----- Keybox provision successfully -----\n\n");

    return EXIT_SUCCESS;
}

