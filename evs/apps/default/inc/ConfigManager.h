/*
 * Copyright (C) 2017 The Android Open Source Project
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
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <cerrno>
#include <string>
#include <vector>

#include <system/graphics-base.h>

class ConfigManager final {
public:
    struct CameraInfo {
        std::string cameraId = "";  // The name of the camera from the point of view of the HAL
        std::string function = "";  // The expected use for this camera ("reverse", "left", "right")
        float position[3] = {0};    // x, y, z -> right, fwd, up in the units of car space
        float yaw = 0;       // radians positive to the left (right hand rule about global z axis)
        float pitch = 0;     // positive upward (ie: right hand rule about local x axis)
        float roll = 0;      // radians positively increasing clockwisely around the optical axis
        float hfov = 0;      // radians
        float vfov = 0;      // radians
        bool hflip = false;  // boolean to flip the preview horizontally
        bool vflip = false;  // boolean to flip the preview vertically
    };

    struct DisplayInfo {
        uint8_t port = 0;            // Display port number to use
        std::string function = "";   // The expected use for this display.
        float frontRangeInCarSpace;  // How far the display extends in front of the car
        float rearRangeInCarSpace;   // How far the display extends behind the car
    };

    bool initialize(const char* configFileName);

    // World space dimensions of the car
    float getCarWidth() const { return mCarWidth; };
    float getCarLength() const { return mWheelBase + mFrontExtent + mRearExtent; };
    float getWheelBase() const { return mWheelBase; };

    // Car space (world space centered on the rear axel) edges of the car
    float getFrontLocation() const { return mWheelBase + mFrontExtent; };
    float getRearLocation() const { return -mRearExtent; };
    float getRightLocation() const { return mCarWidth * 0.5f; };
    float getLeftLocation() const { return -mCarWidth * 0.5f; };

    // Where are the edges of the top down display in car space?
    float getDisplayTopLocation() const {
        // From the rear axel (origin) to the front bumper, and then beyond by the front range
        return mWheelBase + mFrontExtent + mDisplays[mActiveDisplayId].frontRangeInCarSpace;
    };
    float getDisplayBottomLocation() const {
        // From the rear axel (origin) to the back bumper, and then beyond by the back range
        return -mRearExtent - mDisplays[mActiveDisplayId].rearRangeInCarSpace;
    };
    float getDisplayRightLocation(float aspectRatio) const {
        // Given the display aspect ratio (width over height), how far can we see to the right?
        return (getDisplayTopLocation() - getDisplayBottomLocation()) * 0.5f * aspectRatio;
    };
    float getDisplayLeftLocation(float aspectRatio) const {
        // Given the display aspect ratio (width over height), how far can we see to the left?
        return -getDisplayRightLocation(aspectRatio);
    };

    // At which texel (vertically in the image) are the front and rear bumpers of the car?
    float carGraphicFrontPixel() const { return mCarGraphicFrontPixel; };
    float carGraphicRearPixel() const { return mCarGraphicRearPixel; };

    const std::vector<CameraInfo>& getCameras() const { return mCameras; };

    int setActiveDisplayId(int displayId) {
        if (displayId == -1) {
            // -1 is reserved for the default display, which is the first
            // display in config.json's display list
            printf("Uses a display with id %d", mDisplays[0].port);
            mActiveDisplayId = mDisplays[0].port;
            return mActiveDisplayId;
        } else if (displayId < 0) {
            printf("Display %d is invalid.", displayId);
            return -ENOENT;
        } else {
            for (auto display : mDisplays) {
                if (display.port == displayId) {
                    mActiveDisplayId = displayId;
                    return mActiveDisplayId;
                }
            }

            printf("Display %d does not exist.", displayId);
            return -ENOENT;
        }
    }
    const std::vector<DisplayInfo>& getDisplays() const { return mDisplays; };
    const DisplayInfo& getActiveDisplay() const { return mDisplays[mActiveDisplayId]; };
    void useExternalMemory(bool flag) { mUseExternalMemory = flag; }
    bool getUseExternalMemory() const { return mUseExternalMemory; }
    void setExternalMemoryFormat(android_pixel_format_t format) { mExternalMemoryFormat = format; }
    android_pixel_format_t getExternalMemoryFormat() const { return mExternalMemoryFormat; }
    void setMockGearSignal(int32_t signal) { mMockGearSignal = signal; }
    int32_t getMockGearSignal() const { return mMockGearSignal; }

private:
    // Camera information
    std::vector<CameraInfo> mCameras;

    // Display information
    std::vector<DisplayInfo> mDisplays;
    int mActiveDisplayId;

    // Memory management
    bool mUseExternalMemory;

    // Format of external memory
    android_pixel_format_t mExternalMemoryFormat;

    // Gear signal to simulate in test mode
    int32_t mMockGearSignal;

    // Car body information (assumes front wheel steering and origin at center of rear axel)
    // Note that units aren't specified and don't matter as long as all length units are consistent
    // within the JSON file from which we parse.  That is, if everything is in meters, that's fine.
    // Everything in mm?  That's fine too.
    float mCarWidth;
    float mWheelBase;
    float mFrontExtent;
    float mRearExtent;

    // Top view car image information
    float mCarGraphicFrontPixel;  // How many pixels from the top of the image does the car start
    float mCarGraphicRearPixel;   // How many pixels from the top of the image does the car end
};

#endif  // CONFIG_MANAGER_H
