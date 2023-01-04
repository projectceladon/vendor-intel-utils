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

#pragma once

#include <fstream>
#include <functional>
#include <condition_variable>
#include <thread>
#include <vector>
#include "safe_queue.h"
#include "utils/CTransLog.h"

template <int a, int b, int c, int d>
struct fourcc {
    enum { code = (a) | (b << 8) | (c << 16) | (d << 24) };
};

enum FourCC {
    FOURCC_NV12 = fourcc<'N', 'V', '1', '2'>::code,
    FOURCC_BGRA = fourcc<'B', 'G', 'R', 'A'>::code,
    FOURCC_BGRX = fourcc<'B', 'G', 'R', 'X'>::code,
    FOURCC_BGRP = fourcc<'B', 'G', 'R', 'P'>::code,
    FOURCC_BGR = fourcc<'B', 'G', 'R', ' '>::code,
    FOURCC_RGBA = fourcc<'R', 'G', 'B', 'A'>::code,
    FOURCC_RGBX = fourcc<'R', 'G', 'B', 'X'>::code,
    FOURCC_RGB = fourcc<'R', 'G', 'B', ' '>::code,
    FOURCC_RGBP = fourcc<'R', 'G', 'B', 'P'>::code,
    FOURCC_I420 = fourcc<'I', '4', '2', '0'>::code,
    FOURCC_YUV = fourcc<'Y', 'U', 'V', ' '>::code
};

enum IORuntimeDataType {
    ANY           = 0,
    VAAPI_SURFACE = 1,
    DMA_BUFFER    = 2,
    SYSTEM_IMAGE  = 3,
    SYSTEM_BLOCK  = 4,
    SYSTEM_BLOCK_COPY = 5,
};

struct BlockData {
    BlockData(uint8_t *p, uint32_t size) {
        vec_data = std::vector<uint8_t>(p, p + size);
    }

    std::vector<uint8_t>  vec_data;
};

struct IORuntimeData {
    IORuntimeDataType type;
    static const uint32_t MAX_PLANES_NUMBER = 4;
    union {
        uint8_t *planes[MAX_PLANES_NUMBER]; // if type==SYSTEM_IMAGE
        int dma_fd;                         // if type==DMA_BUFFER
        uint32_t va_surface_id;             // if type==VAAPI_SURFACE
        uint8_t *data;                      // if type==SYSTEM_BLOCK
    };
    std::unique_ptr<BlockData> blk;         // if type==SYSTEM_BLOCK_COPY

    int format; // FourCC
    uint32_t width;
    uint32_t height;
    uint32_t size;
    uint32_t stride[MAX_PLANES_NUMBER];
    uint32_t offset[MAX_PLANES_NUMBER];
    bool key_frame;
};

enum RUNTIME_WRITE_MODE {
    INPUT  = 0x1,
    OUTPUT = 0x2,
    INOUT  = INPUT | OUTPUT,
};

enum RUNTIME_WRITER_STATUS {
    STOPPED        = 0,
    INPUT_WRITING  = 1 << 0,
    OUTPUT_WRITING = 1 << 1,
    INOUT_WRITING  = INPUT_WRITING | OUTPUT_WRITING,
};

using IOData = std::shared_ptr<IORuntimeData>;
using OFStream = std::shared_ptr<std::ofstream>;

class IORuntimeWriter : public CTransLog
{
public:
    using Ptr = std::shared_ptr<IORuntimeWriter>;

    IORuntimeWriter(const int codec_id, const char *prefix = "./");
    IORuntimeWriter(const IORuntimeWriter&) = delete;
    IORuntimeWriter& operator=(const IORuntimeWriter&) = delete;
    virtual ~IORuntimeWriter();

    int startWriting(const RUNTIME_WRITE_MODE mode, const int frame_num = -1/* no limit */);

    int stopWriting(const RUNTIME_WRITE_MODE mode);

    RUNTIME_WRITER_STATUS getRuntimeWriterStatus();

    int submitRuntimeData(const RUNTIME_WRITE_MODE mode, IOData data);

    int changeCodecType(const int codec_id);

    static FourCC avFormatToFourCC(const int av_fmt);

private:
    enum STREAM_INDEX {
        INPUT_STREAM = 0,
        OUTPUT_STREAM,
        STREAM_NUMBER
    };

    int copy_image_to_file(IOData data, OFStream of);
    int copy_block_to_file(IOData data, OFStream of);
    int copy_surface_to_file(IOData data, OFStream of);

    void flush_data_queue(SafeQueue<IOData> &queue);

    int mCodecID = 0;

    std::string mPrefix;
    OFStream mStreams[STREAM_NUMBER];

    int mMaxFrameNumbers[STREAM_NUMBER] = { -1, -1 };

    SafeQueue<IOData> mDataQueues[STREAM_NUMBER];

    std::mutex _mutex;

    std::thread mThreads[STREAM_NUMBER];

    int mStatus = RUNTIME_WRITER_STATUS::STOPPED;
};

