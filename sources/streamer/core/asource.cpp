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
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <chrono>
#include <map>
#include <mutex>

#include "asource.h"

#include "ga-common.h"

using namespace std;
using namespace std::chrono;

static std::mutex ccmutex;
static map<long,audio_buffer_t*> gClients;
//
static int gChunksize = 0;
static int gSamplerate = 0;
static int gBitspersample = 0;
static int gChannels = 0;

audio_buffer_t *
audio_source_buffer_init() {
    // XXX:    frames, channels, and bitspersample should be the same as the
    //    configuration -- since these are provided by encoders (clients)
    audio_buffer_t *ab;
    int frames = gChunksize*4;
    int channels = gChannels;
    int bitspersample = gBitspersample;
    if(frames == 0
    || channels == 0
    || bitspersample == 0) {
        ga_logger(Severity::ERR, "audio source: invalid argument (frames=%d, channels=%d, bitspersample=%d)\n",
            frames, channels, bitspersample);
        return NULL;
    }
    ab = new audio_buffer_t{};
    if (ab == NULL) {
        return NULL;
    }
    ab->client_connected = false;
    ab->frames    = frames;
    ab->channels = channels;
    ab->bitspersample = bitspersample;
    ab->bufsize = frames * channels * bitspersample / 8;
    if((ab->buffer = (unsigned char*) malloc(ab->bufsize)) == NULL) {
        delete ab;
        return NULL;
    }
    return ab;
}

void
audio_source_buffer_deinit(audio_buffer_t *ab) {
    if(ab == NULL)
        return;
    if(ab->buffer != NULL)
        free(ab->buffer);
    delete ab;
    return;
}

void
audio_source_buffer_fill_one(audio_buffer_t *ab, const unsigned char *data, int frames) {
    int headspace, tailspace;
    int framesize;
    if(ab == NULL)
        return;
    if(frames <= 0)
        return;
    framesize = frames * ab->channels * ab->bitspersample / 8;
    std::lock_guard<std::mutex> lock(ab->bufmutex);
retry:
    headspace = ab->bufhead;
    tailspace = ab->bufsize - ab->buftail;
    if(framesize > headspace + tailspace) {
        ab->bufcond.notify_one();
        return;
    }
    // we GUARANTEE that bufhead <= buftail
    if(framesize <= tailspace) {
        // case #1: tailspace is sufficient, append at the end
        if(data == NULL) {
            bzero(&ab->buffer[ab->buftail], framesize);
        } else {
            bcopy(data, &ab->buffer[ab->buftail], framesize);
        }
        ab->buftail += framesize;
        ab->bframes += frames;
        ab->bufcond.notify_one();
        return;
    }
    // case #2: framesize > tailspace, but overall space is sufficient
    bcopy(&ab->buffer[ab->bufhead], ab->buffer, ab->buftail - ab->bufhead);
    ab->buftail -= ab->bufhead;
    ab->bufhead = 0;
    goto retry;
    // never reach here
    return;
}

void
audio_source_buffer_fill(const unsigned char *data, int frames) {
    map<long,audio_buffer_t*>::iterator mi;
    std::lock_guard<std::mutex> lock(ccmutex);
    for(mi = gClients.begin(); mi != gClients.end(); mi++) {
        if(mi->second != NULL) {
            if (mi->second->client_connected == true) {
                audio_source_buffer_fill_one(mi->second, data, frames);
            }
        }
    }
}

static nanoseconds timespec2dur(const timespec& ts)
{
       auto dur = seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec};
       return duration_cast<nanoseconds>(dur);
}

static time_point<system_clock, nanoseconds> timespec2tp(const timespec& ts)
{
       return time_point<system_clock, nanoseconds>{
               duration_cast<system_clock::duration>(timespec2dur(ts)) };
}

int
audio_source_buffer_read(audio_buffer_t *ab, unsigned char *buf, int frames) {
    int copyframe = 0, copysize = 0;
    struct timeval tv;
    struct timespec to;
    //
    if(frames <= 0) {
        return 0;
    }
    //
    std::unique_lock<std::mutex> lock(ab->bufmutex);
    //
    if(ab->bframes == 0) {
        gettimeofday(&tv, NULL);
        to.tv_sec = tv.tv_sec+1;
        to.tv_nsec = tv.tv_usec * 1000;
        ab->bufcond.wait_until(lock, timespec2tp(to));
    }
    if(ab->bframes >= frames) {
        copyframe = frames;
    } else {
        copyframe = ab->bframes;
    }
    if(copyframe > 0) {
        copysize = copyframe * ab->channels * ab->bitspersample / 8;
        //
        bcopy(&ab->buffer[ab->bufhead], buf, copysize);
        //
        ab->bufhead += copysize;
        ab->bframes -= copyframe;
        ab->bufPts += copyframe;
        if(ab->bframes == 0) {
            ab->bufhead = ab->buftail = 0;
        }
    }
    //
    return copyframe;
}

void
audio_source_buffer_purge(audio_buffer_t *ab) {
    ga_logger(Severity::INFO, "audio: buffer purged (%d bytes / %d frames).\n",
        ab->buftail - ab->bufhead, ab->bframes);
    std::lock_guard<std::mutex> lock(ab->bufmutex);
    ab->bufPts = 0LL;
    ab->bufhead = ab->buftail = ab->bframes = 0;
    return;
}

void
audio_source_client_register(long tid, audio_buffer_t *ab) {
    std::lock_guard<std::mutex> lock(ccmutex);
    gClients[tid] = ab;
}

void
audio_source_client_unregister(long tid) {
    std::lock_guard<std::mutex> lock(ccmutex);
    gClients.erase(tid);
}

int
audio_source_client_count() {
    size_t n;
    std::lock_guard<std::mutex> lock(ccmutex);
    n = gClients.size();
    return static_cast<int>(n);
}

int
audio_source_chunksize() {
    return gChunksize;
}

int
audio_source_chunkbytes() {
    return gChunksize * gChannels * gBitspersample / 8;
}

int
audio_source_samplerate() {
    return gSamplerate;
}

int
audio_source_bitspersample() {
    return gBitspersample;
}

int
audio_source_channels() {
    return gChannels;
}

int
audio_source_setup(int chunksize, int samplerate, int bitspersample, int channels) {
    gChunksize = chunksize;
    gSamplerate = samplerate;
    gBitspersample = bitspersample;
    gChannels = channels;
    return 0;
}

