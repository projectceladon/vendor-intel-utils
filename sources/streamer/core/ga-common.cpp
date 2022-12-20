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
 * implementation: common GA functions and macros
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#ifndef ANDROID
#include <execinfo.h>
#endif /* !ANDROID */
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#endif /* !WIN32 */
#ifdef ANDROID
#include <android/log.h>
#endif /* ANDROID */
#ifdef __APPLE__
#include <syslog.h>
#endif


#include "ga-common.h"
#include "ga-conf.h"
#include "rtspconf.h"

#include <map>
#include <list>
#include <limits>
using namespace std;

#ifndef NIPQUAD
/** For printing IPv4 addresses: convert an unsigned int to 4 unsigned char. */
#define NIPQUAD(x)    ((unsigned char*)&(x))[0],    \
            ((unsigned char*)&(x))[1],    \
            ((unsigned char*)&(x))[2],    \
            ((unsigned char*)&(x))[3]
#endif

/** The gloabl log file name */
static char *ga_logfile = NULL;


static Severity ga_loglevel = Severity::INFO;

/**
 * Compute the time difference for two \a timeval data structure, i.e.,
 * \a tv1 - \a tv2.
 *
 * @param tv1 [in] Pointer to the first \a timeval data structure.
 * @param tv2 [in] Pointer to the second \a timeval data structure.
 * @return The difference in micro seconds.
 */
EXPORT
long long
tvdiff_us(struct timeval *tv1, struct timeval *tv2) {
    struct timeval delta;
    delta.tv_sec = tv1->tv_sec - tv2->tv_sec;
    delta.tv_usec = tv1->tv_usec - tv2->tv_usec;
    if(delta.tv_usec < 0) {
        delta.tv_sec--;
        delta.tv_usec += 1000000;
    }
    return 1000000LL*delta.tv_sec + delta.tv_usec;
}

/**
 * Sleep and wake up at \a ptv + \a interval (micro seconds).
 *
 * @param interval [in] The expected sleeping time (in micro seconds).
 * @param ptv [in] Pointer to the baseline time.
 * @return Currently always return 0.
 *
 * This function is useful for controlling precise sleeps.
 * We usually have to process each video frame in a fixed interval.
 * Each time interval includes the processing time and the sleeping time.
 * However, the processing time could be different in each iteration, so
 * the sleeping time has to be adapted as well.
 * To achieve the goal, we have to obtain the baseline time \a ptv
 * (using \a gettimeofday function)
 * \em before the processing task and call this function \em after
 * the processing task. In this case, the \a interval is set to the total
 * length of the interval, e.g., 41667 for 24fps video.
 *
 * This function sleeps for \a interval micro seconds if the baseline
 * time is not specified.
 */
EXPORT
long long
ga_usleep(long long interval, struct timeval *ptv) {
    long long delta;
    struct timeval tv;
    if(ptv != NULL) {
        gettimeofday(&tv, NULL);
        delta = tvdiff_us(&tv, ptv);
        if(delta >= interval) {
            usleep(1);
            return -1;
        }
        interval -= delta;
    }
    usleep(interval);
    return 0LL;
}

/**
 * Write message \a s into the log file.
 * This in an internal function only called by \em ga_log function.
 *
 * @param tv [in] The timestamp of logging.
 * @param s [in] The message to be written.
 *
 * This function is SLOW, but it attempts to make all writes successful.
 * It appends the message into the file each time when it is called.
 */
static void
ga_writelog(struct timeval tv, const char *s, Severity level) {
    // suppress logs depending on loglevel
    if (level > ga_loglevel)
        return;

    FILE *fp;
    char sev[4][11] = {
        " [ERR]  : ",
        " [WARN] : ",
        " [INFO] : ",
        " [DBG]  : "};

#if __linux__
    fp = ((Severity::ERR == level) || (Severity::WARNING == level))? stderr: stdout;
    fprintf(fp, "[%d] %ld.%06ld %s %s", getpid(), tv.tv_sec, tv.tv_usec, sev[level], s);
#else
    if (Severity::ERR == level)
        fprintf(stderr, "# [%d] %ld.%06ld ERR : %s", getpid(), tv.tv_sec, tv.tv_usec, s);

    if(ga_logfile == NULL)
        return;
    if((fp = fopen(ga_logfile, "at")) != NULL) {
        fprintf(fp, "[%d][%5d] %ld.%06ld %s %s", getpid(), gettid(), tv.tv_sec, tv.tv_usec, sev[level], s);
        fclose(fp);
    }
#endif
    return;
}

/**
 * Write log messages and print on Android console.
 *
 * @param sev [in] The log message severity level.
 * @param fmt [in] The format string for the message.
 * @param ... [in] The arguments for replacing specifiers in the format string.
 *
 * This function has the same syntax as the \em printf function.
 * It outputs a timestamp before the message, and optionally writing
 * the message into a log file if log feature is turned on.
 */
EXPORT
int
ga_logger(Severity sev, const char *fmt, ...) {
    char msg[4096];
    struct timeval tv;
    va_list ap;
    //
    gettimeofday(&tv, NULL);
    va_start(ap, fmt);
#ifdef ANDROID
    __android_log_vprint(ANDROID_LOG_INFO, "ga_log.native", fmt, ap);
#endif
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
#ifdef __APPLE__
    syslog(LOG_NOTICE, "%s", msg);
#endif
    //
    ga_writelog(tv, msg, sev);
    //
    return 0;
}

/**
 * malloc() and return the offset to align pointer at 16-byte boundary.
 *
 * @param size [in] Requested memory space.
 * @param ptr [out] Pointer to the memory pointer.
 * @param alignment [out] Pointer to the alignment value.
 * @return 0 on success, or -1 on failure.
 *
 * Note: The actually allocated memory space is \a size + 16.
 * Data should be stored starting from \a *ptr + \a *alignment.
 */
EXPORT
int
ga_malloc(int size, void **ptr, int *alignment) {
    if((*ptr = malloc(size+16)) == NULL)
        return -1;
#if defined(__x86_64__) || defined(_WIN64)
    *alignment = 16 - (((unsigned long long) *ptr) & 0x0f);
#else
    *alignment = 16 - (((unsigned) *ptr) & 0x0f);
#endif
    return 0;
}

/**
 * Compute the alignment offset for a given pointer and the align-to value.
 *
 * @param ptr [in] The pointer address used to compute alignment offset.
 * @param alignto [in] The align-to value, must be 2^n, e.g., 1, 2, 4, 8, 16.
 * @return The alignment offset has to be added to the pointer address.
 *
 * Note that this function does not check the alignto value.
 * The caller must ensure that the \a alignto value must be 2^n, n >= 0.
 */
EXPORT
int
ga_alignment(void *ptr, int alignto) {
    int mask = alignto - 1;
#if defined(__x86_64__) || defined(_WIN64)
    return alignto - (((unsigned long long) ptr) & mask);
#else
    return alignto - (((unsigned) ptr) & mask);
#endif
}

/**
 * Get the thread ID in long format.
 */
EXPORT
long
ga_gettid() {
#ifdef WIN32
    return GetCurrentThreadId();
#elif defined __APPLE__
    return pthread_mach_thread_np(pthread_self());
#elif defined ANDROID
    return gettid();
#else
    return (pid_t) syscall(SYS_gettid);
#endif
}

/**
 * Initialize windows socket sub-system.
 *
 * @return 0 on success and -1 on error.
 */
static int
winsock_init() {
#ifdef WIN32
    WSADATA wd;
    if(WSAStartup(MAKEWORD(2,2), &wd) != 0)
        return -1;
#endif
    return 0;
}

/**
 * Initialize GamingAnywhere
 *
 * @param config [in] Pathname to the configuration file
 * @return 0 on success or -1 on error
 */
EXPORT
int
ga_init(const char *config) {
    srand((unsigned int)time(0));
    winsock_init();

    if(config != NULL) {
        if(ga_conf_load(config) < 0) {
            fprintf(stderr, "GA: cannot load configuration file '%s'\n", config);
            return -1;
        }
    }
    return 0;
}

/**
 * Deinitialize GamingAnywhere: currently do nothing
 */
EXPORT
void
ga_deinit() {
    return;
}


/**
 * Set log file name
 */
EXPORT
void
ga_set_logfile(const char* file_name) {
	if (file_name == nullptr) {
		fprintf(stderr, "Error : log file name is not set\n");
		exit(-1);
	}

	ga_conf_writev("logfile", file_name);
}


/**
 * Set level of messages to be printed
 */
EXPORT
void
ga_set_loglevel(Severity level) {
    ga_loglevel = level;
}

/**
 * Get current level of messages
 */
EXPORT
Severity
ga_get_loglevel() {
        return ga_loglevel;
}


EXPORT
std::string ga_compose_logname(std::string logname)
{
	std::string name = logname;
	auto pidPos = name.find("PID");
	if (pidPos != std::string::npos) // case sensitive
		name.replace(pidPos, strlen("PID"), std::to_string(getpid()));
	return name;
}


/**
 * Collecting target system information stored in ga_sysinfo.
 *
 * This function calls once at beginning of each CG instance.
 */
void
ga_system_info() {
#if defined(WIN32)

    std::string result;
    std::string cmd_prefix = "PowerShell.exe -NoLogo -Command \"$ProgressPreference = 'SilentlyContinue';";
    std::string cmd_postfix = "\"";

    ga_logger(Severity::INFO, "********************************************************************************\n");

    ga_logger(Severity::INFO, "CG Version   : %s\n", CG_VERSION);

    RunSystemCommand(cmd_prefix +
        "$computerSys = Get-WmiObject Win32_ComputerSystem;"
        "Write-Host ('Host Name    : {0}' -f $computerSys.DNSHostName)"
        + cmd_postfix, result);
    ga_logger(Severity::INFO, "%s\n", result.c_str());

    RunSystemCommand(cmd_prefix +
        "$os = Get-WmiObject Win32_OperatingSystem;"
        "Write-Host ('OS Version   : {0} - {1}' -f $os.Caption,$os.Version)"
        + cmd_postfix, result);
    ga_logger(Severity::INFO, "%s\n", result.c_str());

    RunSystemCommand(cmd_prefix +
        "$bios = Get-WmiObject Win32_BIOS;"
        "Write-Host ('BIOS Version : {0}, {1}, {2}' -f $bios.BIOSVersion[0],$bios.BIOSVersion[1],$bios.BIOSVersion[2])"
        + cmd_postfix, result);
    ga_logger(Severity::INFO, "%s\n", result.c_str());

    RunSystemCommand(cmd_prefix +
        "foreach($cpu in Get-WmiObject Win32_Processor)"
        "{ Write-Host ('CPU          : {0} - [Cores={1}, Logical={2}] {3}' -f $cpu.DeviceID,$cpu.NumberOfEnabledCore,$cpu.NumberOfLogicalProcessors,$cpu.Name) }"
        + cmd_postfix, result);
    ga_logger(Severity::INFO, "%s\n", result.c_str());

    RunSystemCommand(cmd_prefix +
        "foreach($gpu in Get-WmiObject Win32_VideoController) "
        "{ Write-Host ('GPU          : {0} - {1} - {2} - {3}' -f $gpu.DeviceID,$gpu.Description,$gpu.DriverVersion,$gpu.VideoModeDescription) }"
        + cmd_postfix, result);
    ga_logger(Severity::INFO, "%s\n", result.c_str());

    RunSystemCommand(cmd_prefix +
        "$pwr = $(PowerCfg /GETACTIVESCHEME) -replace \\\".*:\\\";"
        "Write-Host ('Power Scheme :{0}' -f $pwr)"
        + cmd_postfix, result);
    ga_logger(Severity::INFO, "%s\n", result.c_str());

    ga_logger(Severity::INFO, "********************************************************************************\n");

#endif  // defined(WIN32)
}

/**
 * Enable log feature
 *
 * This function must be called if you plan to write logs into a file.
 * It reads the \em logfile option specified in the configuration file.
 */
EXPORT
void
ga_openlog() {
    FILE *fp;
    //
    std::string fn = ga_conf_readstr("logfile");
    if(fn.empty())
        return;

    fn = ga_compose_logname(fn);

    if((fp = fopen(fn.c_str(), "at")) != NULL) {
        fclose(fp);
        ga_logfile = strdup(fn.c_str());
        if (ga_logfile != NULL) {
            // Log target system information at beginning of the logfile
            ga_system_info();
        }
    }
    //
    return;
}

/**
 * Disable log feature
 */
EXPORT
void
ga_closelog() {
    if(ga_logfile != NULL) {
        free(ga_logfile);
        ga_logfile = NULL;
    }
    return;
}

// save file feature

/**
 * Internal function for \em ga_save_init and \em ga_save_init_txt.
 * Returns the FILE pointer.
 *
 * @param filename [in] Pathname to store the image data.
 * @param mode [in] File access mode pass to \em fopen function.
 * @return FILE pointer to the opened file.
 */
static FILE *
ga_save_init_internal(const char *filename, const char *mode) {
    FILE *fp = NULL;
    if(filename != NULL) {
        fp = fopen(filename, mode);
        if(fp == NULL) {
            ga_logger(Severity::ERR, "save file: open %s failed.\n", filename);
        }
    }
    return fp;
}

/**
 * Return the FILE pointer used to store saved raw image frame data.
 *
 * @param filename [in] Pathname to store the image data.
 * @return FILE pointer to the opened file.
 *
 * This function must be called if you plan to use the
 * save raw video image feature.
 * All captured images will be stored in this single file.
 */
EXPORT
FILE *
ga_save_init(const char *filename) {
    return ga_save_init_internal(filename, "wb");
}

/**
 * Return the FILE pointer used to store text-based meta-data for saved image frame.
 *
 * @param filename [in] Pathname to store the text-based meta-data.
 * @return the FILE pointer to the opened file.
 *
 * This function can be called optionally when you need to store
 * text-based meta data for saved raw video image.
 * All stored meta-data will be stored in this single file.
 */
EXPORT
FILE *
ga_save_init_txt(const char *filename) {
    return ga_save_init_internal(filename, "wt");
}

/**
 * Save arbitrary binary data.
 *
 * @param fp [in] FILE pointer to the opened file (by \em ga_save_init).
 * @param buffer [in] Pointer to the data memory.
 * @param size [in] Size of the data, in bytes.
 */
EXPORT
int
ga_save_data(FILE *fp, unsigned char *buffer, int size) {
    if(fp == NULL || buffer == NULL || size < 0)
        return -1;
    if(size == 0)
        return 0;
    return static_cast<int>(fwrite(buffer, sizeof(char), size, fp));
}

/**
 * Save text data.
 *
 * @param fp [in] FILE pointer to the opened file (by \em ga_save_init_txt).
 * @param fmt [in] Pointer the for format string.
 * @param ... [in] Arguments for specifiers in the format string.
 */
EXPORT
int
ga_save_printf(FILE *fp, const char *fmt, ...) {
    int wlen;
    va_list ap;
    if(fp == NULL)
        return -1;
    va_start(ap, fmt);
    wlen = vfprintf(fp, fmt, ap);
    va_end(ap);
    fflush(fp);
    return wlen;
}

#if 0
/**
 * Save YUV420 image frame data
 *
 * @param fp [in] FILE pointer to the opened file (by \em ga_save_init).
 * @param w [in] Width of the frame.
 * @param h [in] Height of the frame.
 * @param planes [in] Pointers to address of (3) planes [Y, U, and V].
 * @param linesize [in] Pointers to linesize.
 */
EXPORT
int
ga_save_yuv420p(FILE *fp, int w, int h, unsigned char *planes[], int linesize[]) {
    size_t wlen, written = 0;
    int w2 = w/2;
    int h2 = h/2;
    int expected = w * h * 3 / 2;
    unsigned char *src;
    if(fp == NULL || w <= 0 || h <= 0 || planes == NULL || linesize == NULL)
        return -1;
    // write Y
    src = planes[0];
    for(int i = 0; i < h; i++) {
        if((wlen = fwrite(src, sizeof(char), w, fp)) < 0)
            goto err_save_yuv;
        written += wlen;
        src += linesize[0];
    }
#ifdef WIN32
    src = planes[1];
    for (int i = 0; i < h2; i++) {
        if ((wlen = fwrite(src, sizeof(char), w, fp)) < 0)
            goto err_save_yuv;
        written += wlen;
        src += linesize[0];
    }
#endif

#if 0
    // write U/V
    for(int j = 1; j < 3; j++) {
        src = planes[j];
        for(int i = 0; i < h2; i++) {
            if((wlen = fwrite(src, sizeof(char), w2, fp)) < 0)
                goto err_save_yuv;
            written += wlen;
            src += linesize[j];
        }
    }
    //

    if(written != expected) {
        ga_logger(Severity::INFO, "save YUV (%dx%d): expected %d, save %d (frame may be corrupted)\n",
            w, h, expected, written);
    }
#endif
    //
    fflush(fp);
    return static_cast<int>(written);
err_save_yuv:
    return -1;
}

/**
 * Save RGB4 image frame data
 *
 * @param fp [in] FILE pointer to the opened file (by \em ga_save_init).
 * @param w [in] Width of the frame.
 * @param h [in] Height of the frame.
 * @param planes [in] Pointers to the image data memory.
 * @param linesize [in] linesize, usually \a h * pixel size plus (optional) alignments.
 */
EXPORT
int
ga_save_rgb4(FILE *fp, int w, int h, unsigned char *planes, int linesize) {
    size_t wlen, written = 0;
    int w4 = w*4;
    int expected = w * h * 4;
    if(fp == NULL || w <= 0 || h <= 0 || planes == NULL || linesize < w4)
        return -1;
    // write
    for(int i = 0; i < h; i++) {
        if((wlen = fwrite(planes, sizeof(char), w4, fp)) < 0)
            goto err_save_rgb4;
        written += wlen;
        planes += linesize;
    }
    //
    if(written != expected) {
        ga_logger(Severity::INFO, "save RGB4 (%dx%d): expected %d, save %d (frame may be corrupted)\n",
            w, h, expected, written);
    }
    //
    fflush(fp);
    return static_cast<int>(written);
err_save_rgb4:
    return -1;
}
#endif

/**
 * Close a file opened by \em ga_save_init or \em ga_save_init_txt
 *
 * @param fp [in] The opened file pointer.
 * @return This function currently always returns 0.
 */
EXPORT
int
ga_save_close(FILE *fp) {
    if(fp != NULL) {
        fclose(fp);
    }
    return 0;
}

//

static map<int,list<int> > aggmap;

/**
 * Clear everything in the aggregated output buffer.
 */
EXPORT
void
ga_aggregated_reset() {
    aggmap.clear();
    return;
}

/**
 * Output aggregated statistic numbers periodically
 *
 * @param key [in] The key used to uniquely identify an record series.
 * @param limit [in] Output buffered records when the number of records
 *        reaches the \a limit
 * @param value [in] The value to be output
 *
 * This function is used to reduce the output frequency of statistics.
 * If you need to output integer statistics but does not want the output
 * affect the performance too much.
 * You can consider use the aggregated output function.
 * Call this function to output an integer value as usual but the output
 * is only output when the number of records reaches the limit.
 * The \a key value must be different for different statistic series.
 * The \a limit value would be better to be a distinct prime number:
 * To prevent output from different series collides at the same time.
 */
EXPORT
void
ga_aggregated_print(int key, unsigned int limit, int value) {
    map<int,list<int> >::iterator mi;
    // push data
    if((mi = aggmap.find(key)) == aggmap.end()) {
        aggmap[key].push_back(value);
        return;
    } else {
        mi->second.push_back(value);
    }
    // output?
    if(mi->second.size() >= limit) {
        int pos, left, wlen;
        char *ptr, buf[16384] = "AGGREGATED-VALUES:";
        list<int>::iterator li;
        struct timeval tv;
        //
        pos = snprintf(buf, sizeof(buf), "AGGREGATED-OUTPUT[%04x]:", key);
        left = sizeof(buf) - pos;
        ptr = buf + pos;
        for(li = mi->second.begin(); li != mi->second.end() && left>16; li++) {
            wlen = snprintf(ptr, left, " %d", *li);
            ptr += wlen;
            left -= wlen;
        }
        if(li != mi->second.end()) {
            ga_logger(Severity::INFO, "insufficient space for aggregated messages.\n");
        }
        mi->second.clear();
        //
        gettimeofday(&tv, NULL);
#ifdef ANDROID
        __android_log_write(ANDROID_LOG_INFO, "ga_log.native", buf);
#else
        fprintf(stderr, "# [%d] %ld.%06ld %s\n",
            getpid(), tv.tv_sec, tv.tv_usec, buf);
#endif
        ga_writelog(tv, buf, Severity::INFO);
    }
    //
    return;
}

/**
 * Find mpeg start code 00 00 01 or 00 00 00 01.
 *
 * @param buf [in] The byte buffer to search.
 * @param end [in] End of the byte buffer.
 * @param startcode_len [out] Length of start code.
 * @return pointer to the beginning of the start code, or NULL if not found.
 *
 * If a non-NULL pointer is returned, you can read the frame data start from
 * the returned pointer plus \a startcode_len.
 */
EXPORT
unsigned char *
ga_find_startcode(unsigned char *buf, unsigned char *end, int *startcode_len) {
    unsigned char *ptr;
    for(ptr = buf; ptr < end-4; ptr++) {
        if(*ptr == 0 && *(ptr+1)==0) {
            if(*(ptr+2) == 1) {
                *startcode_len = 3;
                return ptr;
            } else if(*(ptr+2)==0 && *(ptr+3)==1) {
                *startcode_len = 4;
                return ptr;
            }
        }
    }
    return NULL;
}

EXPORT
struct gaRect *
ga_fillrect(struct gaRect *rect, int left, int top, int right, int bottom) {
    if(rect == NULL)
        return NULL;
#define SWAP(a,b)    do { int tmp = a; a = b; b = tmp; } while(0);
    if(left > right)
        SWAP(left, right);
    if(top > bottom)
        SWAP(top, bottom);
#undef    SWAP
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
    //
    rect->width = rect->right - rect->left + 1;
    rect->height = rect->bottom - rect->top + 1;
    rect->linesize = rect->width * RGBA_SIZE;
    rect->size = rect->width * rect->height * RGBA_SIZE;
    //
    if(rect->width <= 0 || rect->height <= 0) {
        ga_logger(Severity::ERR, "# invalid rect size (%dx%d)\n", rect->width, rect->height);
        return NULL;
    }
    //
    return rect;
}

#ifdef WIN32

#ifdef WIN32
struct handle_data {
    unsigned long process_id;
    HWND window_handle;
};
BOOL is_main_window(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}
BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
    handle_data& data = *(handle_data*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !is_main_window(handle))
        return TRUE;
    data.window_handle = handle;
    return FALSE;
}
HWND find_main_window(unsigned long process_id)
{
    handle_data data;
    data.process_id = process_id;
    data.window_handle = 0;
    EnumWindows(enum_windows_callback, (LPARAM)&data);
    return data.window_handle;
}
#endif

EXPORT int
ga_window_bounds(int &dw, int &dh, gaPoint &wlt, gaPoint &wrb, gaPoint &clt, gaPoint &crb) {
    int find_wnd_arg = 0;
    HWND hWnd;
    RECT window;
    RECT client;

    if((hWnd = GetWindowHandleOfCurrentProcess()) == NULL) {
        ga_logger(Severity::ERR, "GetWindowHandleOfCurrentProcess failed.\n");
        return -1;
    }

    dw = GetSystemMetrics(SM_CXSCREEN);
    dh = GetSystemMetrics(SM_CYSCREEN);

    if(SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW) == 0) {
        ga_logger(Severity::ERR, "SetWindowPos failed.\n");
        return -1;
    }

    if(GetClientRect(hWnd, &client) == 0) {
        ga_logger(Severity::ERR, "GetClientRect failed.\n");
        return -1;
    }
    else {
        clt.x = client.left;
        clt.y = client.top;
        crb.x = client.right-1;
        crb.y = client.bottom-1;
    }

    POINT cltPoint = {clt.x, clt.y};
    POINT crbPoint = {crb.x, crb.y};
    if(ClientToScreen(hWnd, &cltPoint) == 0
    || ClientToScreen(hWnd, &crbPoint) == 0) {
        ga_logger(Severity::ERR, "Map from client coordinate to screen coordinate failed.\n");
        return -1;
    }
    clt.x = cltPoint.x;
    clt.y = cltPoint.y;
    crb.x = crbPoint.x;
    crb.y = crbPoint.y;

    if(GetWindowRect(hWnd, &window) == 0) {
        ga_logger(Severity::ERR, "GetClientRect failed.\n");
        return -1;
    }
    else {
        wlt.x = window.left;
        wlt.y = window.top;
        wrb.x = window.right-1;
        wrb.y = window.bottom-1;
    }
    
    return 1;
}

EXPORT
int
ga_crop_window(struct gaRect *rect, struct gaRect **prect) {
    char wndname[1024], wndclass[1024];
    char *pname;
    char *pclass;
    int dw, dh, find_wnd_arg = 0;
    HWND hWnd;
    RECT client;
    POINT lt, rb;
    //
    if(rect == NULL || prect == NULL)
        return -1;
    //
    pname = ga_conf_readv("find-window-name", wndname, sizeof(wndname));
    pclass = ga_conf_readv("find-window-class", wndclass, sizeof(wndclass));
    //
    if(pname != NULL && *pname != '\0')
        find_wnd_arg++;
    if(pclass != NULL && *pclass != '\0')
        find_wnd_arg++;
    if(find_wnd_arg <= 0) {
        *prect = NULL;
        return 0;
    }
    //

    /*
    hWnd =  find_main_window(23156);

    */
    if((hWnd = FindWindow(pclass, pname)) == NULL) {
        ga_logger(Severity::ERR, "FindWindow failed for '%s/%s'\n",
            pclass ? pclass : "",
            pname ? pname : "");
        return -1;
    }

    //
    GetWindowText(hWnd, wndname, sizeof(wndname));
    dw = GetSystemMetrics(SM_CXSCREEN);
    dh = GetSystemMetrics(SM_CYSCREEN);
    //
    ga_logger(Severity::INFO, "Found window (0x%08x) :%s%s%s%s\n", hWnd,
        pclass ? " class=" : "",
        pclass ? pclass : "",
        pname ? " name=" : "",
        pname ? pname : "");
    //
    if(SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW) == 0) {
        ga_logger(Severity::ERR, "SetWindowPos failed.\n");
        return -1;
    }
    if(GetClientRect(hWnd, &client) == 0) {
        ga_logger(Severity::ERR, "GetClientRect failed.\n");
        return -1;
    }
    if(SetForegroundWindow(hWnd) == 0) {
        ga_logger(Severity::ERR, "SetForegroundWindow failed.\n");
    }
    //
    lt.x = client.left;
    lt.y = client.top;
    rb.x = client.right-1;
    rb.y = client.bottom-1;
    //
    if(ClientToScreen(hWnd, &lt) == 0
    || ClientToScreen(hWnd, &rb) == 0) {
        ga_logger(Severity::ERR, "Map from client coordinate to screen coordinate failed.\n");
        return -1;
    }
    //
    rect->left = lt.x;
    rect->top = lt.y;
    rect->right = rb.x;
    rect->bottom = rb.y;
    // size check: multiples of 2?
    if((rect->right - rect->left + 1) % 2 != 0)
        rect->left--;
    if((rect->bottom - rect->top + 1) % 2 != 0)
        rect->top--;
    //
    if(rect->left < 0 || rect->top < 0 || rect->right >= dw || rect->bottom >= dh) {
        ga_logger(Severity::ERR, "Invalid window: (%d,%d)-(%d,%d) w=%d h=%d (screen dimension = %dx%d).\n",
            rect->left, rect->top, rect->right, rect->bottom,
            rect->right - rect->left + 1,
            rect->bottom - rect->top + 1);
        return -1;
    }
    //
    *prect = rect;
    return 1;
}
#else
int
ga_crop_window(struct gaRect *rect, struct gaRect **prect) {
    // XXX: implement find window for Apple
    *prect = NULL;
    return 0;
}
#endif

/**
 * Show the backtrace of current process
 * This function is only for debug purpose.
 */
EXPORT
void
ga_backtrace() {
#if defined(WIN32) || defined(ANDROID)
    return;
#else
    int j, nptrs;
#define SIZE 100
    void *buffer[SIZE];
    char **strings;

    nptrs = backtrace(buffer, SIZE);
    printf("-- backtrace() returned %d addresses -----------\n", nptrs);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(-1);
    }

    for (j = 0; j < nptrs; j++)
        printf("%s\n", strings[j]);

    free(strings);
    printf("------------------------------------------------\n");
#endif    /* WIN32 */
}

#ifdef ANDROID
/**
 * Built-in signal handler for emulating pthread_cancel.
 */
static void
pthread_cancel_handler(int s) {
    if(s == SIGUSR2) {
        pthread_exit(NULL);
    }
    return;
}
#endif

/**
 * Initialize the emulated pthread_cancel function.
 *
 * This function MUST be called before calling pthread_cancel.
 * Otherwise, the entire process would be killed.
 * pthread_cancel() emulating is done by using pthread_kill,
 * which wake up the specified thread to handle the signal.
 *
 * This function can be called only once because a handler is
 * registered process-wide.
 *
 * This function registers a handler for SIGUSR2.
 */
EXPORT
void
pthread_cancel_init() {
#ifdef ANDROID
    signal(SIGUSR2, pthread_cancel_handler);
#endif
    return;
}

#ifdef ANDROID
/**
 * Emulate pthread_cancel in Android
 *
 * @param thread [in] thread id.
 *
 * The thread to be cancelled must be setup by calling pthread_cancel_init().
 * Otherwise, it could be terminated, and not sure if there would be side-effect
 * or not.
 */
EXPORT
int
pthread_cancel(pthread_t thread) {
    return pthread_kill(thread, SIGUSR2);
}
#endif

/**
* Get number of channels in the channel layout
*
* @param channel_layout The channel layout
*
* @return The number of channels in the channel layout
*/
EXPORT
int ga_get_channel_layout_nb_channels(uint64_t channel_layout) {
    int count = 0;

    if (channel_layout == 0)
        return 0;

    while (channel_layout != 0) {
        channel_layout &= (channel_layout - 1);
        count++;
    }

    return count;
}

/**
* Sample format information data structure
*/
typedef struct SampleFmtInfo {
    char              name[8];  //!< Sample format name
    int               bits;     //!< Number of bits per sample
    bool              planar;   //!< Sample format is planar
    ga_sample_format  alt_fmt;  //!< Planar<->packed alternative form
} SampleFmtInfo;

/**
* Table of sample format informaiton
*/
std::map<ga_sample_format, SampleFmtInfo> sample_fmt_info = {
    { ga_sample_format::GA_SAMPLE_FMT_U8,   SampleFmtInfo{   "u8",  8, false, ga_sample_format::GA_SAMPLE_FMT_U8P  } },
    { ga_sample_format::GA_SAMPLE_FMT_S16,  SampleFmtInfo{  "s16", 16, false, ga_sample_format::GA_SAMPLE_FMT_S16P } },
    { ga_sample_format::GA_SAMPLE_FMT_S32,  SampleFmtInfo{  "s32", 32, false, ga_sample_format::GA_SAMPLE_FMT_S32P } },
    { ga_sample_format::GA_SAMPLE_FMT_FLT,  SampleFmtInfo{  "flt", 32, false, ga_sample_format::GA_SAMPLE_FMT_FLTP } },
    { ga_sample_format::GA_SAMPLE_FMT_DBL,  SampleFmtInfo{  "dbl", 64, false, ga_sample_format::GA_SAMPLE_FMT_DBLP } },
    { ga_sample_format::GA_SAMPLE_FMT_U8P,  SampleFmtInfo{  "u8p",  8,  true, ga_sample_format::GA_SAMPLE_FMT_U8   } },
    { ga_sample_format::GA_SAMPLE_FMT_S16P, SampleFmtInfo{ "s16p", 16,  true, ga_sample_format::GA_SAMPLE_FMT_S16  } },
    { ga_sample_format::GA_SAMPLE_FMT_S32P, SampleFmtInfo{ "s32p", 32,  true, ga_sample_format::GA_SAMPLE_FMT_S32  } },
    { ga_sample_format::GA_SAMPLE_FMT_FLTP, SampleFmtInfo{ "fltp", 32,  true, ga_sample_format::GA_SAMPLE_FMT_FLT  } },
    { ga_sample_format::GA_SAMPLE_FMT_DBLP, SampleFmtInfo{ "dblp", 64,  true, ga_sample_format::GA_SAMPLE_FMT_DBL  } }
};

/**
* Get number of bytes per sample
*
* @param sample_fmt The sample format
*
* @return Return number of bytes of the sample format
*/
int
ga_get_bytes_per_sample(ga_sample_format sample_fmt) {
    std::map<ga_sample_format, SampleFmtInfo>::iterator it = sample_fmt_info.find(sample_fmt);
    if (it != sample_fmt_info.end()) {
        return it->second.bits >> 3;
    }
    return 0;
}

/**
* Is sample format planar
*
* @param sample_fmt    The sample format
*
* @return Return true if planar format, otherwise return false
*/
bool
ga_sample_fmt_is_planar(ga_sample_format sample_fmt) {
    std::map<ga_sample_format, SampleFmtInfo>::iterator it = sample_fmt_info.find(sample_fmt);
    if (it != sample_fmt_info.end()) {
        return it->second.planar;
    }
    return false;
}

/**
* Get the required buffer size for the given audio parameters.
*
* @param[out] linesize    Calculated line size, may be nullptr
* @param      nb_channels The number of channels
* @param      nb_samples  The number of samples in single channel
* @param      sample_fmt  The sample format
* @param      align       Buffer size alignment (0 = default, 1 = no alignment)
*
* @return Required buffer size, or negative error code on failure
*/
EXPORT
int
ga_samples_get_buffer_size(int* linesize, int nb_channels, int nb_samples, ga_sample_format sample_fmt, int align) {
    static constexpr int int_max = (numeric_limits<int>::max)();
    int line_size = 0;
    int sample_size = ga_get_bytes_per_sample(sample_fmt);
    bool planar = ga_sample_fmt_is_planar(sample_fmt);

    if (!sample_size || nb_samples <= 0 || nb_channels <= 0)
        return GA_E_INVALIDARG;

    if (!align) {
        if (nb_samples > int_max - 31)
            return GA_E_INVALIDARG;
        align = 1;
        nb_samples = GAALIGN(nb_samples, 32);
    }

    if ((nb_channels > int_max / align) || ((nb_channels * nb_samples) > ((int_max - (align * nb_channels)) / sample_size)))
        return GA_E_INVALIDARG;

    line_size = planar ? GAALIGN(nb_samples * sample_size, align) :
                         GAALIGN(nb_samples * sample_size * nb_channels, align);
    if (linesize)
        *linesize = line_size;

    return planar ? line_size * nb_channels : line_size;
}
