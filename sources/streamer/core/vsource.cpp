// Copyright (C) 2022 Intel Corporation

/*
 * Copyright (c) 2013-2014 Chun-Ying Huang
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
 * Define video sources: the implementation
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <map>
#ifndef WIN32
#include <arpa/inet.h>
#endif

#include "vsource.h"
#include "ga-common.h"
#include "ga-conf.h"
#include "ga-crc.h"

/**< Video buffer allocation alignment: should be 2^n */
#define    VSOURCE_ALIGNMENT    16
/**< Video buffer allocation alignment mask: should be \em VSOURCE_ALIGNMENT-1 */
#define    VSOURCE_ALIGNMENT_MASK    0x0f

// embed colorcode feature
#define    COLORCODE_MAX_DIGIT    10    /**< Maximum number of embedded color code digits */
#define    COLORCODE_MAX_WIDTH    128    /**< Maximum Width of each embedded color code digit */
#define    COLORCODE_DEF_DIGIT    5    /**< Number of embedded color code sequence digits */
#define    COLORCODE_DEF_WIDTH    80    /**< Width of each embedded color code digit */
#define    COLORCODE_DEF_HEIGHT    80    /**< Height of embedded color code */
#define    COLORCODE_CRC        2    /**< Colorcode: current 2 octals for CRC */
#define    COLORCODE_ID        2    /**< Colorcode: current 2 octals for ID */
#define    COLORCODE_SUFFIX    (COLORCODE_CRC + COLORCODE_ID)    /**< Digits
                      * appended to the embedded color code sequence */

// golbal image structure
static int gChannels;        /**< Total number of video channels */
static vsource_t gVsource[VIDEO_SOURCE_CHANNEL_MAX];    /**< Video source */
static dpipe_t *gPipe[VIDEO_SOURCE_CHANNEL_MAX];    /**< Video pipeline */

/**
 * Initialize a video frame
 *
 * @param channel [in] The channel Id of the video frame.
 * @param frame [in] Pointer to an allocated video frame structure.
 * @return Return the same pointer as \a frame, unless an incorrect configuration is given.
 *
 * Note that video frame data is stored right after a video frame structure.
 * So the size of allocated video frame structure must be at least:
 * \em sizeof(vsource_frame_t)
 * + \em video-source-max-stride * \em video-source-max-height
 * + \em VSOURCE_ALIGNMENT.
 *
 * \a imgbufsize will be set to \em video-source-max-stride * \em video-source-max-height,
 * and \a imgbuf is pointed to an aligned memory address.
 */
vsource_frame_t *
vsource_frame_init(int channel, vsource_frame_t *frame) {
    int i;
    vsource_t *vs;
    //
    if(channel < 0 || channel >= VIDEO_SOURCE_CHANNEL_MAX)
        return NULL;
    vs = &gVsource[channel];
    // has not been initialized?
    if(vs->max_width == 0)
        return NULL;
    //
    bzero(frame, sizeof(vsource_frame_t));
    //
    for(i = 0; i < VIDEO_SOURCE_MAX_STRIDE; i++) {
        frame->linesize[i] = vs->max_stride;
    }
    frame->maxstride = vs->max_stride;
    frame->imgbufsize = vs->max_height * vs->max_stride;
    frame->imgbuf = ((unsigned char *) frame) + sizeof(vsource_frame_t);
    frame->imgbuf += ga_alignment(frame->imgbuf, VSOURCE_ALIGNMENT);
    //ga_logger(Severity::INFO, "XXX: frame=%p, imgbuf=%p, sizeof(vframe)=%d, bzero(%d)\n",
    //    frame, frame->imgbuf, sizeof(vsource_frame_t), frame->imgbufsize);
    bzero(frame->imgbuf, frame->imgbufsize);
    return frame;
}

/**
 * Release a video frame data structure.
 *
 * Currently it does nothing because memory allocation is done in pipeline.
 */
void
vsource_frame_release(vsource_frame_t *frame) {
    return;
}

/**
 * Duplicate a video frame.
 *
 * @param src [in] Pointer to a source video frame.
 * @param dst [in] Pointer to an initialized destination video frame.
 */
void
vsource_dup_frame(vsource_frame_t *src, vsource_frame_t *dst) {
    int j;
    dst->imgpts = src->imgpts;
#if defined(_WIN32)
    dst->pixelformat = src->pixelformat;
#endif
    for(j = 0; j < VIDEO_SOURCE_MAX_STRIDE; j++) {
        dst->linesize[j] = src->linesize[j];
    }
    dst->realwidth = src->realwidth;
    dst->realheight = src->realheight;
    dst->realstride = src->realstride;
    dst->realsize = src->realsize;
    bcopy(src->imgbuf, dst->imgbuf, src->realstride * src->realheight/*dst->imgbufsize*/);
    return;
}

/**
 * Get the number of channels of the video source.
 *
 * @return The total number of channels.
 */
int
video_source_channels() {
    return gChannels;
}

/**
 * Get the video source setup of a given channel.
 *
 * @param channel [in] The channel Id.
 * @return Pointer to the obtained video source info data structure.
 */
vsource_t *
video_source(int channel) {
    if(channel < 0 || channel > gChannels) {
        return NULL;
    }
    return &gVsource[channel];
}

/**
 * Add a pipename to a video source. This is an internal function.
 *
 * @param vs [in] The video source data structure.
 * @param pipename [in] The pipeline name to be added.
 * @return Pointer to the added pipeline name.
 */
static const char *
video_source_add_pipename_internal(vsource_t *vs, const char *pipename) {
    pipename_t *p;
    if(vs == NULL || pipename == NULL)
        return NULL;
    if((p = (pipename_t *) malloc(sizeof(pipename_t) + strlen(pipename) + 1)) == NULL)
        return NULL;
    p->next = vs->pipename;
    bcopy(pipename, p->name, strlen(pipename)+1);
    vs->pipename = p;
    return p->name;
}

/**
 * Add a pipeline name to a video source data structure.
 *
 * @param channel [in] The video source channel id.
 * @param pipename [in] The pipeline name to be added.
 * @return Pointer to the added pipeline name.
 */
const char *
video_source_add_pipename(int channel, const char *pipename) {
    vsource_t *vs = video_source(channel);
    if(vs == NULL)
        return NULL;
    return video_source_add_pipename_internal(vs, pipename);
}

/**
 * Get the pipeline name of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return Pointer to the pipeline name.
 */
const char *
video_source_get_pipename(int channel) {
    vsource_t *vs = video_source(channel);
    if(vs == NULL)
        return NULL;
    if(vs->pipename == NULL)
        return NULL;
    return vs->pipename->name;
}

/**
 * Get the maximum width of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The maximum width of the video source, or -1 on error.
 */
int
video_source_max_width(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->max_width;
}

/**
 * Get the maximum height of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The maximum height of the video source, or -1 on error.
 */
int
video_source_max_height(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->max_height;
}

/**
 * Get the maximum stride of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The maximum stride of the video source, or -1 on error.
 */
int
video_source_max_stride(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->max_stride;
}

/**
 * Get the current width of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The current width of the video source, or -1 on error.
 */
int
video_source_curr_width(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->curr_width;
}

/**
 * Get the current height of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The current height of the video source, or -1 on error.
 */
int
video_source_curr_height(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->curr_height;
}

/**
 * Get the current stride of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The current stride of the video source, or -1 on error.
 */
int
video_source_curr_stride(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->curr_stride;
}

/**
 * Get the output width of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The output width of the video source, or -1 on error.
 */
int
video_source_out_width(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->out_width;
}

/**
 * Get the output height of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The output height of the video source, or -1 on error.
 */
int
video_source_out_height(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->out_height;
}

/**
 * Get the output stride of a video source.
 *
 * @param channel [in] The channel id of the video source.
 * @return The output stride of the video source, or -1 on error.
 */
int
video_source_out_stride(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? -1 : vs->out_stride;
}

/**
  * Return the maximum memory size to store a frame (including size for alignment)
  *
  * @param channel [in] The channel id
  * @return The size in bytes, or 0 if the given \a channel is not initialized
  */
int
video_source_mem_size(int channel) {
    vsource_t *vs = video_source(channel);
    return vs == NULL ? 0 : (vs->max_height * vs->max_stride + VSOURCE_ALIGNMENT);
}

/** Return the larger value of \a x and \a y */
#ifndef max
#define    max(x, y)    ((x) > (y) ? (x) : (y))
#endif

/**
 * The generic function to setup video sources.
 *
 * @param config [in] Pointer to an array of video configurations.
 * @param nConfig [in] Number of configurations in the array.
 * @return 0 on success, or -1 on error.
 *
 * - The configurations have to provide current video width and height
 *   of each video channel.
 * - The maximum resolution is read from \em max-resolution parameter, and
 *   the output resolution is read from \em output-resolution parameter in
 *   the configuration file.
 * - The pipeline name is automatically generated based on the index of
 *   each video configuration.
 * - The corresponding video pipeline is created as well.
 */
int
video_source_setup_ex(vsource_config_t *config, int nConfig) {
    int idx;
    int maxres[2] = { 0, 0 };
    int outres[2] = { 0, 0 };
    //
    if(config==NULL || nConfig <=0 || nConfig > VIDEO_SOURCE_CHANNEL_MAX) {
        ga_logger(Severity::ERR, "video source: invalid video source configuration request=%d; MAX=%d; config=%p\n",
            nConfig, VIDEO_SOURCE_CHANNEL_MAX, config);
        return -1;
    }
    //
    if(ga_conf_readints("max-resolution", maxres, 2) != 2) {
        maxres[0] = maxres[1] = 0;
    }
    if(ga_conf_readints("output-resolution", outres, 2) != 2) {
        outres[0] = outres[1] = 0;
    }
    //
    for(idx = 0; idx < nConfig; idx++) {
        vsource_t *vs = &gVsource[idx];
        dpipe_buffer_t *data = NULL;
        char pipename[64];
        //
        bzero(vs, sizeof(vsource_t));
        snprintf(pipename, sizeof(pipename), VIDEO_SOURCE_PIPEFORMAT, idx);
        vs->channel     = idx;
        if(video_source_add_pipename_internal(vs, pipename) == NULL) {
            ga_logger(Severity::ERR, "video source: setup pipename failed (%s).\n", pipename);
            return -1;
        }
        vs->max_width   = max(VIDEO_SOURCE_DEF_MAXWIDTH, maxres[0]);
        vs->max_height  = max(VIDEO_SOURCE_DEF_MAXHEIGHT, maxres[1]);
        vs->max_stride  = max(VIDEO_SOURCE_DEF_MAXWIDTH, maxres[0]) * 4;
        vs->curr_width  = config[idx].curr_width;
        vs->curr_height = config[idx].curr_height;
        vs->curr_stride = config[idx].curr_stride;
        if(outres[0] != 0) {
            vs->out_width   = outres[0];
            vs->out_height  = outres[1];
            vs->out_stride  = outres[0] * 4;
        } else {
            vs->out_width   = vs->curr_width;
            vs->out_height  = vs->curr_height;
            vs->out_stride  = vs->curr_stride;
        }
        // create pipe
        gPipe[idx] = dpipe_create(idx, pipename, VIDEO_SOURCE_POOLSIZE,
                sizeof(vsource_frame_t) + vs->max_height * vs->max_stride + VSOURCE_ALIGNMENT);
        if(gPipe[idx] == NULL) {
            ga_logger(Severity::ERR, "video source: init pipeline failed.\n");
            return -1;
        }
        for(data = gPipe[idx]->in; data != NULL; data = data->next) {
            if(vsource_frame_init(idx, (vsource_frame_t*) data->pointer) == NULL) {
                ga_logger(Severity::ERR, "video source: init faile failed.\n");
                return -1;
            }
        }
        //
        ga_logger(Severity::INFO, "video-source: %s initialized max-curr-out = (%dx%d)-(%dx%d)-(%dx%d)\n",
            pipename, vs->max_width, vs->max_height,
            vs->curr_width, vs->curr_height, vs->out_width, vs->out_height);
    }
    //
    gChannels = idx;
    //
    return 0;
}

/**
 * Setup up a one-channel only video source
 *
 * @param curr_width [in] Current video source width.
 * @param curr_height [in] Current video source height.
 * @param curr_stride [in] Current video source stride.
 * @return 0 on success, or -1 on error.
 *
 * This function calls the \em video_source_setup_ex function.
 */
int
video_source_setup(int curr_width, int curr_height, int curr_stride) {
    vsource_config_t c;
    bzero(&c, sizeof(c));
    //config.rtp_id = channel_id;
    c.curr_width = curr_width;
    c.curr_height = curr_height;
    c.curr_stride = curr_stride;
    //
    return video_source_setup_ex(&c, 1);
}
