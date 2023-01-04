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

#include <map>
#include "ffmpeg_compat.h"
#include "CFFMux.h"

using namespace std;


//#define DUMP_VIDEO
#ifdef DUMP_VIDEO
FILE *fptr_rtmp = NULL;
const char *file_name_rtmp_dump = "dump-rtmp.264";
static int encoded_frames = 1000;
static int frame_idx = 0;
#endif

CFFMux::CFFMux(const char *url, const char *format) : CTransLog(__func__) {
    int ret;
    AVCompat_AVOutputFormat* oFmt = av_guess_format(format, url, nullptr);

    m_sUrl = url;
    m_bInited = false;
    m_pFmt = nullptr;

    if (!oFmt) {
        Warn("Fail to find a %s Muxer, try to set the mpegts muxer for %s.\n", format, url);
        oFmt = av_guess_format("mpegts", url, nullptr);
        if (!oFmt) {
            Error("Fail to find a suit Muxer for %s.\n", url);
            return;
        }
    }

    ret = avformat_alloc_output_context2(&m_pFmt, oFmt, nullptr, url);
    if (!m_pFmt || ret != 0) {
        Error("Fail to alloc Mux context.Msg: %s\n", ErrToStr(ret).c_str());
        return;
    }

    if (!(oFmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_pFmt->pb, url, AVIO_FLAG_WRITE);
        if (ret != 0) {
            avformat_free_context(m_pFmt);
            Error("Fail to open corresponding file. Error %s\n", ErrToStr(ret).c_str());
            return;
        }
    }
}

CFFMux::~CFFMux() {
    if (m_pFmt) {
        for (unsigned id = 0; id < m_pFmt->nb_streams; id++)
            Info("Total written frames on stream[%d]: %d.\n",
                 id, m_pFmt->streams[id]->nb_frames);
        av_write_trailer(m_pFmt);
        if (m_pFmt->pb)
            avio_close(m_pFmt->pb);
        avformat_free_context(m_pFmt);
    }
}

int CFFMux::addStream(int idx, CStreamInfo *info) {
    int ret = 0;
    AVStream *st;

    if (!m_pFmt || m_mIdxs.find(idx) != m_mIdxs.end())
        return AVERROR(EINVAL);

    st = avformat_new_stream(m_pFmt, nullptr);
    if (!st) {
        Error("Fail to alloc stream context.\n");
        return AVERROR(ENOMEM);
    }

    ret = avcodec_parameters_copy(st->codecpar, info->m_pCodecPars);
    if (ret < 0) {
        Error("Fail to copy stream infos. Msg: %s\n", ErrToStr(ret).c_str());
        return ret;
    }
    st->codecpar->codec_tag = 0;
    st->time_base = info->m_rTimeBase;

    m_mIdxs[idx] = st->index;
    m_mInfo[idx] = *info;

    return 0;
}

int CFFMux::write(AVPacket* pkt) {
    unsigned idx = pkt->stream_index;
    AVStream *st = m_pFmt->streams[idx];

    if (m_mIdxs.find(idx) == m_mIdxs.end()) {
        Error("No stream %d found.\n", idx);
        return AVERROR(EINVAL);
    }

    if (!m_bInited) {
        int ret = avformat_write_header(m_pFmt, nullptr);
        if (ret < 0) {
            Error("Fail to init muxers. Msg: %s\n", ErrToStr(ret).c_str());
            return ret;
        }
        av_dump_format(m_pFmt, 0, m_sUrl.c_str(), 1);
        m_bInited = true;
    }
    pkt->stream_index = m_mIdxs.at(idx);

    Verbose("Mux <- Encoder[%d]: dts = %s, time %s\n", idx, TsToStr(pkt->dts).c_str(),
            TsToTimeStr(pkt->dts, m_mInfo[idx].m_rTimeBase.num, m_mInfo[idx].m_rTimeBase.den).c_str());

    av_packet_rescale_ts(pkt, m_mInfo[idx].m_rTimeBase, st->time_base);

#ifdef DUMP_VIDEO
    if (fptr_rtmp == NULL && frame_idx == 0) {
        fptr_rtmp = fopen(file_name_rtmp_dump, "wb");
        if (fptr_rtmp == NULL) {
            printf("DUMP stream rtmp: Fail to open output file.\n");
        }
        else {
            printf("DUMP stream rtmp: Success to open output file.\n");
        }
    }
#endif

#ifdef DUMP_VIDEO
    if (fptr_rtmp != NULL && pkt != NULL && pkt->size > 0) {
        if (frame_idx < encoded_frames) {
            if (fptr_rtmp != NULL) {
                fwrite(pkt->data, pkt->size, 1, fptr_rtmp);
                printf("DUMP stream rtmp: Success to wirte video packet. pkt=%d, size=%d\n", frame_idx, pkt->size);
            }
        }
        else {
            if (fptr_rtmp != NULL) {
                fclose(fptr_rtmp);
                fptr_rtmp = NULL;
            }
        }

        frame_idx++;
        printf("DUMP data:\n");
        int size = pkt->size;
        uint8_t *ptr = pkt->data;
        for (int i = 0; i < size && i < 80; i++) {
            if (i % 15 == 0) printf("\n");
            printf("%02x  ", *(ptr + i));
        }
        printf("\n");
    }
#endif

    return av_interleaved_write_frame(m_pFmt, pkt);
}
