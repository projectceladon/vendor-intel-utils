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

#include "CFFFilter.h"
#include "CVAAPIDevice.h"
#include "CQSVAPIDevice.h"
#include <string>

using namespace std;

static inline std::string make_videosize(int w, int h)
{
    return "video_size=" + std::to_string(w) + "x" + std::to_string(h);
}

static inline std::string make_pixfmt(AVPixelFormat fmt)
{
    return "pix_fmt=" + std::to_string(fmt);
}

static inline std::string make_ratio(int n, int d)
{
    return std::to_string(n) + "/" + std::to_string(d);
}

static inline std::string make_timebase(int n, int d)
{
    return "time_base=" + make_ratio(n, d);
}

static inline std::string make_sar(int n, int d)
{
    return "sar=" + make_ratio(n, d);
}

static inline std::string make_framerate(int n, int d)
{
    return "frame_rate=" + make_ratio(n, d);
}

static inline std::string make_samplerate(int f)
{
    return "sample_rate=" + std::to_string(f);
}

static inline std::string make_samplefmt(const char* fmt)
{
    return "sample_fmt=" + std::string(fmt);
}

static inline std::string make_channels(int ch)
{
    return "channels=" + std::to_string(ch);
}

static inline std::string make_chlayout(int ch_layout)
{
    return "channel_layout=" + std::to_string(ch_layout);
}

static inline bool is_pixvaapi(AVPixelFormat fmt)
{
    return (fmt == AV_PIX_FMT_VAAPI);
}

static inline bool is_pixvaapi(int fmt)
{
    return is_pixvaapi((AVPixelFormat)fmt);
}

static inline bool is_pixqsv(AVPixelFormat fmt)
{
    return (fmt == AV_PIX_FMT_QSV);
}

static inline bool is_pixqsv(int fmt)
{
    return is_pixqsv((AVPixelFormat)fmt);
}

CFFFilter::CFFFilter(CStreamInfo *in, CStreamInfo *out, bool bVASurface, bool bQSVSurface, bool isVaapiPlugin, bool isQSVPlugin, const int filter_nbthreads) :
    CTransLog(__func__),
    m_vaapiPlugin(isVaapiPlugin),
    m_qsvPlugin(isQSVPlugin)
{
#ifndef ENABLE_MEMSHARE
    AVFilterContext *pScale = nullptr;
    AVFilterContext *pHwupload = nullptr;
    AVBufferRef *pHwDev;

    if (!m_vaapiPlugin && !m_qsvPlugin) {
         Error("Invalid plugin");
         return;
    }
#endif

    m_pGraph   = avfilter_graph_alloc();
    m_nFrames  = 0;
    m_bInited  = false;
    m_SrcInfo  = *in;
    m_SinkInfo = *out;
    m_pSrc     = m_pSink = nullptr;
    m_bVASurface = bVASurface;
    m_bQSVSurface = bQSVSurface;
    m_lastError = 0;

    // set filter threads number
    if (m_pGraph) {
        m_pGraph->nb_threads = filter_nbthreads > 0 ? filter_nbthreads : DEFAULT_FILTER_NBTHREADS;
        Info("%s : %d  set filter threads to %d\n", __func__, __LINE__, m_pGraph->nb_threads);
    }

    std::string params;

    if (m_SrcInfo.m_pCodecPars->codec_type == AVMEDIA_TYPE_VIDEO) {
        AVPixelFormat fmt;

        if (m_bVASurface) fmt = (AVPixelFormat)AV_PIX_FMT_VAAPI;
        else if (m_bQSVSurface) fmt = (AVPixelFormat)AV_PIX_FMT_QSV;
        else fmt = (AVPixelFormat)m_SrcInfo.m_pCodecPars->format;

        // We compose the following parameters string:
        // "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:sar=%d/%d:frame_rate=%d/%d"
        params = make_videosize(m_SrcInfo.m_pCodecPars->width, m_SrcInfo.m_pCodecPars->height);
        params += ":" + make_pixfmt(fmt);
        params += ":" + make_timebase(1, AV_TIME_BASE);
        params += ":" + make_sar(
          m_SrcInfo.m_pCodecPars->sample_aspect_ratio.num,
          m_SrcInfo.m_pCodecPars->sample_aspect_ratio.den);

        if (m_SrcInfo.m_rFrameRate.den > 0 && m_SrcInfo.m_rFrameRate.num > 0)
            params += ":" + make_framerate(m_SrcInfo.m_rFrameRate.num, m_SrcInfo.m_rFrameRate.den);

        Info("src params: %s\n", params.c_str());
        m_pSrc  = alloc_filter("buffer", params.c_str());
        if (!m_pSrc) {
            Error("Fail to create filter %s.\n", "buffer");
            return;
        }

        Info("sink params: n/a\n");
        m_pSink = alloc_filter("buffersink", NULL);
        if (!m_pSink) {
            Error("Fail to create filter %s.\n", "buffersink");
            return;
        }

#ifndef ENABLE_MEMSHARE
        // We compose the following parameters string:
        // "format=nv12:mode=default:w=%d:h=%d"
        params = "format=nv12:mode=default";
        params += ":w=" + std::to_string(in->m_pCodecPars->width) + ":h=" + std::to_string(in->m_pCodecPars->height);

        const char* scale_plugin = (m_vaapiPlugin)? "scale_vaapi": "scale_qsv";

        Info("%s params: %s\n", scale_plugin, params.c_str());
        pScale = alloc_filter(scale_plugin, params.c_str());
        if (!pScale) {
            Error("Fail to create filter %s.\n", scale_plugin);
            return;
        }

        ///< Check if Converting to Video-Memory is needed
        if ((is_pixvaapi(m_SinkInfo.m_pCodecPars->format) && !m_bVASurface) ||
            (is_pixqsv(m_SinkInfo.m_pCodecPars->format) && !m_bQSVSurface)) {
            Info("hwupload params: n/a\n");
            pHwupload = alloc_filter("hwupload", NULL);
            if (!pHwupload) {
                Error("Fail to create filter %s.\n", "hwupload");
                return;
            }
            if (is_pixqsv(m_SinkInfo.m_pCodecPars->format))
                pHwupload->extra_hw_frames = 64;
        }

        if (is_pixvaapi(m_SinkInfo.m_pCodecPars->format))
            pHwDev = CVAAPIDevice::getInstance()->getVaapiDev();
        else if (is_pixqsv(m_SinkInfo.m_pCodecPars->format))
            pHwDev = CQSVAPIDevice::getInstance()->getQSVapiDev();

        for (unsigned idx = 0; idx < m_pGraph->nb_filters; idx++)
            m_pGraph->filters[idx]->hw_device_ctx = av_buffer_ref(pHwDev);

        if (pHwupload) {
            Info("filter pipeline: src->hwupload->scale->sink\n");
            avfilter_link(m_pSrc, 0, pHwupload, 0);
            avfilter_link(pHwupload, 0, pScale, 0);
            avfilter_link(pScale, 0, m_pSink, 0);
        }
        else {
            Info("filter pipeline: src->scale->sink\n");
            avfilter_link(m_pSrc, 0, pScale, 0);
            avfilter_link(pScale, 0, m_pSink, 0);
        }
        Info("disable MEMSHARE mode\n");
#else
        avfilter_link(m_pSrc, 0, m_pSink, 0);
        Info("enable MEMSHARE mode\n");
#endif
    } else if (m_SrcInfo.m_pCodecPars->codec_type == AVMEDIA_TYPE_AUDIO) {
        params = make_timebase(1, m_SrcInfo.m_pCodecPars->sample_rate);
        params += ":" + make_samplerate(m_SrcInfo.m_pCodecPars->sample_rate);
        params += ":" + make_samplefmt(av_get_sample_fmt_name((AVSampleFormat)m_SrcInfo.m_pCodecPars->format));
        params += ":" + make_channels(m_SrcInfo.m_pCodecPars->channels);
        params += ":" + make_chlayout(m_SrcInfo.m_pCodecPars->channels > 1 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO);

        m_pSrc  = alloc_filter("abuffer", params.c_str());
        if (!m_pSrc) {
            Error("Fail to create filter %s.\n", "abuffer");
            return;
        }

        char buffer[1024] = "\0";
        snprintf(buffer, sizeof(buffer), "sample_fmts=%02x%02x%02x%02x:sample_rates=%02x%02x%02x%02x",
                 m_SinkInfo.m_pCodecPars->format & 0xFF, (m_SinkInfo.m_pCodecPars->format >> 8)&0xFF, 0, 0,
                 m_SinkInfo.m_pCodecPars->sample_rate & 0xFF, (m_SinkInfo.m_pCodecPars->sample_rate >> 8)&0xFF,
                 (m_SinkInfo.m_pCodecPars->sample_rate >> 16)&0xFF, (m_SinkInfo.m_pCodecPars->sample_rate >> 24)&0xFF);
        m_pSink = alloc_filter("abuffersink", buffer);
        if (!m_pSink) {
            Error("Fail to create filter %s.\n", "abuffersink");
            return;
        }

        avfilter_link(m_pSrc, 0, m_pSink, 0);
    }
}

CFFFilter::~CFFFilter() {
    avfilter_graph_free(&m_pGraph);
    Info("Total filtered frames: %d\n", m_nFrames);
}

AVFilterContext *CFFFilter::alloc_filter(const char* name, const char* par) {
    AVFilterContext *pFilter;
    int ret = avfilter_graph_create_filter(&pFilter, avfilter_get_by_name(name), name,
                                           par, this, m_pGraph);
    if (ret < 0) {
        Error("Fail to create %s. Msg: %s\n", name, ErrToStr(ret).c_str());
        return nullptr;
    }
    return pFilter;
}

int CFFFilter::push(AVFrame* frame) {
    int ret = 0;

    if (!m_bInited) {
        AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();

        if (!par) {
            Error("failed to allocated AVBufferSrcParameters\n");
            return -1;
        }

        par->format              = frame->format;
        par->width               = frame->width;
        par->height              = frame->height;
        par->sample_aspect_ratio = frame->sample_aspect_ratio;
        par->sample_rate         = frame->sample_rate;
        par->channel_layout      = frame->channel_layout;
        if (frame->hw_frames_ctx)
            par->hw_frames_ctx   = frame->hw_frames_ctx;
        ret = av_buffersrc_parameters_set(m_pSrc, par);
        av_freep(&par);
        if (ret < 0) {
            Error("Fail to reset parameters. Error '%s'\n", ErrToStr(ret).c_str());
            return ret;
        }

        ret = avfilter_graph_config(m_pGraph, nullptr);
        if (ret < 0) {
            Error("Fail to config filters. Error '%s'\n", ErrToStr(ret).c_str());
            return ret;
        }

        /*
         * FIXME: Rescale the frame's PTS as the filter's time_base may be
         * changed after initialization
         */
        frame->pts = av_rescale_q(frame->pts, m_SrcInfo.m_rTimeBase,
                                  m_pSrc->outputs[0]->time_base);

        /* Update stream info after configuring. */
        m_SrcInfo.m_rTimeBase                        = m_pSrc->outputs[0]->time_base;
        m_SinkInfo.m_rFrameRate                      = av_buffersink_get_frame_rate(m_pSink);
        m_SinkInfo.m_rTimeBase                       = av_buffersink_get_time_base(m_pSink);
#ifndef ENABLE_MEMSHARE
        m_SinkInfo.m_pCodecPars->format              = av_buffersink_get_format(m_pSink);
#else
        if (m_SinkInfo.m_pCodecPars->format == AV_PIX_FMT_QSV ||
                m_SinkInfo.m_pCodecPars->format == AV_PIX_FMT_VAAPI) {
            // do nothing
        } else {
            m_SinkInfo.m_pCodecPars->format = av_buffersink_get_format(m_pSink);
        }
#endif
        m_SinkInfo.m_pCodecPars->width               = av_buffersink_get_w(m_pSink);
        m_SinkInfo.m_pCodecPars->height              = av_buffersink_get_h(m_pSink);
        m_SinkInfo.m_pCodecPars->sample_aspect_ratio = av_buffersink_get_sample_aspect_ratio(m_pSink);
        m_SinkInfo.m_pCodecPars->channels            = av_buffersink_get_channels(m_pSink);
        m_SinkInfo.m_pCodecPars->sample_rate         = av_buffersink_get_sample_rate(m_pSink);
        m_SinkInfo.m_pCodecPars->channel_layout      = av_buffersink_get_channel_layout(m_pSink);
        m_bInited = true;
    }

    Verbose("Filter <- Decoder: dts %s, time %s\n", TsToStr(frame->pts).c_str(),
            TsToTimeStr(frame->pts, m_pSrc->outputs[0]->time_base.num,
                     m_pSrc->outputs[0]->time_base.den).c_str());

    return av_buffersrc_add_frame(m_pSrc, frame);
}

AVFrame *CFFFilter::pop(void) {
    if (!m_bInited) {
        return nullptr;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        Error("failed to allocate AVFrame\n");
        return nullptr;
    }

    int ret = av_buffersink_get_frame(m_pSink, frame);
    if (ret < 0) {
        if (ret == AVERROR(EIO)) {
            m_lastError = ret;
        }
        av_frame_free(&frame);
        return nullptr;
    }

    m_nFrames ++;

    Verbose("Filter -> Encoder: dts %s, time %s\n", TsToStr(frame->pts).c_str(),
            TsToTimeStr(frame->pts, m_pSink->inputs[0]->time_base.num,
                     m_pSink->inputs[0]->time_base.den).c_str());

#ifdef ENABLE_MEMSHARE
    if (m_SinkInfo.m_pCodecPars->format == AV_PIX_FMT_QSV ||
            m_SinkInfo.m_pCodecPars->format == AV_PIX_FMT_VAAPI) {
        frame->format = m_SinkInfo.m_pCodecPars->format;
    }
#endif

    return frame;
}

int CFFFilter::getNumFrames() {
    return m_nFrames;
}

CStreamInfo* CFFFilter::getSinkInfo() {
    return &m_SinkInfo;
}

CStreamInfo* CFFFilter::getSrcInfo() {
    return &m_SrcInfo;
}

void CFFFilter::setVASurfaceFlag(bool bVASurface) {
    m_bVASurface = bVASurface;
}

bool CFFFilter::getVASurfaceFlag() {
    return m_bVASurface;
}

void CFFFilter::setQSVSurfaceFlag(bool bQSVSurface) {
    m_bQSVSurface = bQSVSurface;
}

bool CFFFilter::getQSVSurfaceFlag() {
    return m_bQSVSurface;
}

int CFFFilter::getLastError() {
    return m_lastError;
}

void CFFFilter::setLastError(int iErrorCode) {
    m_lastError = iErrorCode;
}

void CFFFilter::updateDynamicChangedFramerate(int framerate) {
    m_SrcInfo.m_rFrameRate = (AVRational){framerate, 1};
    m_SinkInfo.m_rFrameRate = (AVRational){framerate, 1};
}
