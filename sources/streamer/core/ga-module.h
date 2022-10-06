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

/**
 * @file
 * Common functions and definitions for implementing a GamingAnywhere module: headers.
 */

#ifndef __GA_MODULE__
#define __GA_MODULE__

#include "ga-common.h"
#include <memory>
#include "cursor.h"
#include "QoSMgt.h"

#ifndef WIN32
typedef void * HMODULE;
#endif

/**
 * Enumeration for types of a module.
 */
enum ga_module_types {
    GA_MODULE_TYPE_NULL = 0,    /**< Not used */
    GA_MODULE_TYPE_CONTROL,        /**< Is a controller module */
    GA_MODULE_TYPE_ASOURCE,        /**< Is an audio source module */
    GA_MODULE_TYPE_VSOURCE,        /**< Is an video source module */
    GA_MODULE_TYPE_FILTER,        /**< Is a filter module */
    GA_MODULE_TYPE_AENCODER,    /**< Is an audio encoder module */
    GA_MODULE_TYPE_VENCODER,    /**< Is a video encoder module */
    GA_MODULE_TYPE_ADECODER,    /**< Is an audio decoder module */
    GA_MODULE_TYPE_VDECODER,    /**< Is a video decoder module */
    GA_MODULE_TYPE_SERVER        /**< Is a server module */
};

/**
 * Enumeration for module ioctl() commands.
 */
enum ga_ioctl_commands {
    GA_IOCTL_NULL = 0,        /**< Not used */
    GA_IOCTL_RECONFIGURE,        /**< Reconfiguration */
    GA_IOCTL_RECONFIGURE_DELTAQP,
    GA_IOCTL_REQUEST_KEYFRAME,    /**< Request a key frame. This starts encoding. */
    GA_IOCTL_GET_CREDIT_BYTES,
    GA_IOCTL_SET_MAX_BPS,
    GA_IOCTL_GETSPS = 0x100,    /**< Get SPS: for H.264 and H.265 */
    GA_IOCTL_GETPPS,        /**< Get PPS: for H.264 and H.265 */
    GA_IOCTL_GETVPS,        /**< Get VPS: for H.265 */
    GA_IOCTL_UPDATE_CLIENT_EVENT,    /**< Update keyboard and mouse event for the E2E latency. */
    GA_IOCTL_REQUEST_NEW_CURSOR,
    GA_IOCTL_UPDATE_FRAME_STATS,
    GA_IOCTL_PAUSE,            /**< Pause processing. */
    GA_IOCTL_CHANGE_RENDER_RESOLUTION, /**< Change the resolution in render */
    GA_IOCTL_SET_VIDEO_ALPHA,   /**< Set video alpha mode in render */
    GA_IOCTL_CUSTOM = 0x40000000    /**< For user customization */
};

/**
 * GamingAnyhwere ioctl() error codes.
 */
#define    GA_IOCTL_ERR_NONE        0    /**< No error */
#define    GA_IOCTL_ERR_GENERAL        -1    /**< General error */
#define    GA_IOCTL_ERR_NULLMODULE        -2    /**< Module is NULL */
#define    GA_IOCTL_ERR_NOIOCTL        -3    /**< ioctl() is not implemented */
#define    GA_IOCTL_ERR_NOINTERFACE    -3    /**< ioctl() or interface is not implemented */
#define    GA_IOCTL_ERR_NOTINITIALIZED    -4    /**< Module has not been initialized */
#define    GA_IOCTL_ERR_NOTSUPPORTED    -5    /**< Command is not supported */
#define    GA_IOCTL_ERR_INVALID_ARGUMENT    -6    /**< Invalid argument */
#define    GA_IOCTL_ERR_NOTFOUND        -7    /**< Not found */
#define    GA_IOCTL_ERR_BUFFERSIZE        -8    /**< Buffer error */
#define    GA_IOCTL_ERR_BADID        -9    /**< Invalid Id */
#define    GA_IOCTL_ERR_NOMEM        -10    /**< No memory */

/**
 * Parameter for ioctl()'s codec GET SPS/PPS/VPS command.
 */
typedef struct ga_ioctl_buffer_s {
    int id;
    unsigned char *ptr;    /**< Pointer to the buffer */
    int size;        /**< Size of the buffer */
}    ga_ioctl_buffer_t;

/**
 * Parameter for ioctl()'s codec reconfiguration command.
 */
typedef struct ga_ioctl_reconfigure_s {
    int id;
    int crf;        /**< Constant rate factor */
    int framerate_n;    /**< Framerate numerator */
    int framerate_d;    /**< Framerate denominator */
    int bitrateKbps;    /**< bitrate in Kbit-per-second. Affects both bitrate and vbv-maxrate */
    int bufsize;        /**< vbv-bufsize */
    int width;        /**< Width */
    int height;        /**< Height */
#ifdef WIN32
    int deltaQP;    /**< deltaQP is used to update based on current frame status*/
#endif
}    ga_ioctl_reconfigure_t;

typedef struct ga_ioctl_credit_s {
    int credit_bytes;    /**< Credit bytes */
} ga_ioctl_credit_t;

typedef struct ga_ioctl_maxbps_s {
    int max_bps;  /** <Max available bps */
} ga_ioctl_maxbps_t;

typedef struct ga_ioctl_clevent_s {
    struct timeval timeevent;  /** < event for the time*/
} ga_ioctl_clevent_t;

typedef struct ga_ioctl_framestats_s {
    long frame_ts;
    long frame_size;
    long frame_delay;
    long frame_start_delay;
    long packet_loss;
} ga_ioctl_framestats_t;

typedef struct ga_ioctl_resolution_s {
    int width;
    int height;
} ga_ioctl_resolution_t;
//////////////////////////////////////////////
// Data structures and functions to replace AVPacket
#define GA_NOPTS_VALUE          ((int64_t)UINT64_C(0x8000000000000000))
#define GA_INPUT_BUFFER_PADDING_SIZE 32

/**
 * Enumeration for types of a packet side data. (Replace AVPacketSideDataType)
 */
enum class ga_packet_side_data_type {
    GA_PACKET_DATA_PALETTE = 0,
    GA_PACKET_DATA_NEW_EXTRADATA = 1,
};

/**
 * Data strucure to represent a packet side data. (Replace AVPacketSideData)
 */
typedef struct ga_packet_side_data_s {
    uint8_t* data = nullptr;
    int      size = 0;
    ga_packet_side_data_type type;
} ga_packet_side_data_t;

typedef enum {
    GA_PKT_FLAG_NONE    = 0,
    GA_PKT_FLAG_KEY     = 0x1,   // equal to AV_PKT_FLAG_KEY
    GA_PKT_FLAG_CORRUPT = 0x2,   // equal to AV_PKT_FLAG_CORRUPT
} ga_packet_flags_t;

/**
 * Data strucure to represent a packet data. (Replace AVPacket)
 */
typedef struct ga_packet_s {
    void* buf = nullptr;
    int64_t   pts       = GA_NOPTS_VALUE; /**< Presentation timestamp */
    int64_t   dts       = GA_NOPTS_VALUE; /**< Decompression timestamp */
    uint8_t*  data      = nullptr;        /**< Compressed data */
    int       size      = 0;              /**< Size of compressed data */
    int       stream_index = 0;
    int       flags     = 0;
    ga_packet_side_data_t* side_data = nullptr; /**< Additional packet data */
    int       side_data_elems = 0;        /**< Number of side data elements */
    int       duration  = 0;              /**< Duration of this packet */
    int64_t   pos       = -1;             /**< byte position in stream, -1 if unknown */
    int64_t   convergence_duration = 0;   /**< Time difference */
} ga_packet_t;

EXPORT void ga_init_packet(ga_packet_t* pkt);
EXPORT uint8_t* ga_packet_new_side_data(ga_packet_t* pkt, ga_packet_side_data_type type, int size);
EXPORT uint8_t* ga_packet_get_side_data(ga_packet_t* pkt, ga_packet_side_data_type type, int* size);
EXPORT void ga_packet_free_side_data(ga_packet_t* pkt);

//////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif
//////////////////////////////////////////////
/**
 * Data strucure to represent a module.
 */
typedef struct ga_module_s {
    HMODULE    handle;        /**< Handle to a module */
    int type;        /**< Type of the module */
    const char *name;    /**< Name of the module */
    const char *mimetype;    /**< MIME-type of the module */
#ifdef WIN32
    int (*init)(void *arg, void (*p)(struct timeval));        /**< Pointer to the init function */
#else
    int (*init)(void *arg);        /**< Pointer to the init function */
#endif
    int (*start)(void *arg);    /**< Pointer to the start function */
    //void * (*threadproc)(void *arg);
    int (*stop)(void *arg);        /**< Pointer to the stop function */
    int (*deinit)(void *arg);    /**< Pointer to the deinit function */
    int (*ioctl)(int command, int argsize, void *arg);    /**< Pointer to ioctl function */
    int (*notify)(void *arg);    /**< Pointer to the notify function */
    void * (*raw)(void *arg, int *size);    /**< Pointer to the raw function */
    int (*send_packet)(const char *prefix, int channelId, ga_packet_t *pkt, int64_t encoderPts, struct timeval *ptv);    /**< Pointer to the send packet function: sink only */

#ifdef WIN32
    int (*send_cursor)(std::shared_ptr<CURSOR_DATA> cursorInfo, struct timeval *ptv);  /**< Pointer to the send cusor function: sink only */
#endif
    int (*send_qos)(std::shared_ptr<QosInfo> qosInfo); /** Pointer to the send qos function: sink only */

    void * privdata;        /**< Private data of this module */
}    ga_module_t;
//////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#define    MODULE    extern "C"    /**< Module interface is pure-C implementation: Export C symbols */
#if defined WIN32 && defined GA_MODULE
#define    MODULE_EXPORT    __declspec(dllexport)
#else
#define    MODULE_EXPORT    /**< MODULE_EXPORT is not used in UNIX-like environment */
#endif

EXPORT ga_module_t * ga_load_module(const char *modname, const char *prefix);
EXPORT void ga_unload_module(ga_module_t *m);

#ifdef WIN32
EXPORT int ga_init_single_module(const char *name, ga_module_t *m, void *arg, void (*p)(struct timeval));
EXPORT void ga_init_single_module_or_quit(const char *name, ga_module_t *m, void *arg, void(*p)(struct timeval));
#else
EXPORT int ga_init_single_module(const char *name, ga_module_t *m, void *arg);
EXPORT void ga_init_single_module_or_quit(const char *name, ga_module_t *m, void *arg);
#endif

EXPORT int ga_run_single_module(const char *name, void * (*threadproc)(void*), void *arg);
EXPORT void ga_run_single_module_or_quit(const char *name, void * (*threadproc)(void*), void *arg);
// module function wrappers
EXPORT int ga_module_init(ga_module_t *m, void *arg);
EXPORT int ga_module_start(ga_module_t *m, void *arg);
EXPORT int ga_module_stop(ga_module_t *m, void *arg);
EXPORT int ga_module_deinit(ga_module_t *m, void *arg);
EXPORT int ga_module_ioctl(ga_module_t *m, int command, int argsize, void *arg);
EXPORT int ga_module_notify(ga_module_t *m, void *arg);
EXPORT void * ga_module_raw(ga_module_t *m, void *arg, int *size);
EXPORT int ga_module_send_packet(ga_module_t *m, const char *prefix, int channelId, ga_packet_t *pkt, int64_t encoderPts, struct timeval *ptv);

#ifdef GA_MODULE
// a module must have exported the module_load function
MODULE MODULE_EXPORT ga_module_t * module_load();
#endif

#endif /* __GA_MODULE__ */
