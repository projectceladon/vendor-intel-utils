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

#include "core/CCallbackMux.h"

extern int write_file_output(unsigned char *data, size_t size);

CCallbackMux::CCallbackMux(void* opaque, void* opaque2, cbOpen pOpen,
                            cbWrite pWrite, cbWrite2 pWrite2, cbClose pClose,
                            cbCheckNewConn pCheckNewConn, cbSendMessage pSendMsg)
:   CTransLog(__func__), m_Opaque(opaque), m_Opaque2(opaque2), m_pOpen(pOpen),
    m_pWrite(pWrite), m_pWrite2(pWrite2), m_pClose(pClose),
    m_pCheckNewConn(pCheckNewConn), m_pSendMessage(pSendMsg)
{ }

CCallbackMux::~CCallbackMux() {}

int CCallbackMux::addStream(int, CStreamInfo* info) {
    int ret = 0;
    ///< TODO: Deal with multi-channel streams.
    if (m_bInited || info->m_pCodecPars->codec_type != AVMEDIA_TYPE_VIDEO) {
        Warn("Do not support multi-streams or non-video streams so far.\n");
        return AVERROR(EINVAL);
    }

    if (m_pOpen) {
        ret = m_pOpen(m_Opaque, info->m_pCodecPars->width, info->m_pCodecPars->height,
                      av_q2d(info->m_rFrameRate));
        if (ret < 0) {
            Error("Fail to call cb->open().\n");
            return ret;
        }
    }

    m_bInited = true;

    return 0;
}

//#define DUMP_VIDEO
#ifdef DUMP_VIDEO
    FILE *fptr = NULL;
    const char *file_name = "dump.264";
    static int encoded_frames = 1000;
    static int frame_idx = 0;
#endif
int CCallbackMux::write(AVPacket *pPkt) {
    int ret = 0;
    if (!pPkt)  ///< Do not support flush
        return 0;

    //check if set the env variable for allowing encode unconditionally.
    const char * encode_unconditionally = getenv("encode_unlimit");
    bool isEncodeUnconditionally = (encode_unconditionally && strncmp(encode_unconditionally, "1", strlen("1")) == 0);

    const char * encode_setting_by_env = getenv("enable_encode");
    bool isEncodeEnableByEnv = (encode_setting_by_env && strncmp(encode_setting_by_env, "1", strlen("1")) == 0);

    bool bAllowTransmit = getTransmissionAllowed ? getTransmissionAllowed() : false;

    //if not allow transmitting, but allow encode(in this case, it should be change from start->pause), call checkNewConn.
    if (!(bAllowTransmit || isEncodeEnableByEnv || isEncodeUnconditionally)) {
        checkNewConn();
    }

    if (m_pWrite && (bAllowTransmit || isEncodeEnableByEnv || isEncodeUnconditionally)) {
        ret = m_pWrite(m_Opaque, pPkt->data, static_cast<size_t>(pPkt->size), pPkt->flags);
        if(m_Opaque2 && m_pWrite2) {
            // There will be another connection when the m_Opaque2 is NOT NULL.
           m_pCheckNewConn(m_Opaque2);
        }

        /* Write data to output file in file dump mode */
        /* And it will exit when dump all the frames.  */
        if (m_pWriter) {
            int ret = m_pWriter->writeToOutputStream(reinterpret_cast<const char *>(pPkt->data), pPkt->size);
            // FIXME: don't call exit directly here
            if(ret != 0) {
                exit(0);
            }
        }

        /* Write data to file in runtime dump mode */
        if (m_pRuntimeWriter && m_pRuntimeWriter->getRuntimeWriterStatus() != RUNTIME_WRITER_STATUS::STOPPED) {
            auto pkt_data = std::make_shared<IORuntimeData>();

            pkt_data->type = IORuntimeDataType::SYSTEM_BLOCK_COPY;
            pkt_data->blk.reset(new BlockData(pPkt->data, pPkt->size));
            // pkt_data->data = pPkt->data;
            pkt_data->size = pPkt->size;
            pkt_data->key_frame = (pPkt->flags & AV_PKT_FLAG_KEY) ? true : false;

            m_pRuntimeWriter->submitRuntimeData(RUNTIME_WRITE_MODE::OUTPUT, pkt_data);
        }

        if (ret < 0) {
            Error("Fail to write back by calling cb->write(). EC %d.\n", ret);
            return ret;
        }
    }

#ifdef DUMP_VIDEO
    if (fptr == NULL && frame_idx == 0) {
        fptr = fopen(file_name, "wb");
        if (fptr == NULL) {
            printf("DUMP stream: Fail to open output file.\n");
        } else {
            printf("DUMP stream: Success to open output file.\n");
        }
    }
#endif

#ifdef DUMP_VIDEO
    if (fptr != NULL && pPkt != NULL && pPkt->size > 0) {
        if (frame_idx < encoded_frames) {
            if (fptr != NULL) {
                fwrite(pPkt->data, pPkt->size, 1, fptr);
                printf("DUMP stream: Success to wirte video packet. pPkt=%d, size=%d\n", frame_idx, pPkt->size);
            }
        } else {
            if (fptr != NULL) {
                fclose(fptr);
                fptr = NULL;
            }
        }

        frame_idx++;
        printf("DUMP data:\n");
        int size = pPkt->size;
        uint8_t *ptr = pPkt->data;
        for (int i = 0; i < size && i < 80; i++) {
            if (i % 15 == 0) printf("\n");
            printf("%02x  ", *(ptr + i));
        }
        printf("\n");
    }
#endif
    return ret;
}

int CCallbackMux::write(uint8_t *data, size_t len, int type) {
    if (!m_Opaque2 || !m_pWrite2)
        return -1;

    m_pWrite2(m_Opaque2, data, len, type);

    return 0;
}

int CCallbackMux::checkNewConn() {
    int clientNum = 0;
    if (m_pCheckNewConn) {
        if(m_Opaque) {
            clientNum = m_pCheckNewConn(m_Opaque);
        }
        if(m_Opaque2) {
            clientNum = m_pCheckNewConn(m_Opaque2);
        }
    }
    return clientNum;
}

int CCallbackMux::sendMessage(int msg, unsigned int value) {
    if (m_pSendMessage) {
        m_pSendMessage(m_Opaque, msg, value);
        m_pSendMessage(m_Opaque2, msg, value);
    }

    return 0;
}
