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

#define LOG_TAG "ExtCamPrvdr"
//#define LOG_NDEBUG 0

#include "ExternalCameraProvider.h"

#include <ExternalCameraDevice.h>
#include <RemoteCameraDevice.h>

#include <aidl/android/hardware/camera/common/Status.h>
#include <convert.h>
#include <cutils/properties.h>
#include <linux/videodev2.h>
#include <log/log.h>
#include <sys/inotify.h>
#include <regex>
#include "CameraSocketCommand.h"
#include <pthread.h> 

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/types.h>
#include <linux/vm_sockets.h>
pthread_t thread_id; 

#define MAX_RETRY 3

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

using ::aidl::android::hardware::camera::common::Status;
using ::android::hardware::camera::device::implementation::ExternalCameraDevice;
using ::android::hardware::camera::device::implementation::RemoteCameraDevice;
using ::android::hardware::camera::device::implementation::fromStatus;
using ::android::hardware::camera::external::common::ExternalCameraConfig;

namespace {
// "device@<version>/external/<id>"
const std::regex kDeviceNameRE("device@([0-9]+\\.[0-9]+)/external/(.+)");
const int kMaxDevicePathLen = 256;
constexpr char kDevicePath[] = "/dev/";
constexpr char kPrefix[] = "video";
constexpr int kPrefixLen = sizeof(kPrefix) - 1;
constexpr int kDevicePrefixLen = sizeof(kDevicePath) + kPrefixLen - 1;

bool matchDeviceName(int cameraIdOffset, const std::string& deviceName, std::string* deviceVersion,
                     std::string* cameraDevicePath) {
    std::smatch sm;
    if (std::regex_match(deviceName, sm, kDeviceNameRE)) {
        if (deviceVersion != nullptr) {
            *deviceVersion = sm[1];
        }
        if (cameraDevicePath != nullptr) {
            *cameraDevicePath = "/dev/video" + std::to_string(std::stoi(sm[2]) - cameraIdOffset);
        }
        return true;
    }
    return false;
}
}  // namespace

std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }
    res.push_back (s.substr (pos_start));
    return res;
}


bool ExternalCameraProvider::configureCapabilities() {

    bool valid_client_cap_info = false;
    int camera_id, expctd_cam_id;
    struct ValidateClientCapability val_client_cap[2];
    size_t ack_packet_size = sizeof(camera_header_t) + sizeof(camera_ack_t);
    size_t cap_packet_size = sizeof(camera_header_t) + sizeof(camera_capability_t);
    ssize_t recv_size = 0;
    camera_ack_t ack_payload = ACK_CONFIG;

    camera_info_t camera_info[2] = {};
    camera_capability_t capability = {};

    camera_packet_t *cap_packet = NULL;
    camera_packet_t *ack_packet = NULL;
    camera_header_t header = {};
   
    if ((recv_size = recv(mClientFd, (char *)&header, sizeof(camera_header_t), MSG_WAITALL)) < 0) {
        ALOGE(LOG_TAG "%s: Failed to receive header, err: %s ", __FUNCTION__, strerror(errno));
        goto out;
    }

    if (header.type != REQUEST_CAPABILITY) {
        ALOGE(LOG_TAG "%s: Invalid packet type\n", __FUNCTION__);
        goto out;
    }
    ALOGI(LOG_TAG "%s: Received REQUEST_CAPABILITY header from client", __FUNCTION__);

    cap_packet = (camera_packet_t *)malloc(cap_packet_size);
    if (cap_packet == NULL) {
        ALOGE(LOG_TAG "%s: cap camera_packet_t allocation failed: %d ", __FUNCTION__, __LINE__);
        return false;
    }

    cap_packet->header.type = CAPABILITY;
    cap_packet->header.size = sizeof(camera_capability_t);
    capability.codec_type = (uint32_t)VideoCodecType::kAll;
    capability.resolution = (uint32_t)FrameResolution::kAll;
    capability.maxNumberOfCameras = 2;

    memcpy(cap_packet->payload, &capability, sizeof(camera_capability_t));
    if (send(mClientFd, cap_packet, cap_packet_size, 0) < 0) {
        ALOGE(LOG_TAG "%s: Failed to send camera capabilities, err: %s ", __FUNCTION__,
              strerror(errno));
        goto out;
    }
    free(cap_packet);
    cap_packet = (camera_packet_t *)malloc(cap_packet_size);
    if (cap_packet == NULL) {
        ALOGE(LOG_TAG "%s: cap camera_packet_t allocation failed: %d ", __FUNCTION__, __LINE__);
        return false;
    }

    cap_packet->header.type = CAPABILITY;
    cap_packet->header.size = sizeof(camera_capability_t);
    capability.codec_type = (uint32_t)VideoCodecType::kAll;
    capability.resolution = (uint32_t)FrameResolution::kAll;
    capability.maxNumberOfCameras = 2;

    memcpy(cap_packet->payload, &capability, sizeof(camera_capability_t));
    if (send(mClientFd, cap_packet, cap_packet_size, 0) < 0) {
        ALOGE(LOG_TAG "%s: Failed to send camera capabilities, err: %s ", __FUNCTION__,
              strerror(errno));
        goto out;
    }
    ALOGI(LOG_TAG " %s: Sent CAPABILITY packet to client", __FUNCTION__);

    if ((recv_size = recv(mClientFd, (char *)&header, sizeof(camera_header_t), MSG_WAITALL)) < 0) {
        ALOGE(LOG_TAG "%s: Failed to receive header, err: %s ", __FUNCTION__, strerror(errno));
        goto out;
    }


    if (header.type != CAMERA_INFO) {
        ALOGE(LOG_TAG "%s: invalid camera_packet_type: %s", __FUNCTION__,
              camera_type_to_str(header.type));
        goto out;
    }

    // Get the number fo cameras requested to support from client.
    for (int i = 1; i <= MAX_NUMBER_OF_SUPPORTED_CAMERAS; i++) {
        if (header.size == i * sizeof(camera_info_t)) {
            mNumOfCamerasRequested = i;
            break;
        } else if (mNumOfCamerasRequested == 0 && i == MAX_NUMBER_OF_SUPPORTED_CAMERAS) {
            ALOGE(LOG_TAG
                  "%s: Failed to support number of cameras requested by client "
                  "which is higher than the max number of cameras supported in the HAL",
                  __FUNCTION__);
            goto out;
        }
    }

    if (mNumOfCamerasRequested == 0) {
        ALOGE(LOG_TAG "%s: invalid header size received, size = %zu", __FUNCTION__, recv_size);
        goto out;
    }


    if ((recv_size = recv(mClientFd, (char *)&camera_info,
                          mNumOfCamerasRequested * sizeof(camera_info_t), MSG_WAITALL)) < 0) {
        ALOGE(LOG_TAG "%s: Failed to receive camera info, err: %s ", __FUNCTION__, strerror(errno));
        goto out;
    }

    ALOGI(LOG_TAG "%s: Received CAMERA_INFO packet from client with recv_size: %zd ", __FUNCTION__,
          recv_size);
    ALOGI(LOG_TAG "%s: Number of cameras requested = %d", __FUNCTION__, mNumOfCamerasRequested);

    // validate capability info received from the client.
    for (int i = 0; i < mNumOfCamerasRequested; i++) {
        expctd_cam_id = i;
        if (expctd_cam_id == (int)camera_info[i].cameraId)
            ALOGE(LOG_TAG
                   "%s: Camera Id number %u received from client is matching with expected Id",
                   __FUNCTION__, camera_info[i].cameraId);
        else
            ALOGE(LOG_TAG
                  "%s: [Warning] Camera Id number %u received from client is not matching with "
                  "expected Id %d",
                  __FUNCTION__, camera_info[i].cameraId, expctd_cam_id);

        ALOGI("received codec type %d", camera_info[i].codec_type);
        switch (camera_info[i].codec_type) {
            case uint32_t(VideoCodecType::kH264):
                //gIsInFrameH264 = true;
                val_client_cap[i].validCodecType = true;
                break;
            case uint32_t(VideoCodecType::kI420):
                //gIsInFrameI420 = true;
                val_client_cap[i].validCodecType = true;
                break;
            case uint32_t(VideoCodecType::kMJPEG):
                //gIsInFrameMJPG = true;
                val_client_cap[i].validCodecType = true;
                break;
            default:
                val_client_cap[i].validCodecType = false;
                break;
        }

        switch (camera_info[i].resolution) {
            case uint32_t(FrameResolution::k480p):
            case uint32_t(FrameResolution::k720p):
            case uint32_t(FrameResolution::k1080p):
                val_client_cap[i].validResolution = true;
                break;
            default:
                val_client_cap[i].validResolution = false;
                break;
        }

        switch (camera_info[i].sensorOrientation) {
            case uint32_t(SensorOrientation::ORIENTATION_0):
            case uint32_t(SensorOrientation::ORIENTATION_90):
            case uint32_t(SensorOrientation::ORIENTATION_180):
            case uint32_t(SensorOrientation::ORIENTATION_270):
                val_client_cap[i].validOrientation = true;
                break;
            default:
                val_client_cap[i].validOrientation = false;
                break;
        }

        switch (camera_info[i].facing) {
            case uint32_t(CameraFacing::BACK_FACING):
            case uint32_t(CameraFacing::FRONT_FACING):
                val_client_cap[i].validCameraFacing = true;
                break;
            default:
                val_client_cap[i].validCameraFacing = false;
                break;
        }
    }


    // Check whether recceived any invalid capability info or not.
    // ACK packet to client would be updated based on this verification.
    for (int i = 0; i < mNumOfCamerasRequested; i++) {
        if (!val_client_cap[i].validCodecType || !val_client_cap[i].validResolution ||
            !val_client_cap[i].validOrientation || !val_client_cap[i].validCameraFacing) {
            valid_client_cap_info = false;
            ALOGE("%s: capability info received from client is not completely correct and expected",
                  __FUNCTION__);
            break;
        } else {
            ALOGE("%s: capability info received from client is correct and expected",
                   __FUNCTION__);
            valid_client_cap_info = true;
        }
    }

    // Updating metadata for each camera seperately with its capability info received.
    for (int i = 0; i < mNumOfCamerasRequested; i++) {
        camera_id = i;
        ALOGI(LOG_TAG
              "%s - Client requested for codec_type: %s, resolution: %s, orientation: %u, and "
              "facing: %u for camera Id %d",
              __FUNCTION__, codec_type_to_str(camera_info[i].codec_type),
              resolution_to_str(camera_info[i].resolution), camera_info[i].sensorOrientation,
              camera_info[i].facing, camera_id);
        // Start updating metadata for one camera, so update the status.

        // Wait till complete the metadata update for a camera.
        while (mCallback == nullptr) {
            ALOGE("%s: wait till complete the metadata update for a camera", __FUNCTION__);
            // 200us sleep for this thread.
            usleep(20000);
        }
        std::string deviceName =
            std::string("device@") + ExternalCameraDevice::kDeviceVersion + "/external/"+ std::string("127");
        mCameraStatusMap[deviceName] = CameraDeviceStatus::PRESENT;
        if (mCallback != nullptr) {
            mCallback->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::PRESENT);
        }

    }

    ack_packet = (camera_packet_t *)malloc(ack_packet_size);
    if (ack_packet == NULL) {
        ALOGE(LOG_TAG "%s: ack camera_packet_t allocation failed: %d ", __FUNCTION__, __LINE__);
        goto out;
    }
    ack_payload = (valid_client_cap_info) ? ACK_CONFIG : NACK_CONFIG;

    ack_packet->header.type = ACK;
    ack_packet->header.size = sizeof(camera_ack_t);

    memcpy(ack_packet->payload, &ack_payload, sizeof(camera_ack_t));
    if (send(mClientFd, ack_packet, ack_packet_size, 0) < 0) {
        ALOGE(LOG_TAG "%s: Failed to send camera capabilities, err: %s ", __FUNCTION__,
              strerror(errno));
        goto out;
    }
    ALOGI(LOG_TAG "%s: Sent ACK packet to client with ack_size: %zu ", __FUNCTION__,
          ack_packet_size);
out:
    free(ack_packet);
    free(cap_packet);
    return true;
}
void *RemoteThreadFun(void *argv)
{
    bool status;
    struct sockaddr_un addr_un;
    memset(&addr_un, 0, sizeof(addr_un));
    addr_un.sun_family = AF_UNIX;
    ExternalCameraProvider *threadHandle = (ExternalCameraProvider*)argv;
    int transMode = 1;
    struct sockaddr_vm addr_vm ;
    if(transMode == 1) {
        addr_vm.svm_family = AF_VSOCK;
        addr_vm.svm_port = 1982;
        addr_vm.svm_cid = 3;
        int ret = 0;
        ALOGI("Waiting for connection ");
        int mSocketServerFd = ::socket(AF_VSOCK, SOCK_STREAM, 0);
        if (mSocketServerFd < 0) {
            ALOGV(LOG_TAG " %s:Line:[%d] Fail to construct camera socket with error: [%s]",
            __FUNCTION__, __LINE__, strerror(errno));
            return NULL;
        }
        ret = ::bind(mSocketServerFd, (struct sockaddr *)&addr_vm,
            sizeof(struct sockaddr_vm));
        if (ret < 0) {
            ALOGV(LOG_TAG " %s Failed to bind port(%d). ret: %d, %s", __func__, addr_vm.svm_port, ret,
            strerror(errno));
            close(mSocketServerFd);
	    return NULL;
        }
        ret = listen(mSocketServerFd, 32);
        if (ret < 0) {
            ALOGV("%s Failed to listen on ", __FUNCTION__);
            close(mSocketServerFd);
	    return NULL;
        }

        socklen_t alen = sizeof(struct sockaddr_un);
        threadHandle->mClientFd = ::accept(mSocketServerFd, (struct sockaddr *)&addr_un, &alen);
        ALOGE("Vsock connected");
        if (threadHandle->mClientFd > 0) {
            status = threadHandle->configureCapabilities();
            if(!status) {
                ALOGE("Fail to configure ");
            }
        }
        close(mSocketServerFd);
    }

    pthread_join(thread_id, NULL);
    return argv;
}

ExternalCameraProvider::ExternalCameraProvider() : mCfg(ExternalCameraConfig::loadFromCfg()) {
    mHotPlugThread = std::make_shared<HotplugThread>(this);
    mHotPlugThread->run();
    pthread_create(&thread_id, NULL, RemoteThreadFun, this);
}

ExternalCameraProvider::~ExternalCameraProvider() {
    mHotPlugThread->requestExitAndWait();
}

ndk::ScopedAStatus ExternalCameraProvider::setCallback(
        const std::shared_ptr<ICameraProviderCallback>& in_callback) {
    {
        Mutex::Autolock _l(mLock);
        mCallback = in_callback;
    }

    if (mCallback == nullptr) {
        return fromStatus(Status::OK);
    }

    for (const auto& pair : mCameraStatusMap) {
        mCallback->cameraDeviceStatusChange(pair.first, pair.second);
    }
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getVendorTags(
        std::vector<VendorTagSection>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // No vendor tag support for USB camera
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getCameraIdList(std::vector<std::string>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // External camera HAL always report 0 camera, and extra cameras
    // are just reported via cameraDeviceStatusChange callbacks
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getCameraDeviceInterface(
        const std::string& in_cameraDeviceName, std::shared_ptr<ICameraDevice>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    std::string cameraDevicePath, deviceVersion;
    bool match = matchDeviceName(mCfg.cameraIdOffset, in_cameraDeviceName, &deviceVersion,
                                 &cameraDevicePath);

    if (!match) {
        *_aidl_return = nullptr;
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    if (mCameraStatusMap.count(in_cameraDeviceName) == 0 ||
        mCameraStatusMap[in_cameraDeviceName] != CameraDeviceStatus::PRESENT) {
        *_aidl_return = nullptr;
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }

    ALOGI("Constructing external camera device %s enumerate", cameraDevicePath.c_str());

    std::string delimiter = "/";
    std::vector<std::string> camId = split (in_cameraDeviceName, delimiter);
    
    if(std::stoi(camId[2]) >= 127) {
        
        std::shared_ptr<RemoteCameraDevice> deviceImpl =
                ndk::SharedRefBase::make<RemoteCameraDevice>(camId[2], mClientFd, mCfg);
        if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
            ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraDevicePath.c_str());
            *_aidl_return = nullptr;
            return fromStatus(Status::INTERNAL_ERROR);
        }
        
        IF_ALOGV() {
            int interfaceVersion;
            deviceImpl->getInterfaceVersion(&interfaceVersion);
            ALOGV("%s: device interface version: %d", __FUNCTION__, interfaceVersion);
        }

        *_aidl_return = deviceImpl;

    }else {
        std::shared_ptr<ExternalCameraDevice> deviceImpl =
                ndk::SharedRefBase::make<ExternalCameraDevice>(cameraDevicePath, mCfg);
        if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
            ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraDevicePath.c_str());
            *_aidl_return = nullptr;
            return fromStatus(Status::INTERNAL_ERROR);
        }
        
        IF_ALOGV() {
            int interfaceVersion;
            deviceImpl->getInterfaceVersion(&interfaceVersion);
            ALOGV("%s: device interface version: %d", __FUNCTION__, interfaceVersion);
        }

        *_aidl_return = deviceImpl;
    }
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::notifyDeviceStateChange(int64_t) {
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::getConcurrentCameraIds(
        std::vector<ConcurrentCameraIdCombination>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    *_aidl_return = {};
    return fromStatus(Status::OK);
}

ndk::ScopedAStatus ExternalCameraProvider::isConcurrentStreamCombinationSupported(
        const std::vector<CameraIdAndStreamCombination>&, bool* _aidl_return) {
    if (_aidl_return == nullptr) {
        return fromStatus(Status::ILLEGAL_ARGUMENT);
    }
    // No concurrent stream combinations are supported
    *_aidl_return = false;
    return fromStatus(Status::OK);
}

void ExternalCameraProvider::addExternalCamera(const char* devName) {
    ALOGV("%s: ExtCam: adding %s to External Camera HAL!", __FUNCTION__, devName);
    Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId =
            std::to_string(mCfg.cameraIdOffset + std::atoi(devName + kDevicePrefixLen));
    deviceName =
            std::string("device@") + ExternalCameraDevice::kDeviceVersion + "/external/" + cameraId;
    mCameraStatusMap[deviceName] = CameraDeviceStatus::PRESENT;
    if (mCallback != nullptr) {
        mCallback->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::PRESENT);
    }
}

void ExternalCameraProvider::deviceAdded(const char* devName) {
    int status = 0;
    // sometimes device nodes not enumated hence it fails retry before confirm
    for (int i = 0; i < MAX_RETRY; i++) {
        if (status == 1)
            break; 
        base::unique_fd fd(::open(devName, O_RDWR));
        if (fd.get() < 0) {
            ALOGE("%s open v4l2 device %s failed:%s and iteration %d", __FUNCTION__, devName, strerror(errno), i);
            if(usleep(200000) < 0) {
                ALOGE("%s Failed to sleep %s :%s and iteration %d", __FUNCTION__, devName, strerror(errno), i);
            }
            continue;
        }
        status = 1;
        struct v4l2_capability capability;
        int ret = ioctl(fd.get(), VIDIOC_QUERYCAP, &capability);
        if (ret < 0) {
            ALOGE("%s v4l2 QUERYCAP %s failed", __FUNCTION__, devName);
            return;
        }

        if (!(capability.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
            ALOGW("%s device %s does not support VIDEO_CAPTURE", __FUNCTION__, devName);
            return;
        }
    }

    // See if we can initialize ExternalCameraDevice correctly
    std::shared_ptr<ExternalCameraDevice> deviceImpl =
            ndk::SharedRefBase::make<ExternalCameraDevice>(devName, mCfg);
    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGW("%s: Attempt to init camera device %s failed!", __FUNCTION__, devName);
        return;
    }
    deviceImpl.reset();
    addExternalCamera(devName);
}

void ExternalCameraProvider::deviceRemoved(const char* devName) {
    Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId =
            std::to_string(mCfg.cameraIdOffset + std::atoi(devName + kDevicePrefixLen));

    deviceName =
            std::string("device@") + ExternalCameraDevice::kDeviceVersion + "/external/" + cameraId;

    if (mCameraStatusMap.erase(deviceName) == 0) {
        // Unknown device, do not fire callback
        ALOGE("%s: cannot find camera device to remove %s", __FUNCTION__, devName);
        return;
    }

    if (mCallback != nullptr) {
        mCallback->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::NOT_PRESENT);
    }
}

void ExternalCameraProvider::updateAttachedCameras() {
    ALOGV("%s start scanning for existing V4L2 devices", __FUNCTION__);

    // Find existing /dev/video* devices
    DIR* devdir = opendir(kDevicePath);
    if (devdir == nullptr) {
        ALOGE("%s: cannot open %s! Exiting threadloop", __FUNCTION__, kDevicePath);
        return;
    }

    struct dirent* de;
    while ((de = readdir(devdir)) != nullptr) {
        // Find external v4l devices that's existing before we start watching and add them
        if (!strncmp(kPrefix, de->d_name, kPrefixLen)) {
            std::string deviceId(de->d_name + kPrefixLen);
            if (mCfg.mInternalDevices.count(deviceId) == 0) {
                ALOGV("Non-internal v4l device %s found", de->d_name);
                char v4l2DevicePath[kMaxDevicePathLen];
                snprintf(v4l2DevicePath, kMaxDevicePathLen, "%s%s", kDevicePath, de->d_name);
                deviceAdded(v4l2DevicePath);
            }
        }
    }
    closedir(devdir);
}

// Start ExternalCameraProvider::HotplugThread functions

ExternalCameraProvider::HotplugThread::HotplugThread(ExternalCameraProvider* parent)
    : mParent(parent), mInternalDevices(parent->mCfg.mInternalDevices) {}

ExternalCameraProvider::HotplugThread::~HotplugThread() {
    // Clean up inotify descriptor if needed.
    if (mINotifyFD >= 0) {
        close(mINotifyFD);
    }
}

bool ExternalCameraProvider::HotplugThread::initialize() {
    // Update existing cameras
    mParent->updateAttachedCameras();

    // Set up non-blocking fd. The threadLoop will be responsible for polling read at the
    // desired frequency
    mINotifyFD = inotify_init();
    if (mINotifyFD < 0) {
        ALOGE("%s: inotify init failed! Exiting threadloop", __FUNCTION__);
        return false;
    }

    // Start watching /dev/ directory for created and deleted files
    mWd = inotify_add_watch(mINotifyFD, kDevicePath, IN_CREATE | IN_DELETE);
    if (mWd < 0) {
        ALOGE("%s: inotify add watch failed! Exiting threadloop", __FUNCTION__);
        return false;
    }

    mPollFd = {.fd = mINotifyFD, .events = POLLIN};

    mIsInitialized = true;
    return true;
}

bool ExternalCameraProvider::HotplugThread::threadLoop() {
    // Initialize inotify descriptors if needed.
    if (!mIsInitialized && !initialize()) {
        return true;
    }

    // poll /dev/* and handle timeouts and error
    int pollRet = poll(&mPollFd, /* fd_count= */ 1, /* timeout= */ 250);
    if (pollRet == 0) {
        // no read event in 100ms
        mPollFd.revents = 0;
        return true;
    } else if (pollRet < 0) {
        ALOGE("%s: error while polling for /dev/*: %d", __FUNCTION__, errno);
        mPollFd.revents = 0;
        return true;
    } else if (mPollFd.revents & POLLERR) {
        ALOGE("%s: polling /dev/ returned POLLERR", __FUNCTION__);
        mPollFd.revents = 0;
        return true;
    } else if (mPollFd.revents & POLLHUP) {
        ALOGE("%s: polling /dev/ returned POLLHUP", __FUNCTION__);
        mPollFd.revents = 0;
        return true;
    } else if (mPollFd.revents & POLLNVAL) {
        ALOGE("%s: polling /dev/ returned POLLNVAL", __FUNCTION__);
        mPollFd.revents = 0;
        return true;
    }
    // mPollFd.revents must contain POLLIN, so safe to reset it before reading
    mPollFd.revents = 0;

    uint64_t offset = 0;
    ssize_t ret = read(mINotifyFD, mEventBuf, sizeof(mEventBuf));
    if (ret < sizeof(struct inotify_event)) {
        // invalid event. skip
        return true;
    }

    while (offset < ret) {
        struct inotify_event* event = (struct inotify_event*)&mEventBuf[offset];
        offset += sizeof(struct inotify_event) + event->len;

        if (event->wd != mWd) {
            // event for an unrelated descriptor. ignore.
            continue;
        }

        ALOGV("%s inotify_event %s", __FUNCTION__, event->name);
        if (strncmp(kPrefix, event->name, kPrefixLen) != 0) {
            // event not for /dev/video*. ignore.
            continue;
        }

        std::string deviceId = event->name + kPrefixLen;
        if (mInternalDevices.count(deviceId) != 0) {
            // update to an internal device. ignore.
            continue;
        }

        char v4l2DevicePath[kMaxDevicePathLen];
        snprintf(v4l2DevicePath, kMaxDevicePathLen, "%s%s", kDevicePath, event->name);

        if (event->mask & IN_CREATE) {
            mParent->deviceAdded(v4l2DevicePath);
        } else if (event->mask & IN_DELETE) {
            mParent->deviceRemoved(v4l2DevicePath);
        }
    }
    return true;
}

// End ExternalCameraProvider::HotplugThread functions

}  // namespace implementation
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
