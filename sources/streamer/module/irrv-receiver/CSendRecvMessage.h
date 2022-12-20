// Copyright (C) 2017-2022 Intel Corporation
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

#ifndef CSENDRECVMESSAGE_H
#define CSENDRECVMESSAGE_H

#include <condition_variable>
#include <mutex>
#include <thread>

#include "sock_client.h"
#include "irrv_protocol.h"

enum nal_unit_type_e
{
    NAL_UNKNOWN = 0,
    NAL_SLICE = 1,
    NAL_SLICE_DPA = 2,
    NAL_SLICE_DPB = 3,
    NAL_SLICE_DPC = 4,
    NAL_SLICE_IDR = 5,    /* ref_idc != 0 */
    NAL_SEI = 6,    /* ref_idc == 0 */
    NAL_SPS = 7,
    NAL_PPS = 8
    /* ref_idc == 0 for 6,9,10,11,12 */
};

/**
 * Region Of Interest(ROI).
 * This specifies the per-frame control for ROI.
 */
typedef struct AVRoI2 {
    /**
     *  x position of ROI region.
     */
    int16_t x;
    /**
     *  y position of ROI region.
     */
    int16_t y;
    /**
     * width of ROI region.
     */
    uint16_t width;
    /**
     * height of ROI region.
     */
    uint16_t  height;
    /**
     * roi_value specifies ROI delta QP or ROI priority.
     * refer to VAEncMiscParameterBufferROI in libva header file va.h.
     */
    int8_t   roi_value;
} AVRoI2;

#define MAX_ROI_NUM  8

const char* irrv_ctrl_type(int type);

class CSendRecvMessage {
public:

    CSendRecvMessage(bool startEncoderImmediately = false);

    ~CSendRecvMessage();

    /* Control functions */
    /**
     * Start the process. Throws an exception on failure.
     */
    void start();
    /**
     * Force to stop the procedure.
     */
    void stop();

    inline bool ready() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ready;
    }

    template<class Clock, class Duration>
    inline bool ready(const std::chrono::time_point<Clock, Duration>& timeout_time) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_until(lock, timeout_time, [this]{ return m_ready; });
    }

    int irrv_op(irrv_vctrl_t& ctrl);

    template<irrv_ctrl_type_t CtrlType>
    inline int irrv_op() {
        irrv_vctrl_t ctrl{};
        ctrl.ctrl_type = CtrlType;
        ga_logger(Severity::INFO, "irrv-receiver: %s\n", irrv_ctrl_type(CtrlType));
        return irrv_op(ctrl);
    }

    template<irrv_ctrl_type_t CtrlType>
    inline int irrv_op(unsigned int value) {
        irrv_vctrl_t ctrl{};
        ctrl.ctrl_type = CtrlType;
        ctrl.value = value;
        ga_logger(Severity::INFO, "irrv-receiver: %s: value=%d\n", irrv_ctrl_type(CtrlType), value);
        return irrv_op(ctrl);
    }

    inline int irrv_op(std::vector<irrv_vctrl_event_t>& ctrls);

    inline int irrv_set_encodestart()
    { return irrv_op<IRRV_CTRL_START>(); }

    inline int irrv_set_encodepause()
    { return irrv_op<IRRV_CTRL_PAUSE>(); }

    inline int irrv_set_encodestop()
    { return irrv_op<IRRV_CTRL_STOP>(); }

    // TODO: ICR actually handles ctrl.value somehow passing it to
    // pipeline components. As of now we use ctrl.value=1, need to
    // check what the value ultimately is used for.
    inline int irrv_set_keyframe()
    { return irrv_op<IRRV_CTRL_KEYFRAME_SETTING>(1); }

    inline int irrv_set_bitrate(unsigned int value)
    { return irrv_op<IRRV_CTRL_BITRATE_SETTING>(value); }

    inline int irrv_set_framerate(unsigned int value)
    { return irrv_op<IRRV_CTRL_FRAMERATE_SETTING>(value); }

    inline int irrv_set_max_frame_size(unsigned int size)
    { return irrv_op<IRRV_CTRL_MAXFRAMESIZE_SETTING>(size); }

    inline int irrv_set_qp(unsigned int qp)
    { return irrv_op<IRRV_CTRL_QP_SETTING>(qp); }

    inline int irrv_set_gop_size(unsigned int gop_size)
    { return irrv_op<IRRV_CTRL_GOP_SETTING>(gop_size); }

//    inline int irrv_set_alpha_transmission_flag(unsigned int flag)
//    { return irrv_op<IRRV_CTRL_QP_SETTING>(qp); }

    inline int irrv_set_dump_start()
    { return irrv_op<IRRV_CTRL_DUMP_START>(); }

    inline int irrv_set_dump_stop()
    { return irrv_op<IRRV_CTRL_DUMP_STOP>(); }

    inline int irrv_set_input_dump_start()
    { return irrv_op<IRRV_CTRL_INPUT_DUMP_START>(); }

    inline int irrv_set_input_dump_stop()
    { return irrv_op<IRRV_CTRL_INPUT_DUMP_STOP>(); }

    inline int irrv_set_output_dump_start()
    { return irrv_op<IRRV_CTRL_OUTPUT_DUMP_START>(); }

    inline int irrv_set_output_dump_stop()
    { return irrv_op<IRRV_CTRL_OUTPUT_DUMP_STOP>(); }

    inline int irrv_set_dump_frames(unsigned int frame_number)
    { return irrv_op<IRRV_CTRL_DUMP_FRAMES>(frame_number); }

    inline int irrv_set_screen_capture_start(unsigned int capture_interval, unsigned int quality_factor);

    inline int irrv_set_screen_capture_stop()
    { return irrv_op<IRRV_CTRL_SCREEN_CAPTURE_STOP>(); }

    inline int irrv_set_alpha_transmission_flag(unsigned int flags);

    inline int irrv_change_resolution(unsigned int new_width, unsigned int new_height);

    inline int irrv_set_rolling_intra_refresh(unsigned int type, unsigned int cycle_size, unsigned int qp_delta);

    inline int irrv_set_min_max_qp(unsigned int min_qp, unsigned int max_qp);

    inline int irrv_set_sei(unsigned int sei_type, unsigned int sei_id);

    inline int irrv_set_region_of_interest(int roi_num, AVRoI2 roi_para[]);

    inline int irrv_set_client_feedback(unsigned int delay, unsigned int size);

    void pipe_set_client_feedback(uint32_t delay, uint32_t size);

    void pipe_set_resolution_change(uint32_t width, uint32_t height);

    void pipe_set_video_alpha(uint32_t action);
private:
    void run();

    void recv_es_stream();

    bool irrv_sock_client_init();

    void irrv_send_authentication();

    void irrv_sock_client_disconnect();

    bool private_pipe_connect();

    void private_pipe_disconnect();
private:
    volatile bool m_ready = false;
    volatile bool m_stop = false;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<uint8_t> m_frameData;
    int m_client = -1; // IRRV socket fd
    FILE* m_encout = nullptr;
    bool  m_bEnableAlphaTransmission;

    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_format;
    bool m_bUnexpectedDisconnect = false;
    bool m_bStartEncoderImmediately = false;

    int m_privatePipe = -1;
};

#endif /* CSENDRECVMESSAGE_H */
