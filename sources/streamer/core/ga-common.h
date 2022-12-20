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
 * header: common GA functions and macros
 */

#ifndef __GA_COMMON_H__
#define __GA_COMMON_H__

#include <stdio.h>
#include <cstdint>
#include <string>

#ifndef WIN32
#include <sys/time.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined WIN32 && defined GA_LIB
/** Functions exported from DLL's */
#define    EXPORT __declspec(dllexport)
#elif defined WIN32 && ! defined GA_LIB
/** Functions imported from DLL's */
#define    EXPORT __declspec(dllimport)
#else
/** Not used in UNIX-like systems, but required for compatible with WIN32 libraries */
#define    EXPORT
#endif

#include "ga-win32.h"
#ifdef WIN32
#include <memory>
#endif
/** Enable audio subsystem? */
#define    ENABLE_AUDIO

/** Enable E2E latency telemetry */
#define    E2ELATENCY_TELEMETRY_ENABLED

/** Unit size size for RGBA pixels, in bytes */
#define    RGBA_SIZE    4

struct gaRect {
    int left, top;
    int right, bottom;
    int width, height;
    int linesize;
    int size;
};

#define CURSOR_EVENT_SEND_CURSOR         1
#define CURSOR_EVENT_CLIENT_CONNECT      2
#define CURSOR_EVENT_CLIENT_DISCONNECT   3
struct ServerConfig {
    void *prect;
#ifdef WIN32
    HWND(*pHookInput)(INPUT in, int size);
    void(*pHookClientStatus)(uint32_t count);
#endif
    void(*pRequestKeyCursor)(int event);
};
struct gaImage {
    int width;
    int height;
    int bytes_per_line;
};

#ifdef WIN32
struct gaPoint {
    long x = 0;
    long y = 0;
};
struct gaCursorRect {
    long left   = 0;
    long top    = 0;
    long right  = 0;
    long bottom = 0;
};

struct gaCursorInfo {
public:
    bool visible;
    bool colored;
    gaPoint frame_pos;
    gaPoint hotspot;
    gaCursorRect src_rect;
    gaCursorRect dst_rect;
    long width;
    long height;
    long pitch;
    uint8_t* cursor_buffer;

public:
    explicit gaCursorInfo(): visible(false), colored(false), width(0), height(0), pitch(0), cursor_buffer(nullptr) {}

    gaCursorInfo(const gaCursorInfo& cursor_info) = default;

    gaCursorInfo& operator=(const gaCursorInfo&) = default;

    virtual ~gaCursorInfo() {
        if (cursor_buffer != nullptr) {
            //delete cursor_buffer;
            cursor_buffer = nullptr;
        }
    }
};
#endif

enum Severity {
    ERR,
    WARNING,
    INFO,
    DBG
};

EXPORT long long tvdiff_us(struct timeval *tv1, struct timeval *tv2);
EXPORT long long ga_usleep(long long interval, struct timeval *ptv);
EXPORT int    ga_logger(const Severity sev, const char *fmt, ...);
EXPORT int    ga_malloc(int size, void **ptr, int *alignment);
EXPORT int    ga_alignment(void *ptr, int alignto);
EXPORT long    ga_gettid();
EXPORT void    ga_dump_codecs();
EXPORT int    ga_init(const char *config);
EXPORT void    ga_deinit();
EXPORT void    ga_set_logfile(const char* file_name);
EXPORT std::string ga_compose_logname(std::string logname);
EXPORT void    ga_set_loglevel(enum Severity level);
EXPORT enum Severity  ga_get_loglevel();
EXPORT void    ga_openlog();
EXPORT void    ga_closelog();
// save file feature
EXPORT FILE *    ga_save_init(const char *filename);
EXPORT FILE *    ga_save_init_txt(const char *filename);
EXPORT int    ga_save_data(FILE *fp, unsigned char *buffer, int size);
EXPORT int    ga_save_printf(FILE *fp, const char *fmt, ...);
#if 0
EXPORT int    ga_save_yuv420p(FILE *fp, int w, int h, unsigned char *planes[], int linesize[]);
EXPORT int    ga_save_rgb4(FILE *fp, int w, int h, unsigned char *planes, int linesize);
#endif
EXPORT int    ga_save_close(FILE *fp);
// aggregated output feature
EXPORT void    ga_aggregated_reset();
EXPORT void    ga_aggregated_print(int key, int limit, int value);
// encoders or decoders would require this
EXPORT unsigned char * ga_find_startcode(unsigned char *buf, unsigned char *end, int *startcode_len);
//
EXPORT struct gaRect * ga_fillrect(struct gaRect *rect, int left, int top, int right, int bottom);
#ifdef WIN32
EXPORT int   ga_window_bounds(int &dw, int &dh, gaPoint &wlt, gaPoint &wrb, gaPoint &clt, gaPoint &crb);
#endif
EXPORT int    ga_crop_window(struct gaRect *rect, struct gaRect **prect);
EXPORT void    ga_backtrace();

EXPORT void    pthread_cancel_init();
#ifdef ANDROID
#include <pthread.h>
EXPORT int    pthread_cancel(pthread_t thread);
#endif

constexpr auto GA_E_INVALIDARG = -22;

#define GAALIGN(x, a)  (((x)+(a)-1) & ~((a)-1))

enum ga_audio_channel {
    GA_CH_FRONT_LEFT    = 0x00000001,
    GA_CH_FRONT_RIGHT   = 0x00000002,
    GA_CH_FRONT_CENTER  = 0x00000004,
    GA_CH_LOW_FREQUENCY = 0x00000008,
    GA_CH_BACK_LEFT     = 0x00000010,
    GA_CH_BACK_RIGHT    = 0x00000020,
};

enum ga_audio_layout {
    GA_CH_LAYOUT_MONO   = (ga_audio_channel::GA_CH_FRONT_CENTER),
    GA_CH_LAYOUT_STEREO = (ga_audio_channel::GA_CH_FRONT_LEFT | ga_audio_channel::GA_CH_FRONT_RIGHT),
    GA_CH_LAYOUT_QUAD   = (GA_CH_LAYOUT_STEREO | ga_audio_channel::GA_CH_BACK_LEFT | ga_audio_channel::GA_CH_BACK_RIGHT)
};

EXPORT int ga_get_channel_layout_nb_channels(uint64_t channel_layout);

enum class ga_sample_format {
    GA_SAMPLE_FMT_NONE = -1,
    GA_SAMPLE_FMT_U8,         //!< unsigned 8 bits
    GA_SAMPLE_FMT_S16,        //!< signed 16 bits
    GA_SAMPLE_FMT_S32,        //!< signed 32 bits
    GA_SAMPLE_FMT_FLT,        //!< float
    GA_SAMPLE_FMT_DBL,        //!< double
    GA_SAMPLE_FMT_U8P,        //!< unsigned 8 bits, planar
    GA_SAMPLE_FMT_S16P,       //!< signed 16 bits, planar
    GA_SAMPLE_FMT_S32P,       //!< signed 32 bits, planar
    GA_SAMPLE_FMT_FLTP,       //!< float, planar
    GA_SAMPLE_FMT_DBLP,       //!< double, planar
    GA_SAMPLE_FMT_NB          //!< Number of sample formats.
};

EXPORT int ga_samples_get_buffer_size(int* linesize, int nb_channels, int nb_samples, ga_sample_format sample_fmt, int align);

enum class ga_pixel_format {
    GA_PIX_FMT_NONE = -1,
    GA_PIX_FMT_YUV420P,       ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    GA_PIX_FMT_RGBA,          ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    GA_PIX_FMT_BGRA           ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
};

#endif
