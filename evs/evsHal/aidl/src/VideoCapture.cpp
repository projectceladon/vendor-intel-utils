/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VideoCapture.h"
#include <vector>
#include <string.h>

#include <android-base/logging.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <iomanip>

// NOTE:  This developmental code does not properly clean up resources in case of failure
//        during the resource setup phase.  Of particular note is the potential to leak
//        the file descriptor.  This must be fixed before using this code for anything but
//        experimentation.
bool VideoCapture::open(const char* deviceName, const int32_t width, const int32_t height) {
    // If we want a polling interface for getting frames, we would use O_NONBLOCK
    mDeviceFd = ::open(deviceName, O_RDWR| O_NONBLOCK, 0);
    if (mDeviceFd < 0) {
        PLOG(ERROR) << "failed to open device " << deviceName;
        return false;
    }

    v4l2_capability caps;
    {
        int result = ioctl(mDeviceFd, VIDIOC_QUERYCAP, &caps);
        if (result < 0) {
            PLOG(ERROR) << "failed to get device caps for " << deviceName;
            return false;
        }
    }

    // Verify we can use this device for video capture
    if (!(caps.capabilities & (V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_CAPTURE)) ||
        !(caps.capabilities & V4L2_CAP_STREAMING)) {
        // Can't do streaming capture.
        LOG(ERROR) << "Streaming capture not supported by " << deviceName;
        return false;
    }

    uint32_t requestWidth = 0;
    uint32_t requestHeight = 0;
    mBufferType = (caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? V4L2_BUF_TYPE_VIDEO_CAPTURE :
                                                                 V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    // Report device properties
    LOG(INFO) << "Open Device: " << deviceName << " (fd = " << mDeviceFd << ")";
    LOG(DEBUG) << "  Driver: " << caps.driver;
    LOG(DEBUG) << "  Card: " << caps.card;
    LOG(DEBUG) << "  Version: " << ((caps.version >> 16) & 0xFF) << "."
               << ((caps.version >> 8) & 0xFF) << "." << (caps.version & 0xFF);
    LOG(DEBUG) << "  All Caps: " << std::hex << std::setw(8) << caps.capabilities;
    LOG(DEBUG) << "  Dev Caps: " << std::hex << caps.device_caps;

    // Enumerate the available capture formats (if any)
    LOG(DEBUG) << "Supported capture formats:";
    v4l2_fmtdesc formatDescriptions;
    formatDescriptions.type = mBufferType;
    for (int i = 0; true; i++) {
        formatDescriptions.index = i;
        if (ioctl(mDeviceFd, VIDIOC_ENUM_FMT, &formatDescriptions) == 0) {
            LOG(DEBUG) << "  " << std::setw(2) << i << ": " << formatDescriptions.description << " "
                       << std::hex << std::setw(8) << formatDescriptions.pixelformat << " "
                       << std::hex << formatDescriptions.flags;
            // auto detect USB camera resolution
            struct v4l2_frmsizeenum frmsize;
            frmsize.pixel_format = formatDescriptions.pixelformat;
            frmsize.index = 0;
            while (ioctl(mDeviceFd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
                struct v4l2_frmivalenum frmival;
                memset(&frmival, 0, sizeof(frmival));
                frmival.pixel_format = formatDescriptions.pixelformat;
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    frmival.width = frmsize.discrete.width;
                    frmival.height = frmsize.discrete.height;
                    while (ioctl(mDeviceFd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
                        if ((frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) &&
                            (1.0 * frmival.discrete.denominator / frmival.discrete.numerator >
                             29.0) &&
                            (requestWidth * requestHeight) <
                                frmsize.discrete.width * frmsize.discrete.height) {
                            requestWidth = frmsize.discrete.width;
                            requestHeight = frmsize.discrete.height;
                        }
                        frmival.index++;
                    }
               } else {
                   LOG(INFO) << "Stepwise: step_width=" << frmsize.stepwise.step_width<< " step_height=" << frmsize.stepwise.step_height;
                   LOG(INFO) << "min_width = " << frmsize.stepwise.min_width << " min_height=" << frmsize.stepwise.min_height;
                   LOG(INFO) << "max_width = " << frmsize.stepwise.max_width << " max_height=" << frmsize.stepwise.max_height;
                   requestWidth = frmsize.stepwise.min_width;
                   requestHeight = frmsize.stepwise.min_height;

                }
                frmsize.index++;
            }
        } else {
            // No more formats available
            break;
        }
    }

    // Set our desired output format
    v4l2_format format;
    format.type = mBufferType;
    if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_UYVY;
        format.fmt.pix_mp.width = width;
        format.fmt.pix_mp.height = height;
        format.fmt.pix_mp.num_planes = 1;
        format.fmt.pix_mp.plane_fmt[0].bytesperline = width * 2;
        format.fmt.pix_mp.plane_fmt[0].sizeimage = width * height * 2;
    } else if (strcmp((char*)caps.driver, "virtio-camera") == 0) {
            LOG(INFO) << "Virtio-camera";
            format.type = V4L2_CAP_VIDEO_CAPTURE;
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
            format.fmt.pix.width = requestWidth > 0 ? requestWidth : 1280;
            format.fmt.pix.height = requestHeight > 0 ? requestHeight : 960;
    } else {
        format.type = V4L2_CAP_VIDEO_CAPTURE;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        format.fmt.pix.width = requestWidth > 0 ? requestWidth : width;
        format.fmt.pix.height = requestHeight > 0 ? requestHeight : height;
    }

    if (ioctl(mDeviceFd, VIDIOC_S_FMT, &format) < 0) {
        PLOG(ERROR) << "VIDIOC_S_FMT failed";
    }

    // Report the current output format
    if (ioctl(mDeviceFd, VIDIOC_G_FMT, &format) == 0) {
        if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            mFormat = format.fmt.pix_mp.pixelformat;
            mWidth = format.fmt.pix_mp.width;
            mHeight = format.fmt.pix_mp.height;
            mStride = format.fmt.pix_mp.plane_fmt[0].bytesperline;
        } else {
            mFormat = format.fmt.pix.pixelformat;
            mWidth = format.fmt.pix.width;
            mHeight = format.fmt.pix.height;
            mStride = format.fmt.pix.bytesperline;
        }

        LOG(INFO) << "Current output format:  "
                  << "fmt=0x" << std::hex << mFormat << ", " << std::dec << mWidth << " x "
                  << mHeight << ", pitch=" << mStride;
    } else {
        PLOG(ERROR) << "VIDIOC_G_FMT failed";
        return false;
    }

    // Make sure we're initialized to the STOPPED state
    mRunMode = STOPPED;
    mFrames.clear();

    // Ready to go!
    return true;
}

void VideoCapture::close() {
    LOG(DEBUG) << __FUNCTION__;
    // Stream should be stopped first!
    assert(mRunMode == STOPPED);

    if (isOpen()) {
        LOG(DEBUG) << "closing video device file handle " << mDeviceFd;
        ::close(mDeviceFd);
        mDeviceFd = -1;
    }
}

bool VideoCapture::startStream(std::function<void(VideoCapture*, imageBuffer*, void*)> callback) {
    // Set the state of our background thread
    int prevRunMode = mRunMode.fetch_or(RUN);
    if (prevRunMode & RUN) {
        // The background thread is already running, so we can't start a new stream
        LOG(ERROR) << "Already in RUN state, so we can't start a new streaming thread";
        return false;
    }

    // Tell the L4V2 driver to prepare our streaming buffers
    v4l2_requestbuffers bufrequest;
    bufrequest.type = mBufferType;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = BUFFER_COUNT;
    if (ioctl(mDeviceFd, VIDIOC_REQBUFS, &bufrequest) < 0) {
        PLOG(ERROR) << "VIDIOC_REQBUFS failed";
        return false;
    }

    mNumBuffers = BUFFER_COUNT;
    mBufferInfos = std::make_unique<v4l2_buffer[]>(mNumBuffers);
    mPixelBuffers = std::make_unique<void*[]>(mNumBuffers);

    for (int i = 0; i < mNumBuffers; ++i) {
        // Get the information on the buffer that was created for us
        memset(&mBufferInfos[i], 0, sizeof(v4l2_buffer));
        mBufferInfos[i].type = mBufferType;
        mBufferInfos[i].memory = V4L2_MEMORY_MMAP;
        mBufferInfos[i].index = i;

        if (mBufferInfos[i].type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            std::vector<struct v4l2_plane> planes;
            planes.resize(VIDEO_PLANES);
            mBufferInfos[i].m.planes = planes.data();
            mBufferInfos[i].length = planes.size();
        }

        if (ioctl(mDeviceFd, VIDIOC_QUERYBUF, &mBufferInfos[i]) < 0) {
            PLOG(ERROR) << "VIDIOC_QUERYBUF failed";
            return false;
        }

        uint32_t memOffset = mBufferInfos[i].type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ?
                                 mBufferInfos[i].m.planes[0].m.mem_offset :
                                 mBufferInfos[i].m.offset;
        LOG(DEBUG) << "Buffer description:";
        LOG(INFO) << "  offset: " << mBufferInfos[i].m.planes[0].m.mem_offset;
        LOG(DEBUG) << "  length: " << mBufferInfos[i].length;
        LOG(DEBUG) << "  flags : " << std::hex << mBufferInfos[i].flags;
        if (mBufferInfos[i].type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
            LOG(INFO) << "  size : " << std::hex << mBufferInfos[i].m.planes[0].length;

        mBufferSize = (mBufferInfos[i].type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ?
                          mBufferInfos[i].m.planes[0].length :
                          mBufferInfos[i].length;
        std::vector<void*> mappedBuffer;
        mappedBuffer.resize(VIDEO_PLANES);
        for (uint32_t plane = 0; plane < VIDEO_PLANES; plane++) {
            mappedBuffer[plane] =
                mmap(NULL, mBufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, mDeviceFd, memOffset);

            if (mappedBuffer[plane] == MAP_FAILED) {
                PLOG(ERROR) << "mmap() failed";
                return false;
            }

            memset(mappedBuffer[plane], 0, mBufferSize);
        }
        mPixelBuffers[i] = mappedBuffer[0];
        LOG(INFO) << "Buffer mapped at " << mPixelBuffers[i];

        // Queue the first capture buffer
        if (ioctl(mDeviceFd, VIDIOC_QBUF, &mBufferInfos[i]) < 0) {
            PLOG(ERROR) << "VIDIOC_QBUF failed";
            return false;
        }
    }

    // Start the video stream
    const int type = mBufferType;
    if (ioctl(mDeviceFd, VIDIOC_STREAMON, &type) < 0) {
        PLOG(ERROR) << "VIDIOC_STREAMON failed";
       // return false;
    }

    // Remember who to tell about new frames as they arrive
    mCallback = callback;

    // Fire up a thread to receive and dispatch the video frames
    mCaptureThread = std::thread([this]() { collectFrames(); });

    LOG(DEBUG) << "Stream started.";
    return true;
}

void VideoCapture::stopStream() {
    // Tell the background thread to stop
    int prevRunMode = mRunMode.fetch_or(STOPPING);
    if (prevRunMode == STOPPED) {
        // The background thread wasn't running, so set the flag back to STOPPED
        mRunMode = STOPPED;
    } else if (prevRunMode & STOPPING) {
        LOG(ERROR) << "stopStream called while stream is already stopping.  "
                   << "Reentrancy is not supported!";
        return;
    } else {
        // Block until the background thread is stopped
        if (mCaptureThread.joinable()) {
            mCaptureThread.join();
        }

        // Stop the underlying video stream (automatically empties the buffer queue)
        const int type = mBufferType;
        if (ioctl(mDeviceFd, VIDIOC_STREAMOFF, &type) < 0) {
            PLOG(ERROR) << "VIDIOC_STREAMOFF failed";
        }

        LOG(DEBUG) << "Capture thread stopped.";
    }

    for (int i = 0; i < mNumBuffers; ++i) {
        // Unmap the buffers we allocated
        munmap(mPixelBuffers[i], mBufferSize);
    }

    // Tell the L4V2 driver to release our streaming buffers
    v4l2_requestbuffers bufrequest;
    bufrequest.type = mBufferType;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 0;
    ioctl(mDeviceFd, VIDIOC_REQBUFS, &bufrequest);

    // Drop our reference to the frame delivery callback interface
    mCallback = nullptr;

    // Release capture buffers
    mNumBuffers = 0;
    mBufferInfos = nullptr;
    mPixelBuffers = nullptr;
}

bool VideoCapture::returnFrame(int id) {
    if (mFrames.find(id) == mFrames.end()) {
        LOG(WARNING) << "Invalid request to return a buffer " << id << " is ignored.";
        return false;
    }

    // Requeue the buffer to capture the next available frame
    if (ioctl(mDeviceFd, VIDIOC_QBUF, &mBufferInfos[id]) < 0) {
        PLOG(ERROR) << "VIDIOC_QBUF failed";
        return false;
    }

    // Remove ID of returned buffer from the set
    mFrames.erase(id);

    return true;
}

// This runs on a background thread to receive and dispatch video frames
void VideoCapture::collectFrames() {
    // Run until our atomic signal is cleared
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(mDeviceFd, &fds);

    while (mRunMode == RUN) {
        struct v4l2_plane mplanes[VIDEO_PLANES];
         struct v4l2_buffer buf = {
                        .type = mBufferType, .memory = V4L2_MEMORY_MMAP};

        if (mBufferType == V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
            LOG(INFO) << "MPLANE setting to dqbuf";
            buf.length = VIDEO_PLANES;
            buf.m.planes = mplanes;
        }

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;  // 10ms
        int ret = select(mDeviceFd + 1, &fds, nullptr, nullptr, &tv);
        if (ret == -1) {
            LOG(INFO) << "Fatal error: selection failure ";
            break;
        } else if (ret == 0) {
            LOG(INFO) << "Warning: timeout of device ";
            continue;
        }

        // Wait for a buffer to be ready
        if (ioctl(mDeviceFd, VIDIOC_DQBUF, &buf) < 0) {
            PLOG(ERROR) << "VIDIOC_DQBUF failed";
            break;
        }

        mFrames.insert(buf.index);

        // Update a frame metadata
        mBufferInfos[buf.index] = buf;

        // If a callback was requested per frame, do that now
        if (mCallback) {
            mCallback(this, &mBufferInfos[buf.index], mPixelBuffers[buf.index]);
        }
    }

    // Mark ourselves stopped
    LOG(DEBUG) << "VideoCapture thread ending";
    mRunMode = STOPPED;
}

int VideoCapture::setParameter(v4l2_control& control) {
    int status = ioctl(mDeviceFd, VIDIOC_S_CTRL, &control);
    if (status < 0) {
        PLOG(ERROR) << "Failed to program a parameter value "
                    << "id = " << std::hex << control.id;
    }

    return status;
}

int VideoCapture::getParameter(v4l2_control& control) {
    int status = ioctl(mDeviceFd, VIDIOC_G_CTRL, &control);
    if (status < 0) {
        PLOG(ERROR) << "Failed to read a parameter value"
                    << " fd = " << std::hex << mDeviceFd << " id = " << control.id;
    }

    return status;
}

std::set<uint32_t> VideoCapture::enumerateCameraControls() {
    // Retrieve available camera controls
    struct v4l2_queryctrl ctrl = {.id = V4L2_CTRL_FLAG_NEXT_CTRL};

    std::set<uint32_t> ctrlIDs;
    while (0 == ioctl(mDeviceFd, VIDIOC_QUERYCTRL, &ctrl)) {
        if (!(ctrl.flags & V4L2_CTRL_FLAG_DISABLED)) {
            ctrlIDs.insert(ctrl.id);
        }

        ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    if (errno != EINVAL) {
        PLOG(WARNING) << "Failed to run VIDIOC_QUERYCTRL";
    }

    return std::move(ctrlIDs);
}
