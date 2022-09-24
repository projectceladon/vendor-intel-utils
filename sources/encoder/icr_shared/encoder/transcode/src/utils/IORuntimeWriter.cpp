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

#include <ctime>
#include <chrono>
#include <sstream>
#include <assert.h>

extern "C" {
#include <libavcodec/avcodec.h>
}
#include "core/CVAAPIDevice.h"
#include "utils/IORuntimeWriter.h"

#define VA_CALL(_FUNC)                                                                \
    {                                                                                 \
        VAStatus _status = _FUNC;                                                     \
        if (_status != VA_STATUS_SUCCESS) {                                           \
            Error("%s  failed, sts=%d :%s\n", #_FUNC, _status, vaErrorStr(_status));  \
            return -1;                                                                \
        }                                                                             \
    }

FourCC IORuntimeWriter::avFormatToFourCC(const int av_fmt)
{
    if (av_fmt == AV_PIX_FMT_RGBA)
        return FOURCC_RGBA;

    return FOURCC_RGBA;
}

int IORuntimeWriter::copy_image_to_file(IOData data, OFStream of)
{
    // TODO
    return 0;
}

int IORuntimeWriter::copy_block_to_file(IOData data, OFStream of)
{
    if (data->type != SYSTEM_BLOCK)
        return -1;

    of->write(reinterpret_cast<const char*>(data->data), data->size);
    return 0;
}

int IORuntimeWriter::copy_surface_to_file(IOData data, OFStream of)
{
    static VADisplay va_dpy = nullptr;
    if (va_dpy == nullptr) {
        AVBufferRef *pHwDev = CVAAPIDevice::getInstance()->getVaapiDev();
        if (nullptr != pHwDev) {
            AVHWDeviceContext *device_ctx = (AVHWDeviceContext*)pHwDev->data;
            AVVAAPIDeviceContext *hwctx = (AVVAAPIDeviceContext*)device_ctx->hwctx;
            va_dpy = hwctx->display;
        }
    }

    if (va_dpy == nullptr) {
        Error("Cannot get a valid va display!\n");
        return -1;
    }

    if (data->type != VAAPI_SURFACE)
        return -1;

    void *address = nullptr;
    VAImage va_image;
    VAImageFormat image_format;
    image_format.fourcc         = data->format;
    image_format.byte_order     = VA_LSB_FIRST;
    image_format.bits_per_pixel = 32;

    VA_CALL(vaCreateImage(va_dpy, &image_format, data->width, data->height, &va_image));
    VA_CALL(vaGetImage(va_dpy, data->va_surface_id, 0, 0, data->width, data->height, va_image.image_id));
    VA_CALL(vaMapBuffer(va_dpy, va_image.buf, &address));

    uint32_t bytes_per_pixel = image_format.bits_per_pixel / 8;
    for (uint32_t i = 0; i < va_image.num_planes; i++) {
        uint8_t *plane = (uint8_t *)address + va_image.offsets[i];
        uint32_t stride = va_image.pitches[i];

        for (uint32_t h = 0; h < va_image.height; h++) {
            of->write(reinterpret_cast<const char*>(plane + stride * h), va_image.width * bytes_per_pixel);
        }
    }

    VA_CALL(vaUnmapBuffer(va_dpy, va_image.buf));
    VA_CALL(vaDestroyImage(va_dpy, va_image.image_id));
    return 0;
}

void IORuntimeWriter::flush_data_queue(SafeQueue<IOData> &queue)
{
    if (queue.empty())
        return;

    while (!queue.empty())
        queue.pop();
}

IORuntimeWriter::IORuntimeWriter(const int codec_id, const char* prefix) :
    CTransLog("IORuntimeWriter")
    ,mCodecID(codec_id)
    ,mPrefix(prefix)
{
    Verbose("Create a new iostream writer.\n");
}

IORuntimeWriter::~IORuntimeWriter()
{
    stopWriting(INOUT);

    flush_data_queue(mDataQueues[INPUT_STREAM]);

    flush_data_queue(mDataQueues[OUTPUT_STREAM]);
}

int IORuntimeWriter::startWriting(const RUNTIME_WRITE_MODE mode, const int frame_num)
{
    std::unique_lock<std::mutex> lock(_mutex);

    Verbose("io runtimewriter start writing... mode:%d, frame:%d\n", mode, frame_num);

    // Already started
    if (mStatus == INOUT_WRITING)
        return 0;

    const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();

    const std::time_t t_c = std::chrono::system_clock::to_time_t(now);

    char foo[32] = { 0 };
    auto local_tm = std::localtime(&t_c);
    if (local_tm != nullptr)
        strftime(foo, sizeof(foo), "%Y-%m-%d_%H-%M-%S", local_tm);

    std::string current_time(foo);

    // Start input writing
    if (!(mStatus & INPUT_WRITING) && (mode & INPUT)) {
        std::string file_name = mPrefix + "input" + "_" + current_time + ".rgba";
        auto os = std::make_shared<std::ofstream>(file_name);
        assert (os.get() != nullptr && "Expected a valid file stream");

        if (!os->good()) {
            Error("Open file:%s failed!", file_name.c_str());
            os->close();
            return -1;
        }

        flush_data_queue(mDataQueues[INPUT_STREAM]);

        // trigger input thread
        mThreads[INPUT_STREAM] = std::thread([this, os, frame_num]() {
            int count_written = 0;
            Verbose("io runtimewriter input thread function\n");
            while (1) {
                int ret = -1;
                auto data_in = this->mDataQueues[INPUT_STREAM].pop();

                if (data_in->type == SYSTEM_BLOCK && data_in->data == nullptr)
                    break;

                switch (data_in->type) {
                case SYSTEM_IMAGE:
                    ret = copy_image_to_file(data_in, os);
                    break;

                case VAAPI_SURFACE:
                    ret = copy_surface_to_file(data_in, os);
                    break;

                case SYSTEM_BLOCK:
                    ret = copy_block_to_file(data_in, os);
                    break;

                default:
                    break;
                }

                if (ret < 0)
                    continue;

                if (frame_num > 0 && ++count_written >= frame_num)
                    break;
            }

            // finished
            os->close();
            Verbose("io runtimewriter input thread exit\n");
        });

        mStreams[INPUT_STREAM] = os;
        mStatus |= INPUT_WRITING;
    }

    // Start output writing
    if (!(mStatus & OUTPUT_WRITING) && (mode & OUTPUT)) {
        std::string ext;

        if (mCodecID == AV_CODEC_ID_H264)
            ext = ".264";
        else if (mCodecID == AV_CODEC_ID_H265)
            ext = ".265";
        else if (mCodecID == AV_CODEC_ID_AV1)
            ext = ".ivf";

        std::string file_name = mPrefix + "output" + "_" + current_time + ext;
        auto os = std::make_shared<std::ofstream>(file_name);
        assert (os.get() != nullptr && "Expected a valid file stream");

        if (!os->good()) {
            Error("Open file:%s failed!", file_name.c_str());
            os->close();
            return -1;
        }

        flush_data_queue(mDataQueues[OUTPUT_STREAM]);

        // trigger output thread
        mThreads[OUTPUT_STREAM] = std::thread([this, os, frame_num]() {
            int count_written = 0;

            Verbose("io runtimewriter output thread function\n");
            while (1) {
                auto out_data = this->mDataQueues[OUTPUT_STREAM].pop();

                if (out_data->type == SYSTEM_BLOCK && out_data->data == nullptr)
                    break;

                // the 1st packet must contain key frame data
                if (count_written == 0 && !out_data->key_frame) {
                    Debug("not a key frame  %d %d \n", count_written, out_data->key_frame);
                    continue;
                }

                if (frame_num < 0 || count_written < frame_num) {
                    if (out_data->type == SYSTEM_BLOCK) {
                        os->write(reinterpret_cast<const char*>(out_data->data), out_data->size);
                    } else if (out_data->type == SYSTEM_BLOCK_COPY) {
                        os->write(reinterpret_cast<const char*>(&out_data->blk->vec_data[0]), out_data->size);
                    }
                    count_written++;
                }

                if (frame_num > 0 && count_written >= frame_num)
                    break;
            }
            os->close();
            Verbose("io runtimewriter output thread exit\n");
        });

        mStreams[OUTPUT_STREAM] = os;
        mStatus |= OUTPUT_WRITING;
    }

    return 0;
}

int IORuntimeWriter::stopWriting(const RUNTIME_WRITE_MODE mode)
{
    std::unique_lock<std::mutex> lock(_mutex);

    if (mStatus == STOPPED)
        return 0;

    Verbose("io runtimewriter stop writing... %d %d\n", mode, mStatus);

    if ((mode & INPUT) && (mStatus & INPUT_WRITING)) {
        if (mThreads[INPUT_STREAM].joinable()) {
            // push am empty data to inform the thread stop
            IOData _empty_data = std::make_shared<IORuntimeData>();
            assert (_empty_data.get() != nullptr && "Expected a valid IORuntimeData");
            _empty_data->type = SYSTEM_BLOCK;
            _empty_data->data = nullptr;
            mDataQueues[INPUT_STREAM].push(_empty_data);

            mThreads[INPUT_STREAM].join();
        }
        mStatus &= ~INPUT_WRITING;
    }

    if ((mode & OUTPUT) && (mStatus & OUTPUT_WRITING)) {
        if (mThreads[OUTPUT_STREAM].joinable()) {
            // push am empty data to inform the thread stop
            IOData _empty_data = std::make_shared<IORuntimeData>();
            assert (_empty_data.get() != nullptr && "Expected a valid IORuntimeData");
            _empty_data->type = SYSTEM_BLOCK;
            _empty_data->data = nullptr;
            mDataQueues[OUTPUT_STREAM].push(_empty_data);

            mThreads[OUTPUT_STREAM].join();
        }
        mStatus &= ~OUTPUT_WRITING;
    }

    return 0;
}

RUNTIME_WRITER_STATUS IORuntimeWriter::getRuntimeWriterStatus()
{
    std::unique_lock<std::mutex> lock(_mutex);

    return (RUNTIME_WRITER_STATUS)mStatus;
}

int IORuntimeWriter::changeCodecType(const int codec_id)
{
    std::unique_lock<std::mutex> lock(_mutex);

    mCodecID = codec_id;

    return 0;
}

int IORuntimeWriter::submitRuntimeData(const RUNTIME_WRITE_MODE mode, std::shared_ptr<IORuntimeData> data)
{
    std::unique_lock<std::mutex> lock(_mutex);

    if (mStatus == STOPPED)
        return 0;

    if (mode != INPUT && mode != OUTPUT) {
        Warn("Not a valid mode value: %d\n", mode);
        return -1;
    }

    if (mode == INPUT && (mStatus & INPUT_WRITING))
        mDataQueues[INPUT_STREAM].push(data);

    if (mode == OUTPUT && (mStatus & OUTPUT_WRITING))
        mDataQueues[OUTPUT_STREAM].push(data);

    return 0;
}
