// Copyright (C) 2022 Intel Corporation

/*
 * Copyright (c) 2013 Chun-Ying Huang
 *
 * This file is part of GamingAnywhere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <csignal>
#include <mutex>

#include "ga-common.h"
#include "ga-conf.h"
#include "ga-module.h"
#include "encoder-common.h"

#ifdef WIN32
#define GA_MODULE_PREFIX "mod/"
#endif

static char *imagepipefmt = "video-%d";
static char *video_encoder_param = imagepipefmt;

namespace {
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop = false;
}

class Ctx
{
public:
    Ctx() = default;
    virtual ~Ctx(){};
    virtual int LoadModules() = 0;
    virtual void UnloadModules() = 0;
    virtual int InitModules() = 0;
    virtual void DeinitModules() = 0;
    virtual int RunModules() = 0;
    virtual void StopModules() = 0;
};

class AndroidIcrCtx: public Ctx
{
public:
    AndroidIcrCtx() = default;
    virtual ~AndroidIcrCtx() = default;
    int LoadModules() override;
    void UnloadModules() override;
    int InitModules() override;
    void DeinitModules() override;
    int RunModules() override;
    void StopModules() override;

private:
    ga_module_t *m_vsource = nullptr;
    ga_module_t *m_vencoder = nullptr;
    ga_module_t *m_server = nullptr;
};

int AndroidIcrCtx::LoadModules() {
    const char* mod_vencoder = GA_MODULE_PREFIX "irrv-receiver";
    const char* mod_live = GA_MODULE_PREFIX "server-webrtc";

    if((m_vencoder = ga_load_module(mod_vencoder, nullptr)) == nullptr) {
        ga_logger(Severity::ERR, "failed to load module: %s\n", mod_vencoder);
        return -1;
    }
    ga_logger(Severity::INFO, "module loaded: %s\n", mod_vencoder);

    if((m_server = ga_load_module(mod_live, nullptr)) == nullptr)
    {
        ga_logger(Severity::ERR, "failed to load module: %s\n", mod_live);
        return -1;
    }
    ga_logger(Severity::INFO, "module loaded: %s\n", mod_live);

    return 0;
}

void AndroidIcrCtx::UnloadModules() {
    ga_unload_module(m_server);
    ga_unload_module(m_vencoder);
}

int AndroidIcrCtx::InitModules() {
    ga_init_single_module_or_quit("video-encoder", m_vencoder, imagepipefmt);
    ga_init_single_module_or_quit("server-webrtc", m_server, NULL);
    return 0;
}

void AndroidIcrCtx::DeinitModules() {
        ga_module_deinit(m_server, NULL);
        ga_module_deinit(m_vencoder, NULL);
}

int AndroidIcrCtx::RunModules() {
    encoder_register_vencoder(m_vencoder, video_encoder_param);
    if(m_server->start(NULL) < 0)
        exit(-1);
    return 0;
}

void AndroidIcrCtx::StopModules() {
    ga_module_stop(m_vencoder, NULL);
        ga_module_stop(m_server, NULL);
}

static const char* p2p_default_host = "localhost";
static const char* p2p_default_port = "8095";
static const char* icr_default_host = "127.0.0.1";
static const char* icr_default_port = "23432";
static const char* default_icr_start_immediately = "0";
static const char* default_loglevel = "info";
static const char* default_owt_loglevel = "none";
static const char* default_workdir = "/opt/workdir";
static const char* default_session = "0";
static const char* default_enable_multi_user = "0";
static const char* default_user = "0";
static const char* default_server_peer_id = "ga0";
static const char* default_client_peer_id = "client0";
static const char* default_bufferred_records = "5";
static const char* default_device = "/dev/dri/renderD128";
static const char* default_codec = "h264";
static const char* default_rc = "vbr";
static const char* default_width = "1920";
static const char* default_height = "1080";
static const char* default_framerate = "30";
static const char* default_video_bitrate = "3000000";
static const char* default_tcae = "1";
static const char* default_virtual_input_num = "2";
static const char* default_k8s_env = "0";
static const char* default_measure_latency = "0";
static const char* default_enable_render_drc = "0";
static const char* default_ice_port_min= "50000";
static const char* default_ice_port_max = "50999";
static const char* default_coturn_username = "username";
static const char* default_coturn_password = "password";
static const char* default_coturn_port = "3478";
static const char* default_client_clones = "0";

void usage(const char* app) {
    printf("usage: %s [options] config\n", app);
    printf("\n");
    printf("Global options:\n");
    printf("  -h, --help              Print this help\n");
    printf("  --loglevel <level>      Loglevel to use (default: %s)\n", default_loglevel);
    printf("              error         Only errors will be printed\n");
    printf("              warning       Errors and warnings will be printed\n");
    printf("              info          Errors, warnings and info messages will be printed\n");
    printf("              debug         Everything will be printed, including lowlevel debug messages\n");
    printf("  --owt-loglevel <level>  OWT loglevel to use (default: %s)\n", default_owt_loglevel);
    printf("              none          Don't log\n");
    printf("              error         Log errors\n");
    printf("              warning       Log errors and warnings\n");
    printf("              info          Log info messages which might be worth to investigate\n");
    printf("              verbose       Verbose logging\n");
    printf("              sensitive     Sensitive logging\n");
    printf("\n");
    printf("P2P and other helper server options:\n");
    printf("  -s, --server <ip>              p2p server ip address (default: %s)\n", p2p_default_host);
    printf("  -p, --port <port>              p2p server port (default: %s)\n", p2p_default_port);
    printf("  --server-peer-id <id>          Server peer ID, 0-INT_MAX (default: %s)\n",
      default_server_peer_id);
    printf("  --client-peer-id <id>          Client peer ID, 0-INT_MAX (default: %s)\n",
      default_client_peer_id);
    printf("  --client-clones <num>          number of clone client (default: %s)\n", default_client_clones);
    printf("  --ice-port-min <port>          ICE port min, 0-USHRT_MAX (default: %s)\n", default_ice_port_min);
    printf("  --ice-port-max <port>          ICE port max, 0-USHRT_MAX (default: %s)\n", default_ice_port_max);
    printf("  --coturn-ip <ip>               Coturn IP\n");
    printf("  --coturn-port <port>           Coturn prot, 0-USHRT_MAX (default: %s)\n", default_coturn_port);
    printf("  --coturn-username <username>   Coturn username (default: %s)\n", default_coturn_username);
    printf("  --coturn-password <password>   Coturn password (default: %s)\n", default_coturn_password);
    printf("\n");
    printf("ICR server options:\n");
    printf("  --icr-ip <ip>                   ICR server ip address (default: %s)\n", icr_default_host);
    printf("  --icr-port <port>               ICR server port (default: %s)\n", icr_default_port);
    printf("                                    Note: actual port will be <port>+1000+<n>, see -n option\n");
    printf("  --icr-start-immediately 0|1     ICR encoder will be started immediately without waiting for connection from client (default: %s)\n", default_icr_start_immediately);
    printf("\n");
    printf("AIC (Android In Container) server options:\n");
    printf("  --workdir <path>                Path to AIC workdir (default: %s)\n", default_workdir);
    printf("  -n <n>                          AIC session (instance) to connect to, 0-INT_MAX (default: %s)\n",
      default_session);
    printf("  --enable-multi-user 0|1         Enable multi user, one AIC connect with multi streamer (default: %s)\n",
      default_enable_multi_user);
    printf("  --user <user_id>                AIC user id, (default: %s)\n", default_user);
    printf("  --virtual-input-num <int>       Virtual input number  (default: %s)\n", default_virtual_input_num);
    printf("\n");
    printf("Video encoding options:\n");
    printf("  --codec <codec>                 Video codec (default: %s)\n", default_codec);
    printf("          av1\n");
    printf("          h264 or avc\n");
    printf("          h265 or hevc\n");
    printf("  --enable-render-drc 0|1         Enable render resolution change automatically with client window size change (default: %s)\n", default_enable_render_drc);
    printf("\n");
    printf("Kubernetes options:\n");
    printf("  --k8s 0|1           K8S environment (default: %s)\n", default_k8s_env);
    printf("\n");
    printf("Latency measuring options:\n");
    printf("  --measure-latency 0|1           Measure latency (default: %s)\n", default_measure_latency);
}

static void signal_handler(int signal) {
    if (signal == SIGTERM || signal == SIGINT) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop  = true;
        printf("\nUser requested to terminate server.\n");
        m_cv.notify_one();
    }
}

static enum Severity get_loglevel(const char* level) {
    if (std::string("error") == level)
        return Severity::ERR;
    else if (std::string("warning") == level)
        return Severity::WARNING;
    else if (std::string("info") == level)
        return Severity::INFO;
    else if (std::string("debug") == level)
        return Severity::DBG;
    return Severity::ERR;
}

int
main(int argc, const char *argv[]) {
    int config_idx = 1;
    const char* p2p = p2p_default_host;
    const char* p = p2p_default_port;
    const char* icr_ip = icr_default_host;
    const char* icr_port = icr_default_port;
    const char* icr_start_immediately = default_icr_start_immediately;
    const char* loglevel = default_loglevel;
    const char* owt_loglevel = default_owt_loglevel;
    const char* server_id = default_server_peer_id;
    const char* client_id = default_client_peer_id;
    const char* workdir = default_workdir;
    const char* session = default_session;
    const char* enable_multi_user = default_enable_multi_user;
    const char* user = default_user;
    const char* target_delay = nullptr;
    const char* bufferred_records = default_bufferred_records;
    const char* device = default_device;
    const char* width = default_width;
    const char* height = default_height;
    const char* framerate = default_framerate;
    const char* video_codec = default_codec;
    const char* video_rc = default_rc;
    const char* video_bitrate = default_video_bitrate;
    const char* enable_tcae = default_tcae;
    const char* tcae_debug_log = nullptr;
    const char* video_bs_file = nullptr;
    const char* video_stats_file = nullptr;
    const char* virtual_input_num = default_virtual_input_num;
    const char* k8s_env = default_k8s_env;
    const char* measure_latency = default_measure_latency;
    const char* enable_render_drc = default_enable_render_drc;
    const char* ice_port_min = default_ice_port_min;
    const char* ice_port_max = default_ice_port_max;
    const char* coturn_ip = nullptr;
    const char* coturn_username = default_coturn_username;
    const char* coturn_password = default_coturn_password;
    const char* coturn_port = default_coturn_port;
    const char* client_clones = default_client_clones;
    for (config_idx = 1; config_idx < argc; ++config_idx) {
        if (std::string("-h") == argv[config_idx] ||
            std::string("--help") == argv[config_idx]) {
            usage(argv[0]);
            exit(0);
        } else if (std::string("-s") == argv[config_idx] ||
                   std::string("--server") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            p2p = argv[config_idx];
        } else if (std::string("-p") == argv[config_idx] ||
                   std::string("--port") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            p = argv[config_idx];
        } else if (std::string("--icr-ip") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            icr_ip = argv[config_idx];
        } else if (std::string("--icr-port") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            icr_port = argv[config_idx];
        } else if (std::string("--icr-start-immediately") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            icr_start_immediately = argv[config_idx];
        } else if (std::string("--loglevel") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            loglevel = argv[config_idx];
        } else if (std::string("--owt-loglevel") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            owt_loglevel = argv[config_idx];
        } else if (std::string("--server-peer-id") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            server_id = argv[config_idx];
        } else if (std::string("--client-peer-id") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            client_id = argv[config_idx];
        } else if (std::string("--workdir") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            workdir = argv[config_idx];
        } else if (std::string("-n") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            session = argv[config_idx];
        } else if (std::string("--enable-multi-user") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            enable_multi_user = argv[config_idx];
        } else if (std::string("--user") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            user = argv[config_idx];
        } else if (std::string("--netpred-delay") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            target_delay = argv[config_idx];
        } else if (std::string("--netpred-records") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            bufferred_records = argv[config_idx];
        } else if (std::string("-d") == argv[config_idx] ||
                   std::string("--device") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            device = argv[config_idx];
        } else if (std::string("--width") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            width = argv[config_idx];
        } else if (std::string("--height") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            height = argv[config_idx];
        } else if (std::string("--framerate") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            framerate = argv[config_idx];
        } else if (std::string("--codec") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            video_codec = argv[config_idx];
        } else if (std::string("--video-rc") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            video_rc = argv[config_idx];
        } else if (std::string("--video-bitrate") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            video_bitrate = argv[config_idx];
        } else if (std::string("--enable-tcae") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            enable_tcae = argv[config_idx];
        } else if (std::string("--tcae-debug-log") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            tcae_debug_log = argv[config_idx];
        } else if (std::string("--video-bs-file") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            video_bs_file = argv[config_idx];
        } else if (std::string("--video-stats-file") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            video_stats_file = argv[config_idx];
        } else if (std::string("--virtual-input-num") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            virtual_input_num = argv[config_idx];
        } else if (std::string("--k8s") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            k8s_env = argv[config_idx];
        } else if (std::string("--measure-latency") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            measure_latency = argv[config_idx];
        } else if (std::string("--enable-render-drc") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            enable_render_drc = argv[config_idx];
        } else if (std::string("--ice-port-min") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            ice_port_min = argv[config_idx];
        } else if (std::string("--ice-port-max") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            ice_port_max = argv[config_idx];
        } else if (std::string("--coturn-ip") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            coturn_ip = argv[config_idx];
        } else if (std::string("--coturn-username") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            coturn_username = argv[config_idx];
        } else if (std::string("--coturn-password") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            coturn_password = argv[config_idx];
        } else if (std::string("--coturn-port") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            coturn_port = argv[config_idx];
        } else if (std::string("--client-clones") == argv[config_idx]) {
            if (++config_idx >= argc) break;
            client_clones = argv[config_idx];
        } else {
            break;
        }
    }

    if(config_idx >= argc) {
        fprintf(stderr, "fatal: invalid option or no config specified\n");
        usage(argv[0]);
        return -1;
    }

    ga_set_loglevel(get_loglevel(loglevel));

    if(ga_init(argv[config_idx]) < 0)
        return -1;

    ga_conf_writev("owt-loglevel", owt_loglevel);
    ga_conf_writev("signaling-server-host", p2p);
    ga_conf_writev("signaling-server-port", p);
    ga_conf_writev("server-peer-id", server_id);
    ga_conf_writev("client-peer-id", client_id);
    ga_conf_writev("icr-ip", icr_ip);
    ga_conf_writev("icr-port", icr_port);
    ga_conf_writev("icr-start-immediately", icr_start_immediately);
    ga_conf_writev("aic-workdir", workdir);
    ga_conf_writev("android-session", session);
    ga_conf_writev("enable-multi-user", enable_multi_user);
    ga_conf_writev("user", user);
    if (target_delay)
        ga_conf_writev("netpred-target-delay", target_delay);
    ga_conf_writev("netpred-records", bufferred_records);
    ga_conf_writev("video-device", device);
    ga_conf_writev("video-res-width", width);
    ga_conf_writev("video-res-height", height);
    ga_conf_writev("video-fps", framerate);
    ga_conf_writev("video-codec", video_codec);
    ga_conf_writev("video-rc", video_rc);
    ga_conf_mapwritev("video-specific", "b", video_bitrate);
    ga_conf_writev("enable-tcae", enable_tcae);
    ga_conf_writev("k8s", k8s_env);
    ga_conf_writev("measure-latency", measure_latency);
    ga_conf_writev("enable-render-drc", enable_render_drc);
    if (tcae_debug_log)
        ga_conf_writev("tcae-debug-log", tcae_debug_log);
    if (video_bs_file)
        ga_conf_writev("video-bs-file", video_bs_file);

    {
        ga_logger(Severity::INFO, "periodic-server: signaling-server-host=%s\n", ga_conf_readstr("signaling-server-host").c_str());
        ga_logger(Severity::INFO, "periodic-server: signaling-server-port=%s\n", ga_conf_readstr("signaling-server-port").c_str());
        ga_logger(Severity::INFO, "periodic-server: icr-ip=%s\n", ga_conf_readstr("icr-ip").c_str());
        ga_logger(Severity::INFO, "periodic-server: icr-port=%s\n", ga_conf_readstr("icr-port").c_str());
        ga_logger(Severity::INFO, "periodic-server: aic-workdir=%s\n", workdir);
        ga_logger(Severity::INFO, "periodic-server: netpred-target-delay=%s\n", ga_conf_readstr("netpred-target-delay").c_str());
        ga_logger(Severity::INFO, "periodic-server: netpred-records=%s\n", ga_conf_readstr("netpred-records").c_str());
        ga_logger(Severity::INFO, "periodic-server: user=%s\n", user);
    }
    if (video_stats_file != nullptr)
        ga_conf_writev("video-stats-file", video_stats_file);

    if (virtual_input_num != nullptr)
        ga_conf_writev("virtual-input-num", virtual_input_num);

    ga_conf_writev("ice-port-min", ice_port_min);
    ga_conf_writev("ice-port-max", ice_port_max);
    if (coturn_ip != nullptr)
        ga_conf_writev("coturn-ip", coturn_ip);
    ga_conf_writev("coturn-username", coturn_username);
    ga_conf_writev("coturn-password", coturn_password);
    ga_conf_writev("coturn-port", coturn_port);
    ga_conf_writev("client-clones", client_clones);

    std::unique_ptr<Ctx> ctx = std::make_unique<AndroidIcrCtx>();

    if(ctx->LoadModules() < 0) { return -1; }
    if(ctx->InitModules() < 0) { return -1; }
    if(ctx->RunModules() < 0)  { return -1; }

    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, []{ return m_stop; });
    }
    printf("Aborting...\n");
    ctx->StopModules();
    ctx->DeinitModules();
    ctx->UnloadModules();
    ga_deinit();

    return 0;
}

