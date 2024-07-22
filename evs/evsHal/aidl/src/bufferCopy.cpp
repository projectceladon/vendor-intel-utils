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

#include "bufferCopy.h"

#include <android-base/logging.h>

#include <libyuv.h>

namespace aidl::android::hardware::automotive::evs::implementation {

// Round up to the nearest multiple of the given alignment value
template <unsigned alignment>
int align(int value) {
    static_assert((alignment && !(alignment & (alignment - 1))), "alignment must be a power of 2");

    unsigned mask = alignment - 1;
    return (value + mask) & ~mask;
}

void fillNV21FromNV21(const BufferDesc& tgtBuff, uint8_t* tgt, void* imgData, unsigned) {
    // The NV21 format provides a Y array of 8bit values, followed by a 1/2 x 1/2 interleave U/V
    // array. It assumes an even width and height for the overall image, and a horizontal stride
    // that is an even multiple of 16 bytes for both the Y and UV arrays.

    // Target  and source image layout properties (They match since the formats match!)
    const AHardwareBuffer_Desc* pDesc =
            reinterpret_cast<const AHardwareBuffer_Desc*>(&tgtBuff.buffer.description);
    const unsigned strideLum = align<16>(pDesc->width);
    const unsigned sizeY = strideLum * pDesc->height;
    const unsigned strideColor = strideLum;  // 1/2 the samples, but two interleaved channels
    const unsigned sizeColor = strideColor * pDesc->height / 2;
    const unsigned totalBytes = sizeY + sizeColor;

    // Simply copy the data byte for byte
    memcpy(tgt, imgData, totalBytes);
}

void fillNV21FromYUYV(const BufferDesc& tgtBuff, uint8_t* tgt, void* imgData, unsigned imgStride) {
    // The YUYV format provides an interleaved array of pixel values with U and V subsampled in
    // the horizontal direction only.  Also known as interleaved 422 format.  A 4 byte
    // "macro pixel" provides the Y value for two adjacent pixels and the U and V values shared
    // between those two pixels.  The width of the image must be an even number.
    // We need to down sample the UV values and collect them together after all the packed Y values
    // to construct the NV21 format.
    // NV21 requires even width and height, so we assume that is the case for the incomming image
    // as well.
    uint32_t* srcDataYUYV = (uint32_t*)imgData;
    struct YUYVpixel {
        uint8_t Y1;
        uint8_t U;
        uint8_t Y2;
        uint8_t V;
    };

    // Target image layout properties
    const AHardwareBuffer_Desc* pDesc =
            reinterpret_cast<const AHardwareBuffer_Desc*>(&tgtBuff.buffer.description);
    const unsigned strideLum = align<16>(pDesc->width);
    const unsigned sizeY = strideLum * pDesc->height;
    const unsigned strideColor = strideLum;  // 1/2 the samples, but two interleaved channels

    // Source image layout properties
    const unsigned srcRowPixels = imgStride / 4;  // imgStride is in units of bytes
    const unsigned srcRowDoubleStep = srcRowPixels * 2;
    uint32_t* topSrcRow = srcDataYUYV;
    uint32_t* botSrcRow = srcDataYUYV + srcRowPixels;

    // We're going to work on one 2x2 cell in the output image at at time
    for (unsigned cellRow = 0; cellRow < pDesc->height / 2; cellRow++) {
        // Set up the output pointers
        uint8_t* yTopRow = tgt + (cellRow * 2) * strideLum;
        uint8_t* yBotRow = yTopRow + strideLum;
        uint8_t* uvRow = (tgt + sizeY) + cellRow * strideColor;

        for (unsigned cellCol = 0; cellCol < pDesc->width / 2; cellCol++) {
            // Collect the values from the YUYV interleaved data
            const YUYVpixel* pTopMacroPixel = (YUYVpixel*)&topSrcRow[cellCol];
            const YUYVpixel* pBotMacroPixel = (YUYVpixel*)&botSrcRow[cellCol];

            // Down sample the U/V values by linear average between rows
            const uint8_t uValue = (pTopMacroPixel->U + pBotMacroPixel->U) >> 1;
            const uint8_t vValue = (pTopMacroPixel->V + pBotMacroPixel->V) >> 1;

            // Store the values into the NV21 layout
            yTopRow[cellCol * 2] = pTopMacroPixel->Y1;
            yTopRow[cellCol * 2 + 1] = pTopMacroPixel->Y2;
            yBotRow[cellCol * 2] = pBotMacroPixel->Y1;
            yBotRow[cellCol * 2 + 1] = pBotMacroPixel->Y2;
            uvRow[cellCol * 2] = uValue;
            uvRow[cellCol * 2 + 1] = vValue;
        }

        // Skipping two rows to get to the next set of two source rows
        topSrcRow += srcRowDoubleStep;
        botSrcRow += srcRowDoubleStep;
    }
}

void fillRGBAFromYUYV(const BufferDesc& tgtBuff, uint8_t* tgt, void* imgData, unsigned imgStride) {
    const AHardwareBuffer_Desc* pDesc =
            reinterpret_cast<const AHardwareBuffer_Desc*>(&tgtBuff.buffer.description);
    // Converts YUY2ToARGB (little endian).  Please note that libyuv uses the
    // little endian while we're using the big endian in RGB format names.
    const auto dstStrideInBytes = pDesc->stride * 4;  // 4-byte per pixel
    auto result = libyuv::YUY2ToARGB((const uint8_t*)imgData,
                                     imgStride,  // input stride in bytes
                                     tgt,
                                     dstStrideInBytes,  // output stride in bytes
                                     pDesc->width, pDesc->height);
    if (result) {
        LOG(ERROR) << "Failed to convert YUYV to BGRA.";
        return;
    }

    // Swaps R and B pixels to convert BGRA to RGBA in place.
    // TODO(b/190783702): Consider allocating an extra space to store ARGB data
    //                    temporarily if below operation is too slow.
    result = libyuv::ABGRToARGB(tgt, dstStrideInBytes, tgt, dstStrideInBytes, pDesc->width,
                                pDesc->height);
    if (result) {
        LOG(ERROR) << "Failed to convert BGRA to RGBA.";
    }
}

void fillYUYVFromYUYV(const BufferDesc& tgtBuff, uint8_t* tgt, void* imgData, unsigned imgStride) {
    const AHardwareBuffer_Desc* pDesc =
            reinterpret_cast<const AHardwareBuffer_Desc*>(&tgtBuff.buffer.description);
    unsigned width = pDesc->width;
    unsigned height = pDesc->height;
    uint8_t* src = (uint8_t*)imgData;
    uint8_t* dst = (uint8_t*)tgt;
    unsigned srcStrideBytes = imgStride;
    unsigned dstStrideBytes = pDesc->stride * 2;

    for (unsigned r = 0; r < height; r++) {
        // Copy a pixel row at a time (2 bytes per pixel, averaged over a YUYV macro pixel)
        memcpy(dst + r * dstStrideBytes, src + r * srcStrideBytes, width * 2);
    }
}

void fillYUYVFromUYVY(const BufferDesc& tgtBuff, uint8_t* tgt, void* imgData, unsigned imgStride) {
    const AHardwareBuffer_Desc* pDesc =
            reinterpret_cast<const AHardwareBuffer_Desc*>(&tgtBuff.buffer.description);
    unsigned width = pDesc->width;
    unsigned height = pDesc->height;
    uint32_t* src = (uint32_t*)imgData;
    uint32_t* dst = (uint32_t*)tgt;
    unsigned srcStridePixels = imgStride / 2;
    unsigned dstStridePixels = pDesc->stride;

    const int srcRowPadding32 =
            srcStridePixels / 2 - width / 2;  // 2 bytes per pixel, 4 bytes per word
    const int dstRowPadding32 =
            dstStridePixels / 2 - width / 2;  // 2 bytes per pixel, 4 bytes per word

    for (unsigned r = 0; r < height; r++) {
        for (unsigned c = 0; c < width / 2; c++) {
            // Note:  we're walking two pixels at a time here (even/odd)
            uint32_t srcPixel = *src++;

            uint8_t Y1 = (srcPixel)&0xFF;
            uint8_t U = (srcPixel >> 8) & 0xFF;
            uint8_t Y2 = (srcPixel >> 16) & 0xFF;
            uint8_t V = (srcPixel >> 24) & 0xFF;

            // Now we write back the pair of pixels with the components swizzled
            *dst++ = (U) | (Y1 << 8) | (V << 16) | (Y2 << 24);
        }

        // Skip over any extra data or end of row alignment padding
        src += srcRowPadding32;
        dst += dstRowPadding32;
    }
}

}  // namespace aidl::android::hardware::automotive::evs::implementation
