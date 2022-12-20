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
 * Common functions and definitions for implementing a GamingAnywhere module.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <map>
#include <thread>
#ifndef WIN32
#include <dlfcn.h>
#endif

#include "ga-common.h"
#include "ga-module.h"

using namespace std;

/**
 * Map to keep loaded modules.
 */
static map<ga_module_t *, HMODULE> mlist;

/**
 * Load a module.
 *
 * @param modname [in] module (file) name without its extension.
 * @param prefix [in] unused now.
 * @return The loaded module in \em ga_module_t structure.
 *
 * Note that the module name should not contain its filename extension.
 * Filename extension is automatically specified based on its platform.
 *
 * A GamingAnywhere module must implement the \em module_load() function.
 * This function has to return an instance of the \em ga_module_t,
 * which is then returned to the caller of this function.
 */
ga_module_t *
ga_load_module(const char *modname, const char *prefix) {
    char fn[1024];
    ga_module_t *m;
    HMODULE handle;
    ga_module_t * (*do_module_load)();
#ifdef WIN32
    snprintf(fn, sizeof(fn), "%s.dll", modname);
#elif defined __APPLE__
    snprintf(fn, sizeof(fn), "%s.dylib", modname);
#else
    snprintf(fn, sizeof(fn), "%s.so", modname);
#endif
    //
    if((handle = dlopen(fn, RTLD_NOW|RTLD_LOCAL)) == NULL) {
        ga_logger(Severity::ERR, "ga_load_module: load module (%s) failed - %s.\n", fn, dlerror());
        return NULL;
    }
    if((do_module_load = (ga_module_t * (*)()) dlsym(handle, "module_load")) == NULL) {
        ga_logger(Severity::ERR, "ga_load_module: [%s] is not a valid module.\n", fn);
        dlclose(handle);
        return NULL;
    }
    if((m = do_module_load()) != NULL) {
        mlist[m] = handle;
    }
    //
    return m;
}

/**
 * Unload a module.
 *
 * @param m [in] Pointer to a loaded module.
 */
void
ga_unload_module(ga_module_t *m) {
    map<ga_module_t *, HMODULE>::iterator mi;
    if(m == NULL)
        return;
    if((mi = mlist.find(m)) == mlist.end())
        return;
    dlclose(mi->second);
    mlist.erase(mi);
    return;
}

/**
 * Initialize a module by calling module->init function
 *
 * @param name [in] Name of this module - just for logging purpose.
 * @param m [in] The module instance.
 * @param arg [in] The argument pass to the init function.
 *    Note that the argument could be different for different types of module.
 * @return 0 on success, or -1 on error.
 */
 #ifdef WIN32
int
ga_init_single_module(const char *name, ga_module_t *m, void *arg, void(*p)(struct timeval)) {
    if(m->init == NULL)
        return 0;
    if(m->init(arg, p) < 0) {
        ga_logger(Severity::ERR, "%s init failed.\n", name);
        return -1;
    }
    return 0;
}
#else
int
ga_init_single_module(const char *name, ga_module_t *m, void *arg) {
    if(m->init == NULL)
        return 0;
    if(m->init(arg) < 0) {
        ga_logger(Severity::ERR, "%s init failed.\n", name);
        return -1;
    }
    return 0;
}
#endif

/**
 * Initialize a module, or quit if the initialization is failed.
 *
 * @param name [in] Name of this module - just for logging purpose.
 * @param m [in] The module instance.
 * @param arg [in] The argument pass to the init function.
 *    Note that the argument could be different for different types of module.
 *
 * This function calls \em ga_init_single_module, and quit the program
 * if an error is returned from that function.
 */
 #ifdef WIN32
void
ga_init_single_module_or_quit(const char *name, ga_module_t *m, void *arg, void(*p)(struct timeval)) {
    if(ga_init_single_module(name, m, arg, p) < 0)
        exit(-1);
    return;
}
#else
void
ga_init_single_module_or_quit(const char *name, ga_module_t *m, void *arg) {
    if(ga_init_single_module(name, m, arg) < 0)
        exit(-1);
    return;
}
#endif

/**
 * Run a module's thread procedure. XXX: Could be removed in the future.
 *
 * @param name [in] Name of this module - just for logging purpose.
 * @param threadproc [in] The thread procedure.
 * @param arg [in] The argument pass to the thread procedure.
 * @return 0 on success, or -1 on error.
 *
 * It is not recommended to use this function.
 * Use module->start() instead on modern module implementations.
 */
int
ga_run_single_module(const char *name, void * (*threadproc)(void*), void *arg) {
    if(threadproc == NULL)
        return 0;
    try {
        std::thread th(*threadproc, arg);
        th.detach();
    } catch(...) {
        ga_logger(Severity::ERR, "cannot create %s thread\n", name);
        return -1;
    }
    return 0;
}

/**
 * Run a module's thread procedure, or quit if launch failed.
 * XXX: Could be removed in the future.
 *
 * @param name [in] Name of this module - just for logging purpose.
 * @param threadproc [in] The thread procedure.
 * @param arg [in] The argument pass to the thread procedure.
 * @return 0 on success, or -1 on error.
 */
void
ga_run_single_module_or_quit(const char *name, void * (*threadproc)(void*), void *arg) {
    if(ga_run_single_module(name, threadproc, arg) < 0)
        exit(-1);
    return;
}

/**
 * Wrapper for module's init() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param arg [in] Pointer to the argument.
 *
 * We recommend to use this function instead of calling \em m->init() directly.
 * This function ensures that a module has defined the \em init interface
 * before calling the interface.
 *
 * This function returns 0 (no error) if the interface is not defined.
 */
 #ifdef WIN32
int
ga_module_init(ga_module_t *m, void *arg, void(*p)(struct timeval)) {
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->init == NULL)
        return 0;
    return m->init(arg,p);
}
#else
int
ga_module_init(ga_module_t *m, void *arg) {
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->init == NULL)
        return 0;
    return m->init(arg);
}
#endif

/**
 * Wrapper for module's start() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param arg [in] Pointer to the argument.
 *
 * We recommend to use this function instead of calling \em m->start() directly.
 * This function ensures that a module has defined the \em start interface
 * before calling the interface.
 */
int
ga_module_start(ga_module_t *m, void *arg) {
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->start == NULL)
        return GA_IOCTL_ERR_NOINTERFACE;
    return m->start(arg);
}

/**
 * Wrapper for module's stop() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param arg [in] Pointer to the argument.
 *
 * We recommend to use this function instead of calling \em m->stop() directly.
 * This function ensures that a module has defined the \em stop interface
 * before calling the interface.
 *
 * This function returns 0 (no error) if the interface is not defined.
 */
int
ga_module_stop(ga_module_t *m, void *arg) {
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->stop == NULL)
        return 0;
    return m->stop(arg);
}

/**
 * Wrapper for module's deinit() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param arg [in] Pointer to the argument.
 *
 * We recommend to use this function instead of calling \em m->deinit() directly.
 * This function ensures that a module has defined the \em deinit interface
 * before calling the interface.
 *
 * This function returns 0 (no error) if the interface is not defined.
 */
int
ga_module_deinit(ga_module_t *m, void *arg) {
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->deinit == NULL)
        return 0;
    return m->deinit(arg);
}

/**
 * Wrapper for module's ioctl() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param command [in] The ioctl() command.
 * @param argsize [in] The size of the argument.
 * @param arg [in] Pointer to the argument.
 *
 * We recommend to use this function instead of calling \em m->ioctl() directly.
 * This function ensures that a module has defined the \em ioctl interface
 * before sending ioctl commands.
 */
int
ga_module_ioctl(ga_module_t *m, int command, int argsize, void *arg) {
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->ioctl == NULL)
        return GA_IOCTL_ERR_NOIOCTL;
    return m->ioctl(command, argsize, arg);
}

/**
 * Wrapper for module's notify() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param arg [in] Pointer to the argument.
 *
 * We recommend to use this function instead of calling \em m->notify() directly.
 * This function ensures that a module has defined the \em notify interface
 * before calling the interface.
 *
 * This function returns 0 (no error) if the interface is not defined.
 */
int
ga_module_notify(ga_module_t *m, void *arg) {
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->notify == NULL)
        return 0;
    return m->notify(arg);
}

/**
 * Wrapper for module's raw() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param arg [out] Pointer to the store the raw instance.
 * @param size [out] Pointer to the store the size of the raw instance.
 *
 * We recommend to use this function instead of calling \em m->raw() directly.
 * This function ensures that a module has defined the \em raw interface
 * before calling the interface.
 */
void *
ga_module_raw(ga_module_t *m, void *arg, int *size) {
    if(m == NULL)
        return NULL;
    if(m->raw == NULL)
        return NULL;
    return m->raw(arg, size);
}

/**
 * Wrapper for module's send_packet() interface.
 *
 * @param m [in] Pointer to a module instance.
 * @param prefix [in] A name used to identify the sender.
 * @param channelId [in] Channel ID, used to determine audio or video data.
 * @param pkt [in] The packet data to be sent.
 * @param encoderPts [out] Presentation time stamp, store as a 64-bit sequence number.
 * @param ptv [out] Presentation time stamp, stored as a \a timeval structure.
 *
 * This function is only used by a server module.
 *
 * We recommend to use this function instead of calling \em m->send_packet() directly.
 * This function ensures that a module has defined the \em send_packet interface
 * before calling the interface.
 */
int
ga_module_send_packet(ga_module_t *m, const char *prefix, int channelId, ga_packet_t *pkt, int64_t encoderPts, struct timeval *ptv) {
#if 0    /* not checked: for performance consideration */
    if(m == NULL)
        return GA_IOCTL_ERR_NULLMODULE;
    if(m->send_packet == NULL)
        return GA_IOCTL_ERR_NOINTERFACE;
#endif
    return m->send_packet(prefix, channelId, pkt, encoderPts, ptv);
}

// The following functions are to replacing libavcodec functions. Implementation refer to libavcodec.

/**
 * Initialize optional fields of a packet with default values.
 *
 * @param pkt [in,out] The packet data to be initialize.
 *
 * This function is used to replace libavcodec av_init_packet.
 */
void
ga_init_packet(ga_packet_t* pkt) {
    if (pkt == nullptr) {
        ga_logger(Severity::ERR, "ga_init_packet: Invalid argument.\n");
        return;
    }

    pkt->buf        = nullptr;
    pkt->pts        = GA_NOPTS_VALUE;
    pkt->dts        = GA_NOPTS_VALUE;
    pkt->flags      = 0;
    pkt->stream_index = 0;
    pkt->side_data  = nullptr;
    pkt->side_data_elems = 0;
    pkt->duration   = 0;
    pkt->pos        = -1;
    pkt->convergence_duration = 0;
}

/**
 * Allocate new side information of a packet.
 *
 * @param pkt  [in,out] The packet contains side data.
 * @param type [in]     Side informaiton type.
 * @param size [in]     Side information size.
 *
 * This function is used to replace libavcodec av_packet_new_side_data.
 */
uint8_t*
ga_packet_new_side_data(ga_packet_t* pkt, ga_packet_side_data_type type, int size) {
    if (pkt == nullptr) {
        ga_logger(Severity::ERR, "ga_packet_new_side_data: Invalid argument.\n");
        return nullptr;
    }

    int elems = pkt->side_data_elems;

    if ((unsigned)size > INT_MAX - GA_INPUT_BUFFER_PADDING_SIZE) {
        ga_logger(Severity::ERR, "ga_packet_new_side_data: size over range (%d).\n", size);
        return nullptr;
    }

    ga_packet_side_data_t* more_side_data = (ga_packet_side_data_t*)realloc(pkt->side_data, (static_cast<size_t>(elems) + 1) * sizeof(*pkt->side_data));
    if (more_side_data == nullptr) {
        ga_logger(Severity::ERR, "ga_packet_new_side_data: Failed to allocate more side data.\n");
        return nullptr;
    }
    pkt->side_data = more_side_data;

    pkt->side_data[elems].data = (uint8_t*)malloc(static_cast<size_t>(size) + GA_INPUT_BUFFER_PADDING_SIZE);
    if (pkt->side_data[elems].data == nullptr) {
        ga_logger(Severity::ERR, "ga_packet_new_side_data: Failed to allocate new side data.\n");
        return nullptr;
    }
    pkt->side_data[elems].size = size;
    pkt->side_data[elems].type = type;
    pkt->side_data_elems++;

    return pkt->side_data[elems].data;
}

/**
 * Convenience function to free all the side data stored.
 * All the other fields stay untouched.
 *
 * @param pkt  [in,out] Packet's side data to be free.
 *
 * This function is used to replace libavcodec av_packet_free_side_data.
 */
void
ga_packet_free_side_data(ga_packet_t* pkt) {
    if (pkt == nullptr || pkt->side_data == nullptr) {
        return;
    }

    for (int i = 0; i < pkt->side_data_elems; ++i) {
        free(pkt->side_data[i].data);
    }
    free(pkt->side_data);
}

/**
 * Get side information from packet.
 *
 * @param pkt  [in,out] The packet contains side data.
 * @param type [in]     Desired side information type.
 * @param size [in]     Pointer for side information size to store (optional).
 *
 * This function is to replace libavcodec av_packet_get_side_data.
 *
 * @return pointer to data if present or nullptr otherwise
 */
uint8_t*
ga_packet_get_side_data(ga_packet_t* pkt, ga_packet_side_data_type type, int* size) {
    if (pkt == nullptr) {
        ga_logger(Severity::ERR, "ga_packet_get_side_data: Invalid argument.\n");
        return nullptr;
    }

    if (pkt->side_data_elems > 0 && pkt->side_data != nullptr) {
        for (int i = 0; i < pkt->side_data_elems; ++i) {
            if (pkt->side_data[i].type == type) {
                if (size)
                    *size = pkt->side_data[i].size;
                return pkt->side_data[i].data;
            }
        }
    }

    if (size)
        *size = 0;

    return nullptr;
}
