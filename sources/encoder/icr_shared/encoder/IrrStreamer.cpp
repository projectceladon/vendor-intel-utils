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

#include "stream.h"
#include "IrrStreamer.h"

#ifdef ENABLE_MEMSHARE
#include "core/CFFEncoder.h"
#include "core/CEncoder.h"
#endif

using namespace std;

static std::unique_ptr<IrrStreamer> pStreamer;

IrrStreamer* IrrStreamer::get()
{
    return pStreamer.get();
}

void IrrStreamer::Register(int id, int w, int h, float framerate)
{
    pStreamer.reset(new IrrStreamer(id, w, h, framerate));
}

void IrrStreamer::Unregister()
{
    pStreamer = nullptr;
}

IrrStreamer::IrrStreamer(int id, int w, int h, float framerate) : CTransLog(__func__){
    m_nPixfmt    = AV_PIX_FMT_RGBA;
    m_nMaxPkts   = 5;
    m_nCurPkts   = 0;
    m_pTrans     = nullptr;
    m_pDemux     = nullptr;
    m_pWriter    = nullptr;
    m_pPool      = nullptr;
    m_nWidth     = w;
    m_nHeight    = h;
    m_fFramerate = framerate;
    m_bVASurface = false;
    m_bQSVSurface = false;
    m_nClientNum = 0;
    m_bAllowEncode = false;
    m_bAllowTransmit = false;
    m_id         = id;
    m_auth       = false;
    m_hw_frames_ctx = nullptr;
    m_nCodecId   = 0;
#ifdef ENABLE_MEMSHARE
    hwframe_ctx_init();
#endif
    m_tcaeEnabled = false;
}

IrrStreamer::~IrrStreamer() {
    m_Lock.unlock();
    stop();
    av_buffer_unref(&m_hw_frames_ctx);
    av_buffer_pool_uninit(&m_pPool);
}

static inline const char* get_codec_name(AVCodecID id)
{
    switch (id) {
    case AV_CODEC_ID_H264: return "h264";
    case AV_CODEC_ID_HEVC: return "h265";
    case AV_CODEC_ID_AV1: return "av1";
    default: return "unknown";
    }
}

int IrrStreamer::start(IrrStreamInfo *param) {
    CCallbackMux *pMux = nullptr;
    lock_guard<mutex> lock(m_Lock);

    if (!param) {
        Error("%s : %d : fail to get stream information!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    if (m_pTrans) {
        Error("%s : %d : CTransCoder already exits!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    float frameRate = atoi(param->framerate) ? atoi(param->framerate) : m_fFramerate;
    if(param->pix_format== (int)AV_PIX_FMT_BGRA){
        m_nPixfmt    = (AVPixelFormat)param->pix_format;
    }
    m_pDemux = new CIrrVideoDemux(m_nWidth, m_nHeight, m_nPixfmt, frameRate);
    if (!m_pDemux) {
        Error("%s : %d : fail to create Demux!\n", __func__, __LINE__);
        return AVERROR(ENOMEM);
    }

    // m_pDemux->setVASurfaceFlag(m_bVASurface);

    if (strncmp(param->plugin, "qsv", strlen("qsv")) == 0) {
        m_pDemux->setQSVSurfaceFlag(m_bQSVSurface);
    } else {
        m_pDemux->setVASurfaceFlag(m_bVASurface);//vaapi is used by default
    }

    if (!param->url) {
        Error("%s : %d : -url param must be set!\n", __func__, __LINE__);
        delete m_pDemux;
        return AVERROR(EINVAL);
    }

    if (strncmp(param->url, "local", strlen("local")) == 0) {
        string m_url(param->url), sDstUrl, sDstFormat, delim = ":";
        size_t pos = m_url.find(delim);
        if (pos != m_url.npos) {
            sDstUrl = m_url.substr(0, pos);
            sDstFormat = m_url.substr(pos+1, m_url.length());
        } else {
            Error("%s : %d : fail to get url and format!\n", __func__, __LINE__);
            delete m_pDemux;
            return AVERROR(EINVAL);
        }

        m_pTrans = new CTransCoder(m_pDemux, sDstUrl, sDstFormat);
        Debug("%s : %d : create CTransCoder successfully!\n", __func__, __LINE__);
    }

    if (strncmp(param->url, "irrv", strlen("irrv")) == 0) {
        pMux = new CCallbackMux(param->cb_params.opaque, param->cb_params.opaque2, param->cb_params.cbOpen,
                   param->cb_params.cbWrite, param->cb_params.cbWrite2, param->cb_params.cbClose,
                   param->cb_params.cbCheckNewConn, param->cb_params.cbSendMessage);

        if (!pMux) {
            Error("%s : %d :fail to create Mux!\n", __func__, __LINE__);
            delete m_pDemux;
            return AVERROR(ENOMEM);
        }

        auto transmission_allow_func = [this]() { return m_bAllowTransmit; };
        pMux->setGetTransmissionAllowedFunc(transmission_allow_func);

        param->cb_params.opaque = 0; // The fd has been passed down to pMux. Otherwise, manually close it in outer function.
        m_pTrans = new CTransCoder(m_pDemux, pMux);
        Debug("%s : %d : create CTransCoder successfully!\n", __func__, __LINE__);
    }

    if (!m_pTrans) {
        Error("%s : %d : fail to create CTransCoder!\n", __func__, __LINE__);
        delete m_pDemux;
        if (pMux)
            delete pMux;

        return AVERROR(ENOMEM);
    }

    auto run_allow_func = [this]() { return m_bAllowEncode; };
    auto clients_num_func = [this]() { return m_nClientNum; };

    m_pTrans->setGetRunAllowedFunc(run_allow_func);
    m_pTrans->setGetClientsNumFunc(clients_num_func);

    //TODO
    AVCodecID codec_type = AV_CODEC_ID_H264;
    if (strncmp(param->url, "irrv:264", strlen("irrv:264")) == 0) {
        codec_type = AV_CODEC_ID_H264;
    } else if (strncmp(param->url, "irrv:265", strlen("irrv:265")) == 0) {
        codec_type = AV_CODEC_ID_HEVC;
    } else if (strncmp(param->url, "irrv:av1", strlen("irrv:av1")) == 0) {
        codec_type = AV_CODEC_ID_AV1;
    } else {
        codec_type = AV_CODEC_ID_H264;
        Warn("%s : %d : incorrect codec setting, use h264 codec by default!\n", __func__, __LINE__);
    }
    Info("%s : %d : ICR encode type is %s\n", __func__, __LINE__, get_codec_name(codec_type));
    m_nCodecId = codec_type;

    m_pTrans->init(m_id, m_hw_frames_ctx, codec_type);

    if (param->auth) {
        m_auth = true;
        Info("socket authentication is enabled.\n");
    } else {
        Info("socket authentication is disabled.\n");
    }

    if (param->exp_vid_param) {
        string expar = param->exp_vid_param;
        do {
            string kv;
            string::size_type nSep = expar.find_first_of(':');
            if (nSep == string::npos) {
                kv = expar;
                expar.resize(0);
            } else {
                kv = expar.substr(0, nSep);
                expar.erase(0, nSep + 1);
            }

            string::size_type nKvsep = kv.find_first_of('=');
            if (nKvsep == string::npos)
                Debug("%s : %d : %s is not a key-value pair.\n",  __func__, __LINE__, kv.c_str());
            else {
                Debug("%s : %d : find '=' at kv[%lu], key = '%s', value='%s'\n", __func__, __LINE__,
                       nKvsep, kv.substr(0, nKvsep).c_str(), kv.substr(nKvsep + 1).c_str());
                m_pTrans->setOutputProp(kv.substr(0, nKvsep).c_str(), kv.substr(nKvsep + 1).c_str());
            }
        } while (expar.size());
    }

    if (param->latency_opt == 0) {
        m_pDemux->setLatencyOptFlag(false);
    }
    else {
        m_pDemux->setLatencyOptFlag(true);
    }

    if (param->renderfps_enc == 0) {
        m_pDemux->setRenderFpsEncFlag(false);
    }
    else {
        m_pDemux->setRenderFpsEncFlag(true);
        m_pDemux->setMinFpsEnc(param->minfps_enc);
    }

    if (param->skip_frame) {
        m_pTrans->setSkipFrameFlag(true);
    }
    else {
        m_pTrans->setSkipFrameFlag(false);
    }

    set_output_prop(m_pTrans, param);

    if (param->tcaeEnabled)
    {
        m_tcaeEnabled = m_pTrans->enableTcae(param->tcaeLogPath);
    }

    if (m_tcaeEnabled)
        Info("TCAE is enabled\n");
    else
        Info("TCAE is not enabled\n");

    if (m_pWriter)
        m_pTrans->setIOStreamWriter(m_pWriter);

    auto runtime_writer = std::make_shared<IORuntimeWriter>(
        m_nCodecId
    #if defined(ANDROID) || defined(__ANDROID__)
        ,"/data/"
    #endif
    );

    m_pDemux->setRuntimeWriter(runtime_writer);
    if (strncmp(param->url, "irrv", strlen("irrv")) == 0) {
        pMux->setIORuntimeWriter(runtime_writer);
        m_pMux = pMux;
    }

    m_pRuntimeWriter = runtime_writer;

    return m_pTrans->start();
}

void IrrStreamer::stop() {
    lock_guard<mutex> lock(m_Lock);
    m_pTrans->stop();
    delete m_pTrans;
    m_pTrans = nullptr;
    ///< Demux will be released by CTransCoder's deconstuctor.
    m_pDemux = nullptr;

    if (m_pWriter) {
        delete m_pWriter;
        m_pWriter = nullptr;
    }
}

int IrrStreamer::write(irr_surface_t* surface) {
    std::unique_lock<mutex> lock(m_Lock);

    if (!m_pDemux) {
        Error("%s : %d : fail to get Demux!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    if (!m_pPool) {
        size_t size = av_image_get_buffer_size(m_nPixfmt, surface->info.width, surface->info.height, 32);

        m_pPool = av_buffer_pool_init2(size, this, m_BufAlloc, nullptr);
    }

    if (!m_pPool) {
         Error("no buffer pool\n");
         return AVERROR(ENOMEM);
    }

    AVBufferRef *pBuf = av_buffer_pool_get(m_pPool);
    if (!pBuf) {
        Error("no free buffer in buffer pool now!\n");
        return AVERROR(ENOMEM);
    }

    // NOTE: we allocate much more memory than we need to store data for AV_PIX_FMT_QSV or
    // AV_PIX_FMT_VAAPI. The reason is that our pipeline is transcoding pipeline and
    // first component is "rawvideo" decoder which will copy just incoming data to output,
    // but its implementation checks width, height, stride to match buffer size. Thus,
    // we are forced to "lie" to "rawvideo" to make pipeline work.
    if (surface->encode_type == QSVSURFACE_ID) {
        memcpy(pBuf->data, &(surface->mfxSurf), sizeof(void*)); // sizeof(mfxFrameSurface1*)
    } else if (surface->encode_type == VASURFACE_ID) {
        memcpy(pBuf->data, &(surface->vaSurfaceID), sizeof(VASurfaceID));
    } else {
        int stride = surface->info.width * 4;
        int h = surface->info.height;

        if (!surface->flip_image) {
           memcpy(pBuf->data, surface->info.pdata, stride*h);
        } else {
            for (int i = 0; i < h; i++) {
                memcpy(((char*)pBuf->data) + stride * i,
                  ((char*)surface->info.pdata) + stride * (h - i - 1), stride);
            }
        }
    }

    IrrPacket pkt;

    av_init_packet(&pkt.av_pkt);
    pkt.av_pkt.buf  = pBuf;
    pkt.av_pkt.data = pkt.av_pkt.buf->data;
    pkt.av_pkt.size = pkt.av_pkt.buf->size;
    pkt.av_pkt.stream_index = 0;
    pkt.display_ctrl = std::move(surface->display_ctrl);

    return m_pDemux->sendPacket(&pkt);
}

#ifdef ENABLE_MEMSHARE
static void freeAvBuffer(void *opaque, uint8_t *data) {
    AVFrame *hw_frame = (AVFrame *)opaque;
    if(hw_frame) {
        av_frame_free(&hw_frame);
        printf("%s : %d : free buffer %p\n", __func__, __LINE__, hw_frame);
    }
}

AVBufferRef* IrrStreamer::createAvBuffer(int size) {
    if (!m_hw_frames_ctx) {
        Error("%s : %d : no hw_frames_ctx\n", __func__, __LINE__);
        return nullptr;
    }

    int ret = 0;
    AVFrame *sw_frame = nullptr;
    AVFrame *hw_frame = nullptr;

    if (!(hw_frame = av_frame_alloc())) {
        ret = AVERROR(ENOMEM);
        Error("%s : %d : failed to alloc a hw AVFrame", __func__, __LINE__);
        goto close;
    }

    if ((ret = av_hwframe_get_buffer(m_hw_frames_ctx, hw_frame, 0)) < 0) {
        Error("%s : %d : failed to get a hwframe buffer, error code: %d.\n", __func__, __LINE__, ret);
        goto close;
    }

    if (!hw_frame->hw_frames_ctx) {
        ret = AVERROR(ENOMEM);
        Error("%s : %d : no hw frame context", __func__, __LINE__);
        goto close;
    }

    if (!(sw_frame = av_frame_alloc())) {
        ret = AVERROR(ENOMEM);
        Error("%s : %d : failed to alloc a sw AVFrame", __func__, __LINE__);
        goto close;
    }

    ret = av_hwframe_map(sw_frame, hw_frame, AV_HWFRAME_MAP_WRITE | AV_HWFRAME_MAP_OVERWRITE);

    if (ret) {
        Error("%s : %d : failed to map frame into derived "
                "frame context: %d.\n", __func__, __LINE__, ret);
        goto close;
    }

    m_pTrans->set_hw_frames(sw_frame->data[0], hw_frame);
    m_pTrans->erase_encoders();

    AVBufferRef *ref;

    ref = av_buffer_create(sw_frame->data[0], size, freeAvBuffer, hw_frame, 0);

    if (!ref) {
        Error("%s : %d : failed to create av buffer\n", __func__, __LINE__);
        goto close;
    }
    return ref;

close:
    if (hw_frame)
        av_frame_free(&hw_frame);
    if (sw_frame)
        av_frame_free(&sw_frame);
    return nullptr;
}
int IrrStreamer::set_hwframe_ctx(AVBufferRef *hw_device_ctx) {
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames = nullptr;
    int ret = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        Error("%s : %d : failed to create VAAPI frame context.\n", __func__, __LINE__);
        return -1;
    }

    frames = (AVHWFramesContext *)(hw_frames_ref->data);
    frames->format    = (AVPixelFormat)AV_PIX_FMT_VAAPI;
    frames->sw_format = (AVPixelFormat)AV_PIX_FMT_RGBA;
    frames->width     = m_nWidth;
    frames->height    = m_nHeight;
    frames->initial_pool_size = 0;
    Info("%s : %d : frames, format=%d, sw_format =%d, size %dx%d\n", __func__, __LINE__,
            frames->format,
            frames->sw_format,
            frames->width,
            frames->height);
    if ((ret = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        Error("%s : %d : failed to initialize VAAPI frame context."
                "Error code: %d\n", __func__, __LINE__, ret);
        return ret;
    }
    m_hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!m_hw_frames_ctx)
        ret = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return ret;
}

void IrrStreamer::hwframe_ctx_init(){
    int ret;
    AVBufferRef *hw_device_ctx = nullptr;

    ret = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI,
            nullptr, nullptr, 0);
    if (ret < 0) {
        Error("%s : %d : failed to create a VAAPI device. Error code: %d\n", __func__, __LINE__, ret);
        return;
    }
    /* set hw_frames_ctx for encoder's AVCodecContext */
    if ((ret = set_hwframe_ctx(hw_device_ctx)) < 0) {
        Error("%s : %d : failed to set hwframe context.\n", __func__, __LINE__);
        av_buffer_unref(&hw_device_ctx);
        return;
    }
    return;
}
#endif

#ifndef ENABLE_MEMSHARE
#ifdef FFMPEG_v42
AVBufferRef* IrrStreamer::m_BufAlloc(void *opaque, int size) {
#else
AVBufferRef* IrrStreamer::m_BufAlloc(void *opaque, size_t size) {
#endif
    IrrStreamer *pThis = static_cast<IrrStreamer*> (opaque);

    /* Check if the number of packets reaches the limit */
    if (pThis->m_nCurPkts >= pThis->m_nMaxPkts)
        return nullptr;

    return av_buffer_alloc(size);
}
#endif
#ifdef ENABLE_MEMSHARE
#ifdef FFMPEG_v42
AVBufferRef* IrrStreamer::m_BufAlloc(void *opaque, int size) {
#else
AVBufferRef* IrrStreamer::m_BufAlloc(void *opaque, size_t size) {
#endif
    IrrStreamer *pThis = static_cast<IrrStreamer*> (opaque);

    /* Check if the number of packets reaches the limit */
    if (pThis->m_nCurPkts >= pThis->m_nMaxPkts)
        return nullptr;

    if (!pThis->m_pTrans) {
      pThis->Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
      return av_buffer_alloc(size);
    } else {
      pThis->Info("%s : %d : get a av buffer from transcoder\n", __func__, __LINE__);
      return pThis->createAvBuffer(size);
    }
}
#endif

int IrrStreamer::force_key_frame(int force_key_frame) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    if(force_key_frame) {
        m_pTrans->forceKeyFrame(1);
    }

    return 0;
}

int IrrStreamer::set_qp(int qp) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    m_pTrans->setQP(qp);
    return 0;
}

int IrrStreamer::set_bitrate(int bitrate) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    m_pTrans->setBitrate(bitrate);
    return 0;
}

int IrrStreamer::set_max_bitrate(int max_bitrate) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    m_pTrans->setMaxBitrate(max_bitrate);
    return 0;
}

int IrrStreamer::set_max_frame_size(int size) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pTrans->setMaxFrameSize(size);
}

int IrrStreamer::set_rolling_intra_refresh(int type, int cycle_size, int qp_delta) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    m_pTrans->setRollingIntraRefresh(type, cycle_size, qp_delta);
    return 0;
}
#ifdef FFMPEG_v42
int IrrStreamer::set_region_of_interest(int roi_num, AVRoI roi_para[]) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    m_pTrans->setRegionOfInterest(roi_num, roi_para);
    return 0;
}
#endif

int IrrStreamer::set_min_max_qp(int min_qp, int max_qp) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pTrans->setMinMaxQP(min_qp, max_qp);
}

int IrrStreamer::set_framerate(float framerate) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    m_pTrans->setFramerate(framerate);
    m_pTrans->setOutputProp("r", std::to_string(framerate).c_str());
    m_fFramerate = framerate;
    m_pDemux->updateDynamicChangedFramerate((int)framerate);
    return 0;
}

int IrrStreamer::get_framerate(void) {
    return m_fFramerate;
}

int IrrStreamer::change_resolution(int width, int height){
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    if (m_nCodecId == AV_CODEC_ID_H264) {
        if (width < MIN_REROLUTION_VALUE_H264 || height < MIN_REROLUTION_VALUE_H264) {
            Error("%s : %d : When codec is H264, min width or height is 32! width = %d, height = %d\n", __func__, __LINE__, width, height);
            return AVERROR(EINVAL);
        }
    }

    if (m_nCodecId == AV_CODEC_ID_HEVC) {
        if (width < MIN_REROLUTION_VALUE_HEVC || height < MIN_REROLUTION_VALUE_HEVC) {
            Error("%s : %d : When codec is HEVC, min width or height is 128! width = %d, height = %d\n", __func__, __LINE__, width, height);
            return AVERROR(EINVAL);
        }
    }

    if (m_nCodecId == AV_CODEC_ID_AV1) {
        if (width < MIN_REROLUTION_VALUE_AV1 || height < MIN_REROLUTION_VALUE_AV1) {
            Error("%s : %d : When codec is AV1, min width or height is 128! width = %d, height = %d\n", __func__, __LINE__, width, height);
            return AVERROR(EINVAL);
        }
    }

    m_pTrans->changeResolution(width, height);
    return 0;
}

int IrrStreamer::change_codec(AVCodecID codec_type){
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    if (m_nCodecId != AV_CODEC_ID_H264 && m_nCodecId != AV_CODEC_ID_HEVC && m_nCodecId != AV_CODEC_ID_AV1) {
        Error("unsupported codec: %d\n", codec_type);
        return AVERROR(EINVAL);
    }

    if (m_pTrans->changeCodec(codec_type) == 0) {
        m_nCodecId = codec_type;
        m_pMux->sendMessage(IRRV_MESSAGE_VIDEO_FORMAT_CHANGE, codec_type);
        m_pRuntimeWriter->changeCodecType(codec_type);
    }
    return 0;
}

int IrrStreamer::setLatency(int latency) {
    lock_guard<mutex> lock(m_Lock);

    if(!m_pDemux || !m_pTrans) {
        Error("%s : %d : no Demux or CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    Info("%s : %d : latency = %d.\n", __func__, __LINE__, latency);
    m_pDemux->setLatencyStats(latency);
    m_pTrans->setLatencyStats(latency);
    return 0;
}

int IrrStreamer::getWidth() {
    return m_nWidth;
}

int IrrStreamer::getHeight() {
    return m_nHeight;
}

int IrrStreamer::getEncoderType() {
    return m_nCodecId;
}

void IrrStreamer::setVASurfaceFlag(bool bVASurfaceID) {
    m_bVASurface = bVASurfaceID;
    if (m_pDemux) {
        m_pDemux->setVASurfaceFlag(bVASurfaceID);
    }
}

bool IrrStreamer::getVASurfaceFlag() {
    return m_bVASurface;
}

void IrrStreamer::setQSVSurfaceFlag(bool bQSVSurfaceID) {
    m_bQSVSurface = bQSVSurfaceID;
    if (m_pDemux) {
        m_pDemux->setQSVSurfaceFlag(bQSVSurfaceID);
    }
}

bool IrrStreamer::getQSVSurfaceFlag() {
    return m_bQSVSurface;
}

void IrrStreamer::incClientNum() {
    lock_guard<mutex> lock(m_Lock);
    m_nClientNum++;
}

void IrrStreamer::decClientNum() {
    lock_guard<mutex> lock(m_Lock);
    m_nClientNum--;
}

int IrrStreamer::getClientNum() {
    return m_nClientNum;
}

void  IrrStreamer::setEncodeFlag(bool bAllowEncode) {
    lock_guard<mutex> lock(m_Lock);
    m_bAllowEncode = bAllowEncode;
}

bool  IrrStreamer::getEncodeFlag() {
    return m_bAllowEncode;
}

void  IrrStreamer::setTransmitFlag(bool bAllowTransmit) {
    lock_guard<mutex> lock(m_Lock);
    m_bAllowTransmit = bAllowTransmit;
}

bool  IrrStreamer::getTransmitFlag() {
    lock_guard<mutex> lock(m_Lock);
    return m_bAllowTransmit;
}


void  IrrStreamer::setFisrtStartEncoding(bool bFirstStartEncoding) {
    lock_guard<mutex> lock(m_Lock);
    if (!m_pDemux) {
        Error("%s : %d : no Demux!\n", __func__, __LINE__);
        return ;
    }

    m_pDemux->setFirstStartEncoding(bFirstStartEncoding);
}

int IrrStreamer::set_sei(int sei_type, int sei_user_id) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pTrans->setSEI(sei_type, sei_user_id);
}

int IrrStreamer::set_gop_size(int size) {
    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    lock_guard<mutex> lock(m_Lock);

    return m_pTrans->setGopSize(size);
}

bool  IrrStreamer::getAuthFlag() {
    return m_auth;
}

void  IrrStreamer::set_screen_capture_flag(bool bAllowCapture) {
    if (m_pTrans == nullptr) {
        printf("transcoder is not started\n");
        return;
    }

    lock_guard<mutex> lock(m_Lock);

    m_pTrans->setScreenCaptureFlag(bAllowCapture);
}

void IrrStreamer::set_screen_capture_interval(int captureInterval) {
    if (m_pTrans == nullptr) {
        printf("transcoder is not started\n");
        return;
    }

    lock_guard<mutex> lock(m_Lock);

    m_pTrans->setScreenCaptureInterval(captureInterval);
}

void IrrStreamer::set_screen_capture_quality(int quality_factor) {
    if (m_pTrans == nullptr) {
        printf("transcoder is not started\n");
        return;
    }

    lock_guard<mutex> lock(m_Lock);

    m_pTrans->setScreenCaptureQuality(quality_factor);
}

void IrrStreamer::set_iostream_writer_params(const char *input_file, const int width, const int height,
                                             const char *output_file, const int output_frame_number) {
    lock_guard<mutex> lock(m_Lock);

    if (m_pWriter)
        delete m_pWriter;

    IOStreamWriter *writer = new IOStreamWriter();

    if (input_file) {
        int frame_size = width * height * 4; // assume 32bpp
        auto foo = [this](const int w, const int h, const void *p, const int f) {
            irr_surface_t surface{};
            surface.info.width = w;
            surface.info.height = h;
            surface.info.pdata = (unsigned char*)p;
            surface.flip_image = f;
            surface.encode_type = DATA_BUFFER;
            write(&surface);
        };

        writer->setInputStream(input_file, frame_size, width, height, foo);
    }

#ifdef BUILD_FOR_HOST
    const std::string outfile_path_prefix = "";
#else
    const std::string outfile_path_prefix = "/data/";
#endif

    if (output_file) {
        std::string full_path = outfile_path_prefix + output_file;
        writer->setOutputStream(full_path);
    }

    if (output_frame_number > 0)
        writer->setOutputFrameNumber(output_frame_number);

    m_pWriter = writer;
}

#ifdef ENABLE_TCAE
int IrrStreamer::set_client_feedback(unsigned int delay, unsigned int size)
{
    if (m_tcaeEnabled)
        return m_pTrans->setClientFeedback(delay, size);
    else
        return 0;
}
#endif

void IrrStreamer::set_output_prop(CTransCoder *m_pTrans, IrrStreamInfo *param) {

    if (param->rc_params.bitrate)
        m_pTrans->setOutputProp("b", param->rc_params.bitrate);

    if (strncmp(param->plugin, "vaapi", strlen("vaapi")) == 0) {
        m_pTrans->EnableVaapiPlugin();
    }
    
    if (strncmp(param->plugin, "qsv", strlen("qsv")) == 0) {
        m_pTrans->EnableQsvPlugin();
    }

    if (param->codec)
        m_pTrans->setOutputProp("c", param->codec);
    else {
        if (strncmp(param->url, "irrv:264", strlen("irrv:264")) == 0) {
            if (strncmp(param->plugin, "qsv", strlen("qsv")) == 0)
                m_pTrans->setOutputProp("c", "h264_qsv");
            else
                m_pTrans->setOutputProp("c", "h264_vaapi");
        } else if (strncmp(param->url, "irrv:265", strlen("irrv:265")) == 0) {
            if (strncmp(param->plugin, "qsv", strlen("qsv")) == 0)
                m_pTrans->setOutputProp("c", "hevc_qsv");
            else
                m_pTrans->setOutputProp("c", "hevc_vaapi");
        } else if (strncmp(param->url, "irrv:av1", strlen("irrv:av1")) == 0) {
            if (strncmp(param->plugin, "qsv", strlen("qsv")) == 0)
                m_pTrans->setOutputProp("c", "av1_qsv");
            else
                m_pTrans->setOutputProp("c", "av1_vaapi");
        }
    }

    if (param->framerate)
        m_pTrans->setOutputProp("r", param->framerate);

    if (param->low_power)
        m_pTrans->setOutputProp("low_power", "1");

    if (param->res)
        m_pTrans->setOutputProp("s", param->res);

    if (param->gop_size)
        m_pTrans->setOutputProp("g", param->gop_size);

    if (param->rc_params.qfactor) {
#ifdef FFMPEG_v42
        m_pTrans->setOutputProp("global_quality", param->rc_params.qfactor);
#else
        m_pTrans->setOutputProp("quality_factor", param->rc_params.qfactor);
#endif
    }

    if (param->rc_params.qp)
        m_pTrans->setOutputProp("qp", param->rc_params.qp);

    if (param->rc_params.maxrate)
        m_pTrans->setOutputProp("maxrate", param->rc_params.maxrate);

    if (param->rc_params.ratectrl) {
#ifdef FFMPEG_v42
        m_pTrans->setOutputProp("rc_mode", param->rc_params.ratectrl);
#endif
    }

    if (param->max_frame_size)
        m_pTrans->setOutputProp("max_frame_size", param->max_frame_size);

    if (param->slices)
        m_pTrans->setOutputProp("slices", param->slices);

    if (param->ref_info.int_ref_type)
        m_pTrans->setOutputProp("int_ref_type", param->ref_info.int_ref_type);

    if (param->ref_info.int_ref_cycle_size)
        m_pTrans->setOutputProp("int_ref_cycle_size", param->ref_info.int_ref_cycle_size);

    if (param->ref_info.int_ref_qp_delta)
        m_pTrans->setOutputProp("int_ref_qp_delta", param->ref_info.int_ref_qp_delta);

    if (param->roi_info.roi_enabled) {
#ifdef FFMPEG_v42
        m_pTrans->setOutputProp("roi_enabled", param->roi_info.roi_enabled);
        if(param->roi_info.x)
            m_pTrans->setOutputProp("roi_x", param->roi_info.x);
        if(param->roi_info.y)
            m_pTrans->setOutputProp("roi_y", param->roi_info.y);
        if(param->roi_info.width)
            m_pTrans->setOutputProp("roi_w", param->roi_info.width);
        if(param->roi_info.height)
            m_pTrans->setOutputProp("roi_h", param->roi_info.height);
        if(param->roi_info.roi_value)
            m_pTrans->setOutputProp("roi_value", param->roi_info.roi_value);
#else
        Info("ROI is not supported in this FFmpeg version.\n");
#endif
    }

    if (param->rc_params.qmaxI)
        m_pTrans->setOutputProp("qmaxI", param->rc_params.qmaxI);

    if (param->rc_params.qminI)
        m_pTrans->setOutputProp("qminI", param->rc_params.qminI);

    if (param->rc_params.qmaxP)
        m_pTrans->setOutputProp("qmaxP", param->rc_params.qmaxP);

    if (param->rc_params.qminP)
        m_pTrans->setOutputProp("qminP", param->rc_params.qminP);

    if (param->quality)
        m_pTrans->setOutputProp("compression_level", param->quality);

    if (param->rc_params.bufsize)
        m_pTrans->setOutputProp("bufsize", param->rc_params.bufsize);

    //turn on udu_sei option
    if (strncmp(param->plugin, "qsv", strlen("qsv")) == 0) {
        m_pTrans->setOutputProp("udu_sei", 1);
    }

    if (param->sei) {
        m_pTrans->setOutputProp("sei", param->sei);
        if(strncmp(param->plugin, "qsv", strlen("qsv")) == 0) {
            if (param->sei & 1)
                m_pTrans->setOutputProp("hrd_compliance", 1);
            if (param->sei & 4)
                m_pTrans->setOutputProp("recovery_point_sei", 1);
            if (param->sei & 8)
                m_pTrans->setOutputProp("pic_timing_sei", 1);
        }
    }

    if (param->gop_size)
        m_pTrans->setOutputProp("g", param->gop_size);

    if (param->profile)
        m_pTrans->setOutputProp("profile", param->profile);

    if (param->level)
        m_pTrans->setOutputProp("level", param->level);

    if (param->filter_nbthreads)
        m_pTrans->setOutputProp("filter_threads", param->filter_nbthreads);

    if (param->low_delay_brc)
        m_pTrans->setOutputProp("low_delay_brc", 1);

    if(strncmp(param->plugin, "qsv", strlen("qsv")) == 0) {
        // set the I frames as IDR frames in cloud gaming
        m_pTrans->setOutputProp("forced_idr", "1");

        // set the skip frame flag
        if (param->skip_frame)
            m_pTrans->setOutputProp("skip_frame", 3); // support brc mode to align with vaapi-path
    }
}

void IrrStreamer::set_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                           int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop) {
    if (m_pTrans == nullptr) {
        printf("transcoder is not started\n");
        return;
    }

    m_pTrans->set_crop(client_rect_right, client_rect_bottom, fb_rect_right, fb_rect_bottom, crop_top, crop_bottom, crop_left, crop_right, valid_crop);
}


void  IrrStreamer::setSkipFrameFlag(bool bSkipFrame) {
    if (m_pTrans == nullptr) {
        Info("transcoder is not started\n");
        return;
    }

    lock_guard<mutex> lock(m_Lock);

    m_pTrans->setSkipFrameFlag(bSkipFrame);
}

bool  IrrStreamer::getSkipFrameFlag() {
    if (m_pTrans == nullptr) {
        Info("transcoder is not started\n");
        return false;
    }

    lock_guard<mutex> lock(m_Lock);

    return m_pTrans->getSkipFrameFlag();
}

void IrrStreamer::set_alpha_channel_mode(bool isAlpha) {
    lock_guard<mutex> lock(m_Lock);
    m_pTrans->setVideoMode(isAlpha);
}

void IrrStreamer::set_buffer_size(int width, int height) {
    lock_guard<mutex> lock(m_Lock);
    m_pTrans->setFrameBufferSize(width, height);
}

int IrrStreamer::getEncodeNewWidth() {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pTrans->getEncodeNewWidth();
}

int IrrStreamer::getEncodeNewHeight() {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pTrans->getEncodeNewHeight();
}

int IrrStreamer::change_profile_level(const int iProfile, const int iLevel) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pTrans->changeEncoderProfileLevel(iProfile, iLevel);
}

void IrrStreamer::setRenderFpsEncFlag(bool bRenderFpsEnc) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return;
    }

    m_pTrans->setRenderFpsEncFlag(bRenderFpsEnc);
    return;
}

int IrrStreamer::getRenderFpsEncFlag(void) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pTrans) {
        Error("%s : %d : no CTransCoder!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pTrans->getRenderFpsEncFlag();
}

void IrrStreamer::setLatencyOptFlag(bool bLatencyOpt) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pDemux) {
        Error("%s : %d : m_pDemux is null!\n", __func__, __LINE__);
        return;
    }

    m_pDemux->setLatencyOptFlag(bLatencyOpt);
    return;
}

int IrrStreamer::getLatencyOptFlag(void) {
    lock_guard<mutex> lock(m_Lock);

    if (!m_pDemux) {
        Error("%s : %d : m_pDemux is null!\n", __func__, __LINE__);
        return AVERROR(EINVAL);
    }

    return m_pDemux->getLatencyOptFlag();
}
