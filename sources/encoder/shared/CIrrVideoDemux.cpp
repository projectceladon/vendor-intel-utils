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

#include <chrono>
#include "utils/CTransLog.h"
#include "CIrrVideoDemux.h"

#define FIRST_START_ENCODE_FPS_DEFAULT 60

CIrrVideoDemux::CIrrVideoDemux(int w, int h, int format, float framerate):m_Lock(), m_cv() {
    m_Info.m_pCodecPars->codec_type = AVMEDIA_TYPE_VIDEO;
    m_Info.m_pCodecPars->codec_id   = AV_CODEC_ID_RAWVIDEO;
    m_Info.m_pCodecPars->format     = format;
    m_Info.m_pCodecPars->width      = w;
    m_Info.m_pCodecPars->height     = h;
    m_Info.m_rFrameRate             = av_d2q(framerate, 1024);
    m_Info.m_rTimeBase              = AV_TIME_BASE_Q;
    m_nPrevPts                      = 0;
    m_nLastFrameTs                  = 0;
    m_nLastEncodeFrameTs            = 0;
    m_nLeftMcs                      = 0;
    m_eFrc                          = av_q2d(m_Info.m_rFrameRate) > 0 ? eFRC_CONST : eFRC_VARI;
    av_new_packet(&m_Pkt.av_pkt,
                  av_image_get_buffer_size(AVPixelFormat(format), w, h, 32));

    // Latency
    m_nLatencyStats = 0;
    m_bStartLatency = false;

    m_timeoutCount = 0;
    m_stop = false;

}

CIrrVideoDemux::~CIrrVideoDemux() {
    av_packet_unref(&m_Pkt.av_pkt);
}


int CIrrVideoDemux::getNumStreams() {
    return 1;
}

CStreamInfo* CIrrVideoDemux::getStreamInfo(int strIdx) {
    return &m_Info;
}

void CIrrVideoDemux::updateDynamicChangedFramerate(int framerate) {
    m_Info.m_rFrameRate = (AVRational){framerate, 1};
}

int CIrrVideoDemux::readPacket(IrrPacket *irrpkt) {
    int ret;

    TimeLog timelog("IRRB_CIrrVideoDemux_readPacket");
    ATRACE_CALL();

    std::unique_lock<std::mutex> lock(m_Lock);
#ifndef USE_QUICK
    std::chrono::microseconds time_to_wait(1000000);
#else
    std::chrono::microseconds time_to_wait(13000); //13ms
#endif

    ///< Wait for 1 frame's time if const FPS
    int64_t frame_mcs = av_rescale_q(1, av_inv_q(m_Info.m_rFrameRate), m_Info.m_rTimeBase);
    int64_t curr_mcs = av_gettime_relative();
    // Local variable 'targ_mcs' is never used
    //int64_t targ_mcs = 0;
    int64_t diff_mcs = 0;
    int64_t wait_mcs = 0;
    int64_t left_mcs = 0;

    if (!getRenderFpsEncFlag()) {
        if (m_eFrc == eFRC_CONST) {
            if (getLatencyOptFlag()) {
                diff_mcs = curr_mcs - m_nPrevPts;
                wait_mcs = frame_mcs - (diff_mcs % frame_mcs);

                // If previous frame is notified before waiting completed, add the left time
                // to total wait time this frame for compensation,  to keep encoding fps.
                wait_mcs += m_nLeftMcs;

                //
                // If there is a new frame ready during previous encoding time slot,
                // wait more time in this frame to avoid falling in always long latency case.
                //
                if (m_nPrevPts <= m_nLastFrameTs) {
                    wait_mcs += 3000;  // 3ms is experience value of encoding and writeback time
                    // printf("read packet : add extra wait, wait_mcs = %ld \n", wait_mcs);
                }
            }
            else {
                wait_mcs = m_nPrevPts - curr_mcs + frame_mcs;
                //printf("CIrrVideoDemux::readPacket : read packet : wait_mcs = %ld \n", wait_mcs);
                if (wait_mcs < 10000) {
                    wait_mcs = 10000; // If delay too long and the calculated waiting time is less than 10ms, set wait_mcs to 10ms.
                    //printf("CIrrVideoDemux::readPacket: read packet set wait_mcs to frame_mcs - 3000: wait_mcs = %ld \n", wait_mcs);
                }
            }

            time_to_wait = std::chrono::microseconds(wait_mcs);
        }
    }

    ///< Always output a packet no matter we are notified or not
    std::cv_status wait_status;
    if (getRenderFpsEncFlag()) {
        int iStartFrameRate = FIRST_START_ENCODE_FPS_DEFAULT;
        int iInitialFrameRate = (int)av_q2d(m_Info.m_rFrameRate);
        if (iInitialFrameRate > 0) {
            iStartFrameRate = iInitialFrameRate;
        }

        if (m_bFirstStartEncoding) {
            m_iFirstStartEncodingCnt = iStartFrameRate * 4;
            m_bFirstStartEncoding = false;
        }

        if (m_iFirstStartEncodingCnt > 0) {
            m_iFirstStartEncodingCnt--;
            int64_t time_wait_const = 1000000 / iStartFrameRate;
            if (m_timeoutCount < 30)
                time_wait_const = frame_mcs + frame_mcs/10;
            time_to_wait = std::chrono::microseconds(time_wait_const/10);

            for (int i = 0; i < 10; i++) {
                wait_status = m_cv.wait_for(lock, time_to_wait);
                if (wait_status != std::cv_status::timeout || m_stop) {
                    break;
                }
            }
            if (wait_status == std::cv_status::timeout) {
                m_nLastFrameTs = curr_mcs;
                m_timeoutCount++;
            }
        } else if (m_nLastEncodeFrameTs == m_nLastFrameTs) {
            int64_t time_wait_const = 1000000 / getMinFpsEnc();
            if (m_timeoutCount < 30)
                time_wait_const = frame_mcs + frame_mcs/10;
            time_to_wait = std::chrono::microseconds(time_wait_const/10);
            for (int i = 0; i < 10; i++) {
                wait_status = m_cv.wait_for(lock, time_to_wait);
                if (wait_status != std::cv_status::timeout || m_stop) {
                    break;
                }
            }
            if (wait_status == std::cv_status::timeout) {
                m_nLastFrameTs = curr_mcs;
                m_timeoutCount++;
            }
            else {
                m_timeoutCount = 0;
            }
        }
    }
    else {
        wait_status = m_cv.wait_for(lock, time_to_wait);

        if (getLatencyOptFlag()) {
            if (m_eFrc == eFRC_CONST) {
                if (wait_status == std::cv_status::timeout) {
                    m_nLeftMcs = 0;
                }
                else {
                    diff_mcs = av_gettime_relative() - m_nPrevPts;
                    if (left_mcs >= (frame_mcs + m_nLeftMcs)) {
                        left_mcs = 0;
                    }
                    else {
                        left_mcs = (frame_mcs + m_nLeftMcs) - diff_mcs;
                    }

                    m_nLeftMcs = (left_mcs < frame_mcs) ? left_mcs : 0;
                }

                // printf("read packet : m_eFrc = %d, wait_mcs = %ld, left_mcs = %ld, m_nLeftMcs = %ld \n", m_eFrc, wait_mcs, left_mcs, m_nLeftMcs);
            }
        }
    }

    if (m_nLatencyStats && m_bStartLatency) {
        if(m_Pkt.av_pkt.pts!=AV_NOPTS_VALUE){
            m_mProfTimer["pkt_latency"]->profTimerEnd("pkt_latency");
        }
    }

    ret = av_packet_ref(&irrpkt->av_pkt, &m_Pkt.av_pkt);
    irrpkt->display_ctrl = std::move(m_Pkt.display_ctrl);

    // Runtime Dump input
    if (mRuntimeWriter && mRuntimeWriter->getRuntimeWriterStatus() != RUNTIME_WRITER_STATUS::STOPPED) {
        auto pkt_data = std::make_shared<IORuntimeData>();
        if (CDemux::getVASurfaceFlag()) {
            uint32_t surfaceId;
            memcpy(&surfaceId, irrpkt->av_pkt.data, sizeof(uint32_t));
            pkt_data->va_surface_id = surfaceId;
            pkt_data->type = IORuntimeDataType::VAAPI_SURFACE;
        } else {
            pkt_data->data = irrpkt->av_pkt.data;
            pkt_data->size = irrpkt->av_pkt.size;
            pkt_data->type = IORuntimeDataType::SYSTEM_BLOCK;
        }
        pkt_data->width = m_Info.m_pCodecPars->width;
        pkt_data->height = m_Info.m_pCodecPars->height;
        pkt_data->format = IORuntimeWriter::avFormatToFourCC(m_Info.m_pCodecPars->format);

        mRuntimeWriter->submitRuntimeData(RUNTIME_WRITE_MODE::INPUT, pkt_data);
    }

    if (m_nLatencyStats && (m_nPrevPts>0)) {
        m_mProfTimer["pkt_round"]->profTimerEnd("pkt_round", m_nPrevPts);
    }

    irrpkt->av_pkt.pts = irrpkt->av_pkt.dts = m_nPrevPts = av_gettime_relative();
    m_nLastEncodeFrameTs = m_nLastFrameTs;
    //printf ("read packet : m_nPrePts = %ld, m_nLastEncodeFrameTs = %ld, latency = %ld \n", m_nPrevPts, m_nLastEncodeFrameTs, m_nPrevPts - m_nLastEncodeFrameTs);

    if (m_nLatencyStats && m_bStartLatency ) {
        m_Pkt.av_pkt.pts = AV_NOPTS_VALUE;
    }

    lock.unlock();

    return ret;
}

int CIrrVideoDemux::sendPacket(IrrPacket *pkt) {
    std::unique_lock<std::mutex> lock(m_Lock);

    TimeLog timelog("IRRB_CIrrVideoDemux_sendPacket");
    ATRACE_CALL();

    av_packet_unref(&m_Pkt.av_pkt);
    av_packet_move_ref(&m_Pkt.av_pkt, &pkt->av_pkt);
    // if m_Pkt.display_ctrl is not nullptr, the ctrl is not read. Keep it non-nullptr
    // to avoid missing ctrl SEI.
    if (pkt->display_ctrl != nullptr)
        m_Pkt.display_ctrl = std::move(pkt->display_ctrl);

    if (m_nLatencyStats) {
        if (!m_bStartLatency) {
            m_bStartLatency = true;
        }
        m_Pkt.av_pkt.pts = m_mProfTimer["pkt_latency"]->profTimerBegin();
    }

    int64_t curr_mcs = av_gettime_relative();
    int64_t diff1_mcs = 0;
    int64_t diff2_mcs = 0;

    int64_t frame_mcs = av_rescale_q(1, av_inv_q(m_Info.m_rFrameRate), m_Info.m_rTimeBase);

    diff1_mcs =  curr_mcs - m_nLastEncodeFrameTs;
    diff2_mcs =  m_nLastFrameTs - m_nPrevPts;
    /*
    printf("send packet : curr_mcs = %ld, m_nLastFrameTs = %ld, m_nLastEncodeFrameTs = %ld, m_nPrevPts = %ld, diff1_mcs = %ld, diff2_mcs = %ld, frame_mcs = %ld \n", \
            curr_mcs, m_nLastFrameTs, m_nLastEncodeFrameTs, m_nPrevPts, diff1_mcs, diff2_mcs, frame_mcs);
    */

    m_nLastFrameTs = curr_mcs;

    lock.unlock();
    if (!getRenderFpsEncFlag()) {
        if (getLatencyOptFlag()) {
            if (m_eFrc == eFRC_CONST) {

                // For constant fps app,  the interval between post is almost constant.
                // But since the readback time and various lock/unlock time is not constant,  the interval between sendPacket is not constant.
                // So we can't strictly compare it with frame time,  add 1ms time range here for workaround.
                // The better way is to record frame timestamp in post and pass the value to here.
                if (diff1_mcs >= (frame_mcs - 1000)) {
                    bool notify = true;

                    if ((diff2_mcs > 0) && (diff2_mcs < 3000)) {
                        notify = false;
                    }

                    if (notify) {
                        m_cv.notify_one();
                    }
                }
            }
            else {
                m_cv.notify_one();
            }
        }
        else {
            //m_cv.notify_one();
        }
    }
    else {
        m_cv.notify_one();
    }

    return 0;
}

int CIrrVideoDemux::setLatencyStats(int nLatencyStats) {
    std::lock_guard<std::mutex> lock(m_Lock);

    m_nLatencyStats = nLatencyStats;
    if (m_nLatencyStats) {
        if (m_mProfTimer.find("pkt_latency") == m_mProfTimer.end()) {
            m_mProfTimer["pkt_latency"] = new ProfTimer(true);
        }
        m_mProfTimer["pkt_latency"]->setPeriod(nLatencyStats);
        m_mProfTimer["pkt_latency"]->enableProf();

        if (m_mProfTimer.find("pkt_round") == m_mProfTimer.end()) {
            m_mProfTimer["pkt_round"] = new ProfTimer(true);
        }
        m_mProfTimer["pkt_round"]->setPeriod(nLatencyStats);
        m_mProfTimer["pkt_round"]->enableProf();
        m_mProfTimer["pkt_round"]->profTimerBegin();

    }else{
        if (m_bStartLatency) {
            if (m_mProfTimer.find("pkt_latency") != m_mProfTimer.end()) {
                m_mProfTimer["pkt_latency"]->profTimerReset("pkt_latency");
            }
        }

        if (m_mProfTimer.find("pkt_round") != m_mProfTimer.end()) {
            m_mProfTimer["pkt_round"]->profTimerReset("pkt_round");
        }

        m_bStartLatency = false;
    }

    return 0;
}
