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

#ifndef CTRANSCODER_H
#define CTRANSCODER_H

#include <mutex>
#include <thread>
#include <string>
#include <map>
#include <queue>
#include <functional>
#include <signal.h>
#include <unistd.h>
#include "CDemux.h"
#include "CMux.h"
#include "utils/ProfTimer.h"
#include "utils/IOStreamWriter.h"
#include "utils/TimeLog.h"

#ifdef ENABLE_TCAE
class CTcaeWrapper;
#endif

class CTransLog;
class CDecoder;
class CFilter;
class CEncoder;

#define RESOLUTION_WIDTH_DEFAULT   576
#define RESOLUTION_WIDTH_MIN       32
#define RESOLUTION_WIDTH_MAX       4096
#define RESOLUTION_HEIGHT_DEFAULT  960
#define RESOLUTION_HEIGHT_MIN      32
#define RESOLUTION_HEIGHT_MAX      4096

#define HW_ERROR_INTERVAL          5000
#define HW_ERROR_DURATION_MAX      300
#define HW_ERROR_COUNT_MAX         10

#define MAX_ROI_NUM                8

#define DEFAULT_SCREEN_CAPTURE_QUALITY 80
#define SIZE_CHANGE_THRESHOLD 5000

using getTranscoderRunAllowedFlag = std::function<bool(void)>;
using getTranscoderClientsNum = std::function<int(void)>;

typedef struct _crop_info {
    int    client_rect_right;
    int    client_rect_bottom;
    int    fb_rect_right;
    int    fb_rect_bottom;
    int    crop_top;    // crop offset from top border
    int    crop_bottom; // crop offset from bottom border
    int    crop_left;   // crop offset from left border
    int    crop_right;  // crop offset from right border
    int    valid_crop;
} crop_info;

class CTransCoder {
public:
    /**
     * Common Constructor.
     * @param sSrcUrl    input file or url
     * @param sDstUrl    output file or url
     * @param sDstFormat output format. This is necessary if no muxer can be determined
     *                   from the sDstUrl.
     */
    CTransCoder(std::string sSrcUrl, std::string sDstUrl, std::string sDstFormat = "");
    /**
     * Constructor with user-defined demuxer
     * @param sSrcUrl
     * @param pMux    user-defined muxer object.
     * Note: CTransCoder will take control of this object, so use 'new' to
     * allocate this object.
     */
    CTransCoder(std::string sSrcUrl, CMux *pMux);
    CTransCoder(CDemux *pDemux, CMux *pMux);
    CTransCoder(CDemux *pDemux, std::string sDstUrl, std::string sDstFormat = "");
    ~CTransCoder();
    /* Control functions */
    /**
     * init the CTransCoder instance
     */
    void init(int id, AVBufferRef *hw_frames_ctx, AVCodecID codec_type);
    /**
     * Start the process.
     * @return 0 on success, otherwise fails.
     */
    int start();
    /**
     * Wait the transcoder task to finish.
     * That is, the process only terminates when coming accross EOF or error.
     */
    void stop();
    /**
     * Set properties for demuxer/muxer
     * @param key      the property to be set
     * @param value
     * @return 0 on success.
     *
     * @Note: For common demuxers, these properties may be ignored since all necessary
     * information can be found by probing the input stream. But for demuxers such as
     * 'rawvideo', 'width'/'height'/'pixel_format' must be set, or the program will crash.
     * For muxers, supported options are 'w', 'h', 'pix_fmt', 'framerate', 'c', 'codec',
     * 'g', 'b' and some other options which FFmpeg supports.
     */
    int setInputProp(const char *key, const char *value);
    int setOutputProp(const char *key, const char *value);
    inline int setOutputProp(const char *key, int value) {
        std::string str = std::to_string(value);
        return setOutputProp(key, str.c_str());
    }

    void run();

    /* force key frame encoding */
    int forceKeyFrame(int force);
    /* set QP */
    int setQP(int qp);

    /* set bitrate */
    int setBitrate(int bitrate);

    /* set max bitrate */
    int setMaxBitrate(int max_bitrate);

    /* set framerate */
    int setFramerate(float framerate);

    /* set max frame size */
    int setMaxFrameSize(int size);

    /* set rolling intra refresh  */
    int setRollingIntraRefresh(int type, int cycle_size, int qp_delta);

#ifdef FFMPEG_v42
    /* set region of interest  */
    int setRegionOfInterest(int roi_num, AVRoI roi_para[]);
#endif

    /*set min qp and max qp */
    int setMinMaxQP(int min_qp, int max_qp);

    /* change resulution*/
    int changeResolution(int width, int height);

    /**
     * change codec
     *
     * @return 0 if change the codec successfully, -1 if error or no need to change.
     */
    int changeCodec(AVCodecID codec_type);

    /* get init bitrate */
    int getInitBitrate(AVDictionary *dict);

    /* get max bitrate */
    int getMaxBitrate(AVDictionary *dict);

    void erase_encoders();

    void erase_filters();

    void hwErrorCount();

    /**
     * enable latency statistic.
     */
    int setLatencyStats(int nLatencyStats);

    /**
    * init the hw frame ctx.
    */
    int initHwFramesCtx();

    /**
    * create a hw frame with zero buffer content for encoder in case aic is not connect.
    */
    AVBufferRef* createHwFrameBuffer(int size, VASurfaceID *vaSurfaceId);

    /**
     * set sei type and user id
     */
    int setSEI(int sei_type, int sei_user_id);

    /**
     * set gop size
     */
    int setGopSize(int size);

    /* get init gop size */
    int getInitGopSize(AVDictionary *dict);

    /* set screen capture flag */
    void setScreenCaptureFlag(bool bAllowCapture);

    /* set screen capture interval */
    void setScreenCaptureInterval(int capture_interval);

    /* set screen capture quality factor, the quality factor must be in 1-100. */
    void setScreenCaptureQuality(int qualityFactor);

    void setIOStreamWriter(IOStreamWriter *writer);

    void setGetRunAllowedFunc(getTranscoderRunAllowedFlag func) { getRunAllowed = func; }

    void setGetClientsNumFunc(getTranscoderClientsNum func) { getClientsNum = func; }

#ifdef ENABLE_MEMSHARE
    /**
     * Allocate an AVBuffer of the given size using avCreateBuffer().
     *
     * @return an AVBufferRef of given size or NULL when out of memory
     */
    AVBufferRef* createAvBuffer(int size);
    void set_hw_frames(uint8_t *data, AVFrame *hw_frame);
    void set_hw_frames_ctx(AVBufferRef *hw_frames_ctx);
#endif

    void set_crop(int client_rect_right, int client_rect_bottom, int fb_rect_right, int fb_rect_bottom, 
                  int crop_top, int crop_bottom, int crop_left, int crop_right, int valid_crop);

    void setSkipFrameFlag(bool bSkipFrame);
    bool getSkipFrameFlag();

    void setVideoMode(bool isAlpha);

    void setFrameBufferSize(int widht, int height);

  
    int getEncodeNewWidth();
    int getEncodeNewHeight();

    /**
    * Change the encoder's profile or level
    *
    * @return 0 if sucess or -1 for fail(both input profile and level is not support)
    */
    int changeEncoderProfileLevel(const int iProfile, const int iLevel);
    inline void EnableVaapiPlugin() { m_vaapiPlugin = true; m_qsvPlugin = false; }
    inline void EnableQsvPlugin() { m_qsvPlugin = true; m_vaapiPlugin = false; }

    bool enableTcae(const char* tcaeLogPath = nullptr);
#ifdef ENABLE_TCAE
    /* set size and delay from client feedback */
    int setClientFeedback(unsigned int delay, unsigned int size);
#endif

    void setRenderFpsEncFlag(bool bRenderFpsEnc);
    bool getRenderFpsEncFlag(void);

private:
    /* Disable copy constructor and = */
    CTransCoder(const CTransCoder& orig);
    CTransCoder& operator=(const CTransCoder& orig);
    bool allStreamFound();
    int processInput();
    int newInputStream(int strIdx);
    int decode(int strIdx);
    int doOutput(bool flush);
    const char* getInOptVal(const char *short_name, const char *long_name,
                            const char *default_value = nullptr);
    const char* getOutOptVal(const char *short_name, const char *long_name,
                            const char *default_value = nullptr);
    void dynamicSetEncParameters(CEncoder *pEnc, AVFrame *pFrame, AVFrame **pFrameEnc);
    void updateDynamicChangedFramerate(int framerate);

    void updateFrameSkipped();

protected:
    int publishStatusToResourceMonitor(uint32_t, void*);

private:
    CDemux                   *m_pDemux = nullptr;
    std::map<int, CDecoder *> m_mDecoders;
    std::map<int, CFilter *>  m_mFilters;
    std::map<int, CEncoder *> m_mEncoders;
    CMux                     *m_pMux = nullptr;
    CEncoder                 *m_MJPEGEncoder = nullptr;
    IOStreamWriter           *m_pIOStreamWriter = nullptr;

    std::queue<std::unique_ptr<vhal::client::display_control_t>>   m_DispCtrlQueue;
    std::map<int, bool>       m_mStreamFound;
    volatile bool m_stop = false;
    std::thread m_thread;
    std::mutex m_mutex;
    CTransLog                *m_Log = nullptr;
    AVDictionary             *m_pInProp = nullptr, *m_pOutProp = nullptr;
    AVDictionary             *m_pExtProp = nullptr;
    int m_forceKeyFrame = 0;    // force key frame encoding
    int m_setQP = 0;            // set encode QP
    int m_setBitrate = 0;       // set encode bitrate
    int m_bitrate = 0;          // encode bitrate
    int m_setMaxBitrate = 0;    // set encode max bitrate
    int m_maxBitrate = 0;       // encode max bitrate
    int m_setMinMaxQP = 0;      // set encode min/max qp
    int m_maxQP = 0;            // encode max qp;
    int m_minQP = 0;            // encode min qp;
    float m_setFramerate = 0.0; // set framerate
    float m_frameRate = 0.0;    // encode framerate;
    int m_setMaxframeSize = 0;  // set encode max frame size
    bool m_setRIR = false;      // set encode RIR(Rolling intra refresh)
    int  m_IntRefType = 0;      // encode RIR type
    int  m_IntRefCycleSize = 0; // encode RIR cycle size
    int  m_IntRefQPDelta = 0;   // encode RIR delta qp
    bool m_setROI = false;      // set encode ROI(Region of interest)
#ifdef FFMPEG_v42
    int  m_numROI = 0;          // number of regions
    AVRoI m_roiPara[MAX_ROI_NUM];
#endif
    bool m_setSEI = false;                  // set encode SEI
    int  m_SEIType = 0;                     // SEI type
    int  m_SEIUserId = 0;                   // SEI user id
    bool m_isEncHWError = false;
    bool m_setDynamicEncode = false;        // dynamic encode setting
    TimeLog *m_DyEncodeTimeLog = nullptr;   // dynamic encode time log
    int m_gopSize = 0;                      // encode gop size
    bool m_setGopSize = false;              // set encode gop size
    bool m_screenCaptureFlag = false;       // dynamic screen capture setting
    int  m_screenCaptureInterval = 0;       // screen capture interval
    int  m_countScreenCapture = 0;          // to start count when m_setScreenCaptureStart is ture and MJEPG encoder to encode
                                            // when it upto screen capture interval and after set it to 0.
    int m_screenCaptureQuality = DEFAULT_SCREEN_CAPTURE_QUALITY; // screen capture quality,the quality value in 1-100, default 80.
    AVCodecID  m_nCodecId = AV_CODEC_ID_NONE; // encode codec type

    bool m_isResolutionChange = false;
    bool m_isCodecChange = false;
    int m_newWidth = 0;
    int m_newHeight = 0;

    bool m_isFramerateChange = false;

    bool m_bSkipFrame = false;
    int m_nTotalFrameskipped = 0;
    int m_nFrameskipped = 0;
    int m_nFrameEncoded = 0;
    long long  m_startTimestampUS = 0;
    long long  m_curTimestampUS = 0;

    bool m_isAlphaChannelMode = false;


    std::map<std::string, ProfTimer *> m_mProfTimer;
    int m_nLatencyStats = 0;
    int m_nLastPktSize = 0;
    std::map<int, int64_t> m_mPktPts;

	long m_g_transcode_count = 0L;
    int m_id = 0;
    bool m_fpsStats = true;
    uint64_t m_statsStartTimeInMs = 0ULL;
    int  m_hwErrorCnt = 0;
    uint64_t m_hwErrorOccurTimeInMs   = 0ULL;
    uint64_t m_hwErrorDurationInMs    = 0ULL;

    AVBufferRef  *m_phw_frames_ctx = nullptr;

    AVBufferRef  *m_phw_frames_buffer = nullptr;
    VASurfaceID   m_phw_frames_surfaceid = VA_INVALID_SURFACE;
    std::map<const void*, AVFrame *> m_mHwFrames;
    AVBufferRef *m_hw_frames_ctx = nullptr;

    getTranscoderRunAllowedFlag getRunAllowed = nullptr;
    getTranscoderClientsNum getClientsNum = nullptr;

    crop_info m_crop = { 0 };
    // statistics real-time encoding bitrate, the bitrate = total frame size in framerate cycle.
    int m_frameNumInFrameRate = 0;
    uint64_t m_totalFrameSizeInFrameRate = 0ULL;

    int m_buffer_width = 0;
    int m_buffer_height = 0;

    int m_prev_client_right = 0;
    int m_prev_client_bottom = 0;

    bool m_isProfileLevelChange = false;
    bool m_vaapiPlugin = true;
    bool m_qsvPlugin = false;

#ifdef ENABLE_TCAE
    CTcaeWrapper *m_tcae = nullptr;
    bool m_tcaeEnabled = false;
    const char* m_tcaeLogPath = nullptr;
#endif
};

#endif /* CTRANSCODER_H */
