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

#ifndef IRRSTREAMER_H
#define IRRSTREAMER_H

#include <map>
#include <inttypes.h>
#include <stddef.h>
#include "data_types.h"
#include "CTransLog.h"
#include "core/CCallbackMux.h"
#include "core/CIrrVideoDemux.h"
#include "core/CTransCoder.h"
#include "utils/IOStreamWriter.h"
#include "utils/IORuntimeWriter.h"
#include "irrv/irrv_protocol.h"

#define MIN_REROLUTION_VALUE_H264 32
#define MIN_REROLUTION_VALUE_HEVC 128
#define MIN_REROLUTION_VALUE_AV1 128

class IrrStreamer : public CTransLog {
public:
    static IrrStreamer* get();
    static void Register(int id, int w, int h, float framerate);
    static void Unregister();

    struct IrrStreamInfo {
        int pix_format;            ///< fmt
        /* Output-only parameters */
        int gop_size;              ///< Group Of Picture size, default 120
        const char *codec;         ///< Encoder codec, e.x. h264_qsv; may be null
        const char *format;        ///< Mux format, e.x. flv; null as auto
        const char *url;           ///< Output url.
        int low_power;             ///< Enable low-power mode, default not.
        const char *res;           ///< Encoding resolution.
        const char *framerate;     ///< Encoding framerate
        const char *exp_vid_param; ///< Extra encoding/muxer parameters passed to libtrans/FFmpeg
        bool bVASurface;
        rate_control_info_t rc_params;
        int quality;               ///< Encoding quality level
        int max_frame_size;        ///< Encoding max frame size
        irr_ref_info_t ref_info;
        irr_roi_info_t roi_info;
        int slices;                ///< Encoder number of slices, used in parallelized encoding
        int sei;                   ///< Encoding SEI information
        int latency_opt;           ///< Encoding latency optimization, set 1 to enable, 0 to disable
        bool auth;                 ///< Enable Socket authentication
        int renderfps_enc;         ///< Encoding by rendering fps, set 1 to enable, 0 to disable, default is 0.
        int minfps_enc;            ///< min encode fps when renderfps_enc is used.
        const char *profile;       ///< Encoder profile
        const char *level;         ///< Encoder level
        int filter_nbthreads;      ///< filter threads number
        bool low_delay_brc;        ///< enable TCBRC that trictly obey average frame size set by target bitarte
        bool skip_frame;           ///< enable Skip Frame
        const char *plugin;        ///< Encoder plugin option
        bool bQSVSurface;          ///< Is QSV Surface used
        bool tcaeEnabled;          ///< Is TCAE enabled
        const char *tcaeLogPath;   ///< TCAE log file path

        struct CallBackTable {     ///< Callback function tables
            void *opaque;          ///< Used by callback functions
            void *opaque2;         ///< Used by callback functions
            int (*cbOpen) (void */*opaque*/, int /*w*/, int /*h*/, float /*frame_rate*/);
            /* Synchronous write callback*/
            int (*cbWrite) (void */*opaque*/, uint8_t */*data*/, size_t/*size*/);
            int (*cbWrite2) (void */*opaque*/, uint8_t */*data*/, size_t/*size*/, int /* type */);
            void (*cbClose) (void */*opaque*/);
            int (*cbCheckNewConn) (void */*opaque*/);
            int (*cbSendMessage)(void */*opaque*/, int /*msg*/, unsigned int /*value*/);
        } cb_params;
    };

    IrrStreamer(int id, int w, int h, float framerate);
    ~IrrStreamer();

    int   start(IrrStreamInfo *param);
    void  stop();
    int   write(irr_surface_t* surface);
    int   force_key_frame(int force_key_frame);
    int   set_qp(int qp);
    int   set_bitrate(int bitrate);
    int   set_max_bitrate(int max_bitrate);
    int   set_max_frame_size(int size);
    int   set_rolling_intra_refresh(int type, int cycle_size, int qp_delta);
#ifdef FFMPEG_v42
    int   set_region_of_interest(int roi_num, AVRoI roi_para[]);
#endif
    int   set_min_max_qp(int min_qp, int max_qp);
    int   change_resolution(int width, int height);
    int   change_codec(AVCodecID codec_type);
    int   setLatency(int latency);
    int   getWidth();
    int   getHeight();
    int   getEncoderType();
    int   set_framerate(float framerate);
    int   get_framerate(void);
    int   set_sei(int sei_type, int sei_user_id);
    int   set_gop_size(int size);
    void  set_screen_capture_flag(bool bAllowCapture);
    void  set_screen_capture_interval(int captureInterval);
    void  set_screen_capture_quality(int quality_factor);
    void  set_iostream_writer_params(const char *input_file, const int width, const int height,
                                     const char *output_file, const int output_frame_number);

#ifdef ENABLE_TCAE
    int   set_client_feedback(unsigned int delay, unsigned int size);
#endif

    void setVASurfaceFlag(bool bVASurfaceID);
    bool getVASurfaceFlag();

    void setQSVSurfaceFlag(bool bQSVSurfaceID);
    bool getQSVSurfaceFlag();

    void  incClientNum();
    void  decClientNum();
    int   getClientNum();
    void  setEncodeFlag(bool bAllowEncode);
    bool  getEncodeFlag();
    void  setTransmitFlag(bool bAllowTransmit);
    bool  getTransmitFlag();
    void  setFisrtStartEncoding(bool bFirstStartEncoding);
    bool  getAuthFlag();
    void  hwframe_ctx_init();
    int   set_hwframe_ctx(AVBufferRef *hw_device_ctx);
    AVBufferRef* createAvBuffer(int size);
    void set_output_prop(CTransCoder *m_pTrans, IrrStreamInfo *param);

    void  set_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                   int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop);
    IORuntimeWriter::Ptr getRunTimeWriter() { return m_pRuntimeWriter; }

    void  setSkipFrameFlag(bool bSkipFrame);
    bool  getSkipFrameFlag();

    void set_alpha_channel_mode(bool isAlpha);
    void set_buffer_size(int width, int height);

    int getEncodeNewWidth();
    int getEncodeNewHeight();

    /*
    *  * @Desc change the profile and level of the codec
    *  * @param [in] iProfile, iLevel
    *  * @return 0 on success, minus mean fail or not change at all.
    */
    int change_profile_level(const int iProfile, const int iLevel);

    void setRenderFpsEncFlag(bool bRenderFpsEnc);
    /**
    * get the encode by render fps flag.
    *
    * @return minus mean that the call of the function fail, 1 mean that encode by render fps is trun on, 0 mean turn off.
    */
    int getRenderFpsEncFlag(void);

    void setLatencyOptFlag(bool bLatencyOpt);

    /**
    * get the encoder latency optimization flag
    *
    * @return minus mean that the call of the function fail, 1 mean that optimization flag is trun on, 0 mean turn off.
    */
    int getLatencyOptFlag(void);

private:
    CIrrVideoDemux *m_pDemux;
    CTransCoder    *m_pTrans;
    CCallbackMux   *m_pMux;
    IOStreamWriter *m_pWriter;
    IORuntimeWriter::Ptr m_pRuntimeWriter;
    AVBufferPool  *m_pPool = nullptr;
    int            m_nMaxPkts;   ///< Max number of cached frames
    int            m_nCurPkts;
    AVPixelFormat  m_nPixfmt;
    int            m_nWidth, m_nHeight;
    int            m_nCodecId;
    float          m_fFramerate;
    std::mutex     m_Lock;
    bool m_bVASurface;
    bool m_bQSVSurface;
    int            m_nClientNum;
    bool           m_bAllowEncode;
    bool           m_bAllowTransmit;
    int            m_id;
    bool           m_auth;
    AVBufferRef   *m_hw_frames_ctx;
    bool           m_tcaeEnabled;

#ifdef FFMPEG_v42
    static AVBufferRef* m_BufAlloc(void *opaque, int size);
#else
    static AVBufferRef* m_BufAlloc(void *opaque, size_t size);
#endif
};

#endif /* IRRSTREAMER_H */
