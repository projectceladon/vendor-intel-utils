/**
 * @file RemoteCameraDevice.cpp
 * @author Shiva Kumara (shiva.kumara.rudrappa@intel.com)
 * @brief  Implementation of remote camera device api.
 * @version 0.1
 * @date 2024-06-18
 *
 * Copyright (c) 2021 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "RemoteCamDev"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include "RemoteCameraDevice.h"
#include "CameraSocketCommand.h"
#include <aidl/android/hardware/camera/common/Status.h>
#include <convert.h>
#include <linux/videodev2.h>
#include <regex>
#include <set>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace implementation {


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define UPDATE(tag, data, size)                        \
    do {                                               \
        if (metadata->update((tag), (data), (size))) { \
            ALOGE("Update " #tag " failed!");          \
            return -EINVAL;                            \
        }                                              \
    } while (0)


using ::aidl::android::hardware::camera::common::Status;

RemoteCameraDevice::RemoteCameraDevice(const std::string& devicePath, const int clientFd,
                                           const ExternalCameraConfig& config)
    : mFd(clientFd), mCameraId(devicePath), mCfg(config) {    
}

RemoteCameraDevice::~RemoteCameraDevice() {}

ndk::ScopedAStatus RemoteCameraDevice::getCameraCharacteristics(CameraMetadata* _aidl_return) {
    Mutex::Autolock _l(mLock);
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    if (isInitFailedLocked()) {
        return fromStatus(Status::INTERNAL_ERROR);
    }

    const camera_metadata_t* rawMetadata = mCameraCharacteristics.getAndLock();
    convertToAidl(rawMetadata, _aidl_return);
    mCameraCharacteristics.unlock(rawMetadata);
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus RemoteCameraDevice::getPhysicalCameraCharacteristics(const std::string&,
                                                                          CameraMetadata*) {
    ALOGE("%s: Physical camera functions are not supported for external cameras.", __FUNCTION__);
    return fromStatus(Status::ILLEGAL_ARGUMENT);
}

ndk::ScopedAStatus RemoteCameraDevice::getResourceCost(CameraResourceCost* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    _aidl_return->resourceCost = 100;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus RemoteCameraDevice::isStreamCombinationSupported(
        const StreamConfiguration& in_streams, bool* _aidl_return) {
    if (isInitFailed()) {
        ALOGE("%s: camera %s. camera init failed!", __FUNCTION__, mCameraId.c_str());
        return fromStatus(Status::INTERNAL_ERROR);
    }
    Status s = RemoteCameraDeviceSession::isStreamCombinationSupported(in_streams,
                                                                         mSupportedFormats);
    *_aidl_return = s == Status::OK;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus RemoteCameraDevice::open(
        const std::shared_ptr<ICameraDeviceCallback>& in_callback,
        std::shared_ptr<ICameraDeviceSession>* _aidl_return) {
    if (_aidl_return == nullptr) {
        ALOGE("%s: cannot open camera %s. return session ptr is null!", __FUNCTION__,
              mCameraId.c_str());
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    Mutex::Autolock _l(mLock);
    if (isInitFailedLocked()) {
        ALOGE("%s: cannot open camera %s. camera init failed!", __FUNCTION__, mCameraId.c_str());
        return fromStatus(Status::INTERNAL_ERROR);
    }

    std::shared_ptr<RemoteCameraDeviceSession> session;
    ALOGI("%s: Initializing device for camera %s", __FUNCTION__, mCameraId.c_str());
    session = mSession.lock();

    if (session != nullptr && !session->isClosed()) {
        ALOGE("%s: cannot open an already opened camera!", __FUNCTION__);
        return fromStatus(Status::CAMERA_IN_USE);
    }    
    status_t status = INVALID_OPERATION;
    size_t config_cmd_packet_size = sizeof(camera_header_t) + sizeof(camera_config_cmd_t);
    camera_config_cmd_t config_cmd = {};
    config_cmd.version = CAMERA_VHAL_VERSION_2;
    config_cmd.cmd = camera_cmd_t::CMD_OPEN;
    camera_packet_t *config_cmd_packet = NULL;

    ALOGI(" sending open command ");
    config_cmd_packet = (camera_packet_t *)malloc(config_cmd_packet_size);
    if (config_cmd_packet == NULL) {
        ALOGE(LOG_TAG "%s: config camera_packet_t allocation failed: %d ", __FUNCTION__, __LINE__);
        return fromStatus(Status::INTERNAL_ERROR);
    }

    config_cmd_packet->header.type = CAMERA_CONFIG;
    config_cmd_packet->header.size = sizeof(camera_config_cmd_t);
    memcpy(config_cmd_packet->payload, &config_cmd, sizeof(camera_config_cmd_t));

    if (send(mFd, config_cmd_packet, config_cmd_packet_size, 0) < 0) {
        ALOGE(LOG_TAG "%s: Failed to send Camera %s command to client, err %s ", __FUNCTION__,
              (config_cmd.cmd == camera_cmd_t::CMD_CLOSE) ? "CloseCamera" : "OpenCamera", strerror(errno));
        free(config_cmd_packet);
        return fromStatus(Status::INTERNAL_ERROR);
    }
    status = OK;
    free(config_cmd_packet);
    
    session = createSession(in_callback, mSupportedFormats, mCroppingType,
                            mCameraCharacteristics, mFd);
    
    if (session == nullptr) {
        ALOGE("%s: camera device session allocation failed", __FUNCTION__);
        return fromStatus(Status::INTERNAL_ERROR);
    }

    if (session->isInitFailed()) {
        ALOGE("%s: camera device session init failed", __FUNCTION__);
        return fromStatus(Status::INTERNAL_ERROR);
    }

    mSession = session;
    *_aidl_return = session;
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus RemoteCameraDevice::openInjectionSession(
        const std::shared_ptr<ICameraDeviceCallback>&, std::shared_ptr<ICameraInjectionSession>*) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}

ndk::ScopedAStatus RemoteCameraDevice::setTorchMode(bool) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}

ndk::ScopedAStatus RemoteCameraDevice::turnOnTorchWithStrengthLevel(int32_t) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}

ndk::ScopedAStatus RemoteCameraDevice::getTorchStrengthLevel(int32_t*) {
    return fromStatus(Status::OPERATION_NOT_SUPPORTED);
}

std::shared_ptr<RemoteCameraDeviceSession> RemoteCameraDevice::createSession(
        const std::shared_ptr<ICameraDeviceCallback>& cb,
        const std::vector<SupportedV4L2Format>& sortedFormats, const CroppingType& croppingType,
        const common::V1_0::helper::CameraMetadata& chars, int vsockFd) {
    return ndk::SharedRefBase::make<RemoteCameraDeviceSession>(
            cb, sortedFormats, croppingType, chars, vsockFd);
}

bool RemoteCameraDevice::isInitFailed() {
    Mutex::Autolock _l(mLock);
    return isInitFailedLocked();
}

bool RemoteCameraDevice::isInitFailedLocked() {
    if (!mInitialized) {
        status_t ret = initCameraCharacteristics();
        if (ret != OK) {
            ALOGE("%s: init camera characteristics failed: errorno %d", __FUNCTION__, ret);
            mInitFailed = true;
        }
        mInitialized = true;
    }
    return mInitFailed;
}

void RemoteCameraDevice::initSupportedFormatsLocked() {
    std::vector<SupportedV4L2Format> fmts = getCandidateSupportedFormatsLocked();
    if (fmts.size() == 0) {
        ALOGE("%s: cannot find suitable cropping type!", __FUNCTION__);
        return;
    }

    mSupportedFormats = fmts;
    mCroppingType = HORIZONTAL;
    return;
}

status_t RemoteCameraDevice::initCameraCharacteristics() {
    if (!mCameraCharacteristics.isEmpty()) {
        // Camera Characteristics previously initialized. Skip.
        return OK;
    }
    status_t ret;
    ret = initDefaultCharsKeys(&mCameraCharacteristics);
    if (ret != OK) {
        ALOGE("%s: init default characteristics key failed: errorno %d", __FUNCTION__, ret);
        mCameraCharacteristics.clear();
        return ret;
    }

    ret = initCameraControlsCharsKeys(&mCameraCharacteristics);
    if (ret != OK) {
        ALOGE("%s: init camera control characteristics key failed: errorno %d", __FUNCTION__, ret);
        mCameraCharacteristics.clear();
        return ret;
    }

    ret = initOutputCharsKeys(&mCameraCharacteristics);
    if (ret != OK) {
        ALOGE("%s: init output characteristics key failed: errorno %d", __FUNCTION__, ret);
        mCameraCharacteristics.clear();
        return ret;
    }

    ret = initAvailableCapabilities(&mCameraCharacteristics);
    if (ret != OK) {
        ALOGE("%s: init available capabilities key failed: errorno %d", __FUNCTION__, ret);
        mCameraCharacteristics.clear();
        return ret;
    }

    return OK;
}

status_t RemoteCameraDevice::initAvailableCapabilities(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata) {
    ALOGI("%s", __FUNCTION__);
    if (mSupportedFormats.empty()) {
        ALOGE("%s: Supported formats list is empty", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    std::vector<uint8_t> availableCapabilities;
    availableCapabilities.push_back(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE);
    UPDATE(ANDROID_REQUEST_AVAILABLE_CAPABILITIES, availableCapabilities.data(),
            availableCapabilities.size());

    return OK;
}

status_t RemoteCameraDevice::initDefaultCharsKeys(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata) {
    const uint8_t hardware_level = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_EXTERNAL;
    UPDATE(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL, &hardware_level, 1);

    // android.colorCorrection
    const uint8_t availableAberrationModes[] = {ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF};
    UPDATE(ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES, availableAberrationModes,
           ARRAY_SIZE(availableAberrationModes));

    // android.control
    const uint8_t antibandingMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    UPDATE(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES, &antibandingMode, 1);

    const int32_t controlMaxRegions[] = {/*AE*/ 0, /*AWB*/ 0, /*AF*/ 0};
    UPDATE(ANDROID_CONTROL_MAX_REGIONS, controlMaxRegions, ARRAY_SIZE(controlMaxRegions));

    const uint8_t videoStabilizationMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    UPDATE(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES, &videoStabilizationMode, 1);

    const uint8_t awbAvailableMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    UPDATE(ANDROID_CONTROL_AWB_AVAILABLE_MODES, &awbAvailableMode, 1);

    const uint8_t aeAvailableMode = ANDROID_CONTROL_AE_MODE_ON;
    UPDATE(ANDROID_CONTROL_AE_AVAILABLE_MODES, &aeAvailableMode, 1);

    const uint8_t availableFffect = ANDROID_CONTROL_EFFECT_MODE_OFF;
    UPDATE(ANDROID_CONTROL_AVAILABLE_EFFECTS, &availableFffect, 1);

    const uint8_t controlAvailableModes[] = {ANDROID_CONTROL_MODE_OFF, ANDROID_CONTROL_MODE_AUTO};
    UPDATE(ANDROID_CONTROL_AVAILABLE_MODES, controlAvailableModes,
           ARRAY_SIZE(controlAvailableModes));

    // android.edge
    const uint8_t edgeMode = ANDROID_EDGE_MODE_OFF;
    UPDATE(ANDROID_EDGE_AVAILABLE_EDGE_MODES, &edgeMode, 1);

    // android.flash
    const uint8_t flashInfo = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    UPDATE(ANDROID_FLASH_INFO_AVAILABLE, &flashInfo, 1);

    // android.hotPixel
    const uint8_t hotPixelMode = ANDROID_HOT_PIXEL_MODE_OFF;
    UPDATE(ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES, &hotPixelMode, 1);

    // android.jpeg
    const int32_t jpegAvailableThumbnailSizes[] = {0,   0,  240, 180};
    UPDATE(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES, jpegAvailableThumbnailSizes,
           ARRAY_SIZE(jpegAvailableThumbnailSizes));

    const int32_t jpegMaxSize = mCfg.maxJpegBufSize;
    UPDATE(ANDROID_JPEG_MAX_SIZE, &jpegMaxSize, 1);

    // android.lens
    const uint8_t focusDistanceCalibration =
            ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    UPDATE(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION, &focusDistanceCalibration, 1);

    const uint8_t opticalStabilizationMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    UPDATE(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION, &opticalStabilizationMode, 1);

    const uint8_t facing = ANDROID_LENS_FACING_EXTERNAL;
    UPDATE(ANDROID_LENS_FACING, &facing, 1);

    // android.noiseReduction
    const uint8_t noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
    UPDATE(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES, &noiseReductionMode, 1);
    UPDATE(ANDROID_NOISE_REDUCTION_MODE, &noiseReductionMode, 1);

    const int32_t partialResultCount = 1;
    UPDATE(ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &partialResultCount, 1);

    // This means pipeline latency of X frame intervals. The maximum number is 4.
    const uint8_t requestPipelineMaxDepth = 4;
    UPDATE(ANDROID_REQUEST_PIPELINE_MAX_DEPTH, &requestPipelineMaxDepth, 1);

    // Three numbers represent the maximum numbers of different types of output
    // streams simultaneously. The types are raw sensor, processed (but not
    // stalling), and processed (but stalling). For usb limited mode, raw sensor
    // is not supported. Stalling stream is JPEG. Non-stalling streams are
    // YUV_420_888 or YV12.
    const int32_t requestMaxNumOutputStreams[] = {
            /*RAW*/ 0,
            /*Processed*/2,// RemoteCameraDeviceSession::kMaxProcessedStream,
            /*Stall*/1};// RemoteCameraDeviceSession::kMaxStallStream};
    UPDATE(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, requestMaxNumOutputStreams,
           ARRAY_SIZE(requestMaxNumOutputStreams));

    // Limited mode doesn't support reprocessing.
    const int32_t requestMaxNumInputStreams = 0;
    UPDATE(ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS, &requestMaxNumInputStreams, 1);

    // android.scaler
    // TODO: b/72263447 V4L2_CID_ZOOM_*
    const float scalerAvailableMaxDigitalZoom[] = {1};
    UPDATE(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, scalerAvailableMaxDigitalZoom,
           ARRAY_SIZE(scalerAvailableMaxDigitalZoom));

    const uint8_t croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    UPDATE(ANDROID_SCALER_CROPPING_TYPE, &croppingType, 1);

    const int32_t testPatternModes[] = {ANDROID_SENSOR_TEST_PATTERN_MODE_OFF};
    UPDATE(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES, testPatternModes,
           ARRAY_SIZE(testPatternModes));

    const uint8_t timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN;
    UPDATE(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE, &timestampSource, 1);

    // Orientation is a bit odd for external camera, but consider it as the orientation
    // between the external camera sensor (which is usually landscape) and the device's
    // natural display orientation. For devices with natural landscape display (ex: tablet/TV), the
    // orientation should be 0. For devices with natural portrait display (phone), the orientation
    // should be 270.
    const int32_t orientation = 0;//mCfg.orientation;
    UPDATE(ANDROID_SENSOR_ORIENTATION, &orientation, 1);

    // android.shading
    const uint8_t availableMode = ANDROID_SHADING_MODE_OFF;
    UPDATE(ANDROID_SHADING_AVAILABLE_MODES, &availableMode, 1);

    // android.statistics
    const uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    UPDATE(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES, &faceDetectMode, 1);

    const int32_t maxFaceCount = 0;
    UPDATE(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT, &maxFaceCount, 1);

    const uint8_t availableHotpixelMode = ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF;
    UPDATE(ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES, &availableHotpixelMode, 1);

    const uint8_t lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    UPDATE(ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES, &lensShadingMapMode, 1);

    // android.sync
    const int32_t maxLatency = ANDROID_SYNC_MAX_LATENCY_UNKNOWN;
    UPDATE(ANDROID_SYNC_MAX_LATENCY, &maxLatency, 1);

    /* Other sensor/RAW related keys:
     * android.sensor.info.colorFilterArrangement -> no need if we don't do RAW
     * android.sensor.info.physicalSize           -> not available
     * android.sensor.info.whiteLevel             -> not available/not needed
     * android.sensor.info.lensShadingApplied     -> not needed
     * android.sensor.info.preCorrectionActiveArraySize -> not available/not needed
     * android.sensor.blackLevelPattern           -> not available/not needed
     */

    const int32_t availableRequestKeys[] = {ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
                                            ANDROID_CONTROL_AE_ANTIBANDING_MODE,
                                            ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                            ANDROID_CONTROL_AE_LOCK,
                                            ANDROID_CONTROL_AE_MODE,
                                            ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER,
                                            ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                                            ANDROID_CONTROL_AF_MODE,
                                            ANDROID_CONTROL_AF_TRIGGER,
                                            ANDROID_CONTROL_AWB_LOCK,
                                            ANDROID_CONTROL_AWB_MODE,
                                            ANDROID_CONTROL_CAPTURE_INTENT,
                                            ANDROID_CONTROL_EFFECT_MODE,
                                            ANDROID_CONTROL_MODE,
                                            ANDROID_CONTROL_SCENE_MODE,
                                            ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
                                            ANDROID_FLASH_MODE,
                                            ANDROID_JPEG_ORIENTATION,
                                            ANDROID_JPEG_QUALITY,
                                            ANDROID_JPEG_THUMBNAIL_QUALITY,
                                            ANDROID_JPEG_THUMBNAIL_SIZE,
                                            ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
                                            ANDROID_NOISE_REDUCTION_MODE,
                                            ANDROID_SCALER_CROP_REGION,
                                            ANDROID_SENSOR_TEST_PATTERN_MODE,
                                            ANDROID_STATISTICS_FACE_DETECT_MODE,
                                            ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE};
    UPDATE(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS, availableRequestKeys,
           ARRAY_SIZE(availableRequestKeys));

    const int32_t availableResultKeys[] = {ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
                                           ANDROID_CONTROL_AE_ANTIBANDING_MODE,
                                           ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                           ANDROID_CONTROL_AE_LOCK,
                                           ANDROID_CONTROL_AE_MODE,
                                           ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER,
                                           ANDROID_CONTROL_AE_STATE,
                                           ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                                           ANDROID_CONTROL_AF_MODE,
                                           ANDROID_CONTROL_AF_STATE,
                                           ANDROID_CONTROL_AF_TRIGGER,
                                           ANDROID_CONTROL_AWB_LOCK,
                                           ANDROID_CONTROL_AWB_MODE,
                                           ANDROID_CONTROL_AWB_STATE,
                                           ANDROID_CONTROL_CAPTURE_INTENT,
                                           ANDROID_CONTROL_EFFECT_MODE,
                                           ANDROID_CONTROL_MODE,
                                           ANDROID_CONTROL_SCENE_MODE,
                                           ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
                                           ANDROID_FLASH_MODE,
                                           ANDROID_FLASH_STATE,
                                           ANDROID_JPEG_ORIENTATION,
                                           ANDROID_JPEG_QUALITY,
                                           ANDROID_JPEG_THUMBNAIL_QUALITY,
                                           ANDROID_JPEG_THUMBNAIL_SIZE,
                                           ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
                                           ANDROID_NOISE_REDUCTION_MODE,
                                           ANDROID_REQUEST_PIPELINE_DEPTH,
                                           ANDROID_SCALER_CROP_REGION,
                                           ANDROID_SENSOR_TIMESTAMP,
                                           ANDROID_STATISTICS_FACE_DETECT_MODE,
                                           ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE,
                                           ANDROID_STATISTICS_LENS_SHADING_MAP_MODE,
                                           ANDROID_STATISTICS_SCENE_FLICKER};
    UPDATE(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, availableResultKeys,
           ARRAY_SIZE(availableResultKeys));

    UPDATE(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS, AVAILABLE_CHARACTERISTICS_KEYS.data(),
           AVAILABLE_CHARACTERISTICS_KEYS.size());

    return OK;
}

status_t RemoteCameraDevice::initCameraControlsCharsKeys(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata) {
    // android.sensor.info.sensitivityRange   -> V4L2_CID_ISO_SENSITIVITY
    // android.sensor.info.exposureTimeRange  -> V4L2_CID_EXPOSURE_ABSOLUTE
    // android.sensor.info.maxFrameDuration   -> TBD
    // android.lens.info.minimumFocusDistance -> V4L2_CID_FOCUS_ABSOLUTE
    // android.lens.info.hyperfocalDistance
    // android.lens.info.availableFocalLengths -> not available?

    // android.control
    // No AE compensation support for now.
    // TODO: V4L2_CID_EXPOSURE_BIAS
    const int32_t controlAeCompensationRange[] = {0, 0};
    UPDATE(ANDROID_CONTROL_AE_COMPENSATION_RANGE, controlAeCompensationRange,
           ARRAY_SIZE(controlAeCompensationRange));
    const camera_metadata_rational_t controlAeCompensationStep[] = {{0, 1}};
    UPDATE(ANDROID_CONTROL_AE_COMPENSATION_STEP, controlAeCompensationStep,
           ARRAY_SIZE(controlAeCompensationStep));

    // TODO: Check V4L2_CID_AUTO_FOCUS_*.
    const uint8_t afAvailableModes[] = {ANDROID_CONTROL_AF_MODE_AUTO, ANDROID_CONTROL_AF_MODE_OFF};
    UPDATE(ANDROID_CONTROL_AF_AVAILABLE_MODES, afAvailableModes, ARRAY_SIZE(afAvailableModes));

    // TODO: V4L2_CID_SCENE_MODE
    const uint8_t availableSceneMode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    UPDATE(ANDROID_CONTROL_AVAILABLE_SCENE_MODES, &availableSceneMode, 1);

    // TODO: V4L2_CID_3A_LOCK
    const uint8_t aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_FALSE;
    UPDATE(ANDROID_CONTROL_AE_LOCK_AVAILABLE, &aeLockAvailable, 1);
    const uint8_t awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_FALSE;
    UPDATE(ANDROID_CONTROL_AWB_LOCK_AVAILABLE, &awbLockAvailable, 1);

    // TODO: V4L2_CID_ZOOM_*
    const float scalerAvailableMaxDigitalZoom[] = {1};
    UPDATE(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, scalerAvailableMaxDigitalZoom,
           ARRAY_SIZE(scalerAvailableMaxDigitalZoom));

    return OK;
}

status_t RemoteCameraDevice::initOutputCharsKeys(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata) {

    initSupportedFormatsLocked();
    if (mSupportedFormats.empty()) {
        ALOGE("%s: Init supported format list failed", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    std::array<int, /*size*/ 3> halFormats{{HAL_PIXEL_FORMAT_BLOB, HAL_PIXEL_FORMAT_YCbCr_420_888,
                                            HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED}};
    if (mSupportedFormats.empty()) {
        ALOGE("%s: init supported format list failed.", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    for (const auto& supportedFormat : mSupportedFormats) {
        switch (supportedFormat.fourcc) {
        case V4L2_PIX_FMT_YUYV:

            initOutputCharsKeysByFormat(metadata, V4L2_PIX_FMT_YUYV, halFormats,
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT,
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                        ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
                                        ANDROID_SCALER_AVAILABLE_STALL_DURATIONS);
            break;
        case V4L2_PIX_FMT_UYVY:

            initOutputCharsKeysByFormat(metadata, V4L2_PIX_FMT_UYVY, halFormats,
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT,
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                        ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
                                        ANDROID_SCALER_AVAILABLE_STALL_DURATIONS);
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_888:

            initOutputCharsKeysByFormat(metadata, HAL_PIXEL_FORMAT_YCbCr_420_888, halFormats,
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT,
                                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                        ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
                                        ANDROID_SCALER_AVAILABLE_STALL_DURATIONS);
            break;
        default:
            ALOGE("%s: format %c%c%c%c is not supported!", __FUNCTION__,
                  supportedFormat.fourcc & 0xFF, (supportedFormat.fourcc >> 8) & 0xFF,
                  (supportedFormat.fourcc >> 16) & 0xFF, (supportedFormat.fourcc >> 24) & 0xFF);
        }
    }

    // Caculate fps, we set it as fixed 30fps.
    calculateMinFps(metadata);
    int32_t activeArraySize[] = {0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    activeArraySize[2] = DEFAULT_WIDTH;//mCaptureManager->getWidth(mCameraId);
    activeArraySize[3] = DEFAULT_HEIGHT;//mCaptureManager->getHeight(mCameraId);

    // int32_t pixelArraySize[] = {0, 0, 1920, 1080};
    // pixelArraySize[2] = mCaptureManager->getWidth(mCameraId);
    // pixelArraySize[3] = mCaptureManager->getHeight(mCameraId);
    int32_t pixelArraySize[] = {DEFAULT_WIDTH, DEFAULT_HEIGHT};
    pixelArraySize[0] = DEFAULT_WIDTH;//mCaptureManager->getWidth(mCameraId);
    pixelArraySize[1] = DEFAULT_HEIGHT;//mCaptureManager->getHeight(mCameraId);
    UPDATE(ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE, activeArraySize,
           ARRAY_SIZE(activeArraySize));
    UPDATE(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, activeArraySize, ARRAY_SIZE(activeArraySize));
    UPDATE(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixelArraySize, ARRAY_SIZE(pixelArraySize));

    return OK;
}

template <size_t SIZE>
status_t RemoteCameraDevice::initOutputCharsKeysByFormat(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata,
        uint32_t fourcc, const std::array<int, SIZE>& halFormats, int streamConfigTag,
        int streamConfiguration, int minFrameDuration, int stallDuration) {
    ALOGI("%s", __FUNCTION__);
    if (mSupportedFormats.empty()) {
        ALOGE("%s: Init supported format list failed", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    std::vector<int32_t> streamConfigurations;
    std::vector<int64_t> minFrameDurations;
    std::vector<int64_t> stallDurations;
    for (const auto& supportedFormat : mSupportedFormats) {
        if (supportedFormat.fourcc != fourcc) {
            // Skip 4CCs not meant for the halFormats
            continue;
        }

        for (const auto& format : halFormats) {
            ALOGD("streamconf [%dx%d] %d %s", supportedFormat.width, supportedFormat.height, __LINE__, __FILE__);
            streamConfigurations.push_back(format);
            streamConfigurations.push_back(supportedFormat.width);
            streamConfigurations.push_back(supportedFormat.height);
            streamConfigurations.push_back(streamConfigTag);
        }

        int64_t minFrameDuration = std::numeric_limits<int64_t>::max();
        ALOGI("minFrameDuration=%lld", (long long)minFrameDuration);
        for (const auto& fr : supportedFormat.frameRates) {
            // 1000000000LL < (2^32 - 1) and
            // fr.durationNumerator is uint32_t, so no overflow here
            int64_t frameDuration = 1000000000LL * fr.durationNumerator /
                    fr.durationDenominator;
            ALOGD("%s: frameDuration=%lld", __FUNCTION__, (long long)frameDuration);
            if (frameDuration < minFrameDuration) {
                minFrameDuration = frameDuration;
            }
        }

        minFrameDuration = 1000000000LL / 30;
        ALOGI("%s minFrameDuration = %d", __FUNCTION__, (int32_t)minFrameDuration);
        for (const auto& format : halFormats) {
            minFrameDurations.push_back(format);
            minFrameDurations.push_back(supportedFormat.width);
            minFrameDurations.push_back(supportedFormat.height);
            minFrameDurations.push_back(minFrameDuration);
        }
        ALOGI("%s minFrameDurations.size() = %d", __FUNCTION__, (int32_t)minFrameDurations.size());

        // The stall duration is 0 for non-jpeg formats. For JPEG format, stall
        // duration can be 0 if JPEG is small. Here we choose 1 sec for JPEG.
        // TODO: b/72261675. Maybe set this dynamically
        for (const auto& format : halFormats) {
            const int64_t NS_TO_SECOND = 1000000000;
            int64_t stall_duration =
                    (format == HAL_PIXEL_FORMAT_BLOB) ? NS_TO_SECOND : 0;
            stallDurations.push_back(format);
            stallDurations.push_back(supportedFormat.width);
            stallDurations.push_back(supportedFormat.height);
            stallDurations.push_back(stall_duration);
        }
    }

    UPDATE(streamConfiguration, streamConfigurations.data(), streamConfigurations.size());

    UPDATE(minFrameDuration, minFrameDurations.data(), minFrameDurations.size());

    UPDATE(stallDuration, stallDurations.data(), stallDurations.size());
    return OK;
}

status_t RemoteCameraDevice::calculateMinFps(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata) {
    std::set<int32_t> framerates;
    int32_t minFps = std::numeric_limits<int32_t>::max();
    std::vector<int32_t> fpsRanges;
    minFps = 15;
    fpsRanges.push_back(15);
    fpsRanges.push_back(30);
    int64_t maxFrameDuration = 1000000000LL / minFps;
    UPDATE(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, fpsRanges.data(), fpsRanges.size());

    UPDATE(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION, &maxFrameDuration, 1);
    return OK;
}

std::vector<SupportedV4L2Format> RemoteCameraDevice::getCandidateSupportedFormatsLocked() {
    std::vector<SupportedV4L2Format> outFmts;
    SupportedV4L2Format format {
            .width = 1920,//mCaptureManager->getWidth(mCameraId),
            .height = 1080,//mCaptureManager->getHeight(mCameraId),
            .fourcc = HAL_PIXEL_FORMAT_YCbCr_420_888,//V4L2_PIX_FMT_UYVY,//mCaptureManager->getFormat(mCameraId),
    };
    outFmts.push_back(format);
    SupportedV4L2Format hd {
            .width = 1280,//mCaptureManager->getWidth(mCameraId),
            .height = 720,//mCaptureManager->getHeight(mCameraId),
            .fourcc = HAL_PIXEL_FORMAT_YCbCr_420_888,//V4L2_PIX_FMT_UYVY,//mCaptureManager->getFormat(mCameraId),
    };
    outFmts.push_back(hd);
    SupportedV4L2Format vga {
            .width = 640,//mCaptureManager->getWidth(mCameraId),
            .height = 480,//mCaptureManager->getHeight(mCameraId),
            .fourcc = HAL_PIXEL_FORMAT_YCbCr_420_888,//V4L2_PIX_FMT_UYVY,//mCaptureManager->getFormat(mCameraId),
    };
    outFmts.push_back(vga);
    return outFmts;
}

void RemoteCameraDevice::trimSupportedFormats(CroppingType cropType,
                                                std::vector<SupportedV4L2Format>* pFmts) {
    std::vector<SupportedV4L2Format>& sortedFmts = *pFmts;
    if (cropType == VERTICAL) {
        std::sort(sortedFmts.begin(), sortedFmts.end(),
                  [](const SupportedV4L2Format& a, const SupportedV4L2Format& b) -> bool {
                      if (a.width == b.width) {
                          return a.height < b.height;
                      }
                      return a.width < b.width;
                  });
    } else {
        std::sort(sortedFmts.begin(), sortedFmts.end(),
                  [](const SupportedV4L2Format& a, const SupportedV4L2Format& b) -> bool {
                      if (a.height == b.height) {
                          return a.width < b.width;
                      }
                      return a.height < b.height;
                  });
    }

    if (sortedFmts.empty()) {
        ALOGE("%s: input format list is empty!", __FUNCTION__);
        return;
    }

    const auto& maxSize = sortedFmts[sortedFmts.size() - 1];
    float maxSizeAr = ASPECT_RATIO(maxSize);

    // Remove formats that has aspect ratio not croppable from largest size
    std::vector<SupportedV4L2Format> out;
    for (const auto& fmt : sortedFmts) {
        float ar = ASPECT_RATIO(fmt);
        if (isAspectRatioClose(ar, maxSizeAr)) {
            out.push_back(fmt);
        } else if (cropType == HORIZONTAL && ar < maxSizeAr) {
            out.push_back(fmt);
        } else if (cropType == VERTICAL && ar > maxSizeAr) {
            out.push_back(fmt);
        } else {
            ALOGV("%s: size (%d,%d) is removed due to unable to crop %s from (%d,%d)", __FUNCTION__,
                  fmt.width, fmt.height, cropType == VERTICAL ? "vertically" : "horizontally",
                  maxSize.width, maxSize.height);
        }
    }
    sortedFmts = out;
}

binder_status_t RemoteCameraDevice::dump(int fd, const char** args, uint32_t numArgs) {
    std::shared_ptr<RemoteCameraDeviceSession> session = mSession.lock();
    if (session == nullptr) {
        dprintf(fd, "No active camera device session instance\n");
        return STATUS_OK;
    }

    return session->dump(fd, args, numArgs);
}


}  // namespace implementation
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
