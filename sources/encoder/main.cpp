// Copyright (C) 2018-2022 Intel Corporation
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

#include <unistd.h>
#include <signal.h>

#include <stdio.h>
#include <thread>
#include <mutex>
#include <map>
#include <string.h>
#include <locale>
#include <sstream>
#include <iostream>
#include <getopt.h>

#include "cli.hpp"
#include "display_server.h"
#include "encoder_comm.h"
#include "encoder_version.h"
#include "auto_version.h"

using namespace std;

static void inline encoder_info_init(encoder_info_t &info) {
    info.latency_opt = 1; //default to enable Encoding latency optimization
    info.framerate = "30";
    info.renderfps_enc = 0; //default encoding by rendering fps mode set to disable. If latency_opt and renderfps_enc both set to enable, renderfps_enc have priority.
    info.minfps_enc = 3; //Min encode fps set to 3 fps, it is only used when encode by render fps mode is on.
    info.encodeType = VASURFACE_ID;
    info.nPixfmt = AV_PIX_FMT_RGBA; //set icr_encoder's default input pix_fmt as RGBA
                                    //0 means AV_PIX_FMT_YUV420p which may mislead encoder.
    info.streaming  = true;
    info.url        = "irrv:264";
    info.plugin     = "vaapi"; // use vaapi plugin by default

    info.tcaeEnabled = true;
    info.tcaeLogPath = nullptr;
}

static void inline show_version() {
    //for SG1, the version format define as : {ICR/IRR}_{major}.{minor}.{build_number}.{build_date}_{icr_project_stage}_{hardware-platform}_{sha1}
    //for SG2, the version format define as : {ICR/IRR}_{major}.{minor}.{build_number}.{build_date}[_{qualifier}.{number}]_{hardware-platform}_{sha1}
    //for qualifier, the values should be alpha/beta/rc(release candidate)..., the qualifier number start from 1.
    std::stringstream version;

    const string strQualifier = CI_QUALIFIER;
    const string strQualifier_Num = CI_QUALIFIER_NUM;
    if (strQualifier != "" && strQualifier_Num != "") {
        version << ICR_ENCODER << '_' << ICR_VERSION_MAJOR << '.' << ICR_VERSION_MINOR << '.'
            << CI_BUILD_NUMBER << '.' << BUILD_DATE << '_' << strQualifier << '.' << strQualifier_Num << '_'
            << COMMIT_ID;
    }
    else  {
        version << ICR_ENCODER << '_' << ICR_VERSION_MAJOR << '.' << ICR_VERSION_MINOR << '.'
            << CI_BUILD_NUMBER << '.' << BUILD_DATE << '_' << COMMIT_ID;
    }

    sock_log("Hello, this is an intel local renderer! Please see version info below:\n");
    sock_log("======================================================================\n");
    sock_log(" icr_encoder:\t%s\n", version.str().c_str());
    sock_log("======================================================================\n");
}

int main(int argc, char *argv[])
{
    show_version();

    // The first args is the instance id, start from 0.
    int instanceID = 0;
    char *sock_server_id = NULL;
    DisplayServer *server;

#ifndef BUILD_FOR_HOST
    ICRCommProp comm_prop;
    int encode_inside = 0;
    int len = comm_prop.getSystemPropInt(ICR_ENCODER_INSIDE_ENABLE, &encode_inside);
    sock_log("%s : %d : comm_prop.getSystemPropInt ICR_ENCODER_INSIDE_ENABLE len =%d, encode_inside=%d!\n", __func__, __LINE__, len, encode_inside);
    if (encode_inside != 1) {
        sock_log("%s:%d : Encode inside but ro.boot.icr.internal not set to 1, exit!\n", __func__, __LINE__);
        exit(EXIT_SUCCESS);
    }

    char prop_value_container_id[PROP_VALUE_MAX] = { 0 };
    len = comm_prop.getSystemProp(ICR_ENCODER_CONTAINER_ID, prop_value_container_id, PROP_VALUE_MAX);
    if (len <= 0) {
        sock_log("%s:%d : Encode inside but get prop ro.boot.container.id fail, exit!\n", __func__, __LINE__);
        exit(EXIT_SUCCESS);
    }

    sock_server_id = prop_value_container_id;
    instanceID = atoi(prop_value_container_id);
    sock_log("%s : %d : sock_server_id=%s, instanceID=%d\n", __func__, __LINE__, sock_server_id, instanceID);
#else
    if (argc < 2) {
        std::cout << "no valid parameters!" << std::endl;
        icr_print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // To judge if a string(the first parameter, argv[1]) is number.
    std::locale loc;
    std::string first_arg = argv[1];
    if (!isdigit(first_arg[0], loc)) {
        std::cout << "1st parameter must be the id number!" << std::endl;
        exit(EXIT_FAILURE);
    }

    int id_value;
    std::stringstream(first_arg) >> id_value;
    instanceID = id_value;
    sock_server_id = argv[1];
#endif

    encoder_info_t info = { 0 };
    encoder_info_init(info);
    info.encoderInstanceID = instanceID;

    icr_parse_args(argc, argv, info);

#ifndef ENABLE_QSV
    if (strncmp(info.plugin, "qsv", strlen("qsv")) == 0)
    {
        sock_log("%s : %d : QSV path is not enabled in build, can't enable it in command line!\n", __func__, __LINE__);
        goto fail;
    }
#endif


#ifndef BUILD_FOR_HOST
    encoder_properties_sanity_check(&info);
#endif

    if (bitrate_ctrl_sanity_check(&info) < 0)
        goto fail;

    rir_type_sanity_check(&info);

    if (irr_check_options(&info) < 0)
        goto fail;

    server = DisplayServer::Create(info.hwc_sock);
    if (server == nullptr)
    {
        std::cerr << "Failed to create display server" << std::endl;
        goto fail;
    }

    show_encoder_info(&info);

    show_env();

    if (sock_server_id != nullptr) {
        bool ret = server->init(sock_server_id, &info);
        if (!ret) { //init failed
            std::cerr << "Failed to initialize the display server" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    //register signal handlers.
    //sigset is deprecated, and is not supported by Android NDK
    struct sigaction sa_usr;
    sa_usr.sa_flags = 0;
    sa_usr.sa_handler = DisplayServer::signal_handler;

    sigaction(SIGINT,     &sa_usr, NULL); // Ctrl+C trigger
    sigaction(SIGTERM,    &sa_usr, NULL); // Ctrl+\ trigger
    sigaction(SIGQUIT,    &sa_usr, NULL); // kill command will trigger

    server->run();
    server->deinit();

    delete server;

    clear_env();

    exit(EXIT_SUCCESS);

fail:
    icr_print_usage(argv[0]);
    exit(EXIT_FAILURE);
}
