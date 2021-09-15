//
// Copyright 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "h4_protocol.h"

#define LOG_TAG "android.hardware.bluetooth-hci-h4"

#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <asm/byteorder.h>
#include <linux/usb/ch9.h>
#include <libusb/libusb.h>

#define HCI_COMMAND_COMPLETE_EVT 0x0E
#define HCI_COMMAND_STATUS_EVT 0x0F
#define HCI_ESCO_CONNECTION_COMP_EVT 0x2C
#define HCI_RESET_SUPPORTED(x) ((x)[5] & 0x80)
#define HCI_GRP_INFORMATIONAL_PARAMS (0x04 << 10)    /* 0x1000 */
#define HCI_READ_LOCAL_SUPPORTED_CMDS (0x0002 | HCI_GRP_INFORMATIONAL_PARAMS)
#define HCI_GRP_HOST_CONT_BASEBAND_CMDS (0x03 << 10) /* 0x0C00 */
#define HCI_RESET (0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)

#define INTEL_VID 0x8087
#define INTEL_PID_8265 0x0a2b // Windstorm peak
#define INTEL_PID_3168 0x0aa7 //SandyPeak (SdP)
#define INTEL_PID_9260 0x0025 // 9160/9260 (also known as ThunderPeak)
#define INTEL_PID_9560 0x0aaa // 9460/9560 also know as Jefferson Peak (JfP)
#define INTEL_PID_AX201 0x0026 // AX201 also know as Harrison Peak (HrP)
#define INTEL_PID_AX211 0x0033 // AX211 also know as GarfieldPeak (Gfp)

#include <errno.h>
#include <fcntl.h>
#include <log/log.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace hci {

size_t H4Protocol::Send(uint8_t type, const uint8_t* data, size_t length){
    /* For HCI communication over USB dongle, multiple write results in
     * response timeout as driver expect type + data at once to process
     * the command, so using "writev"(for atomicity) here.
     */
    struct iovec iov[2];
    ssize_t ret = 0;
    iov[0].iov_base = &type;
    iov[0].iov_len = sizeof(type);
    iov[1].iov_base = (void *)data;
    iov[1].iov_len = length;

    ALOGV("%x %x %x", data[0],data[1],data[2]);
    while (1) {
        ret = TEMP_FAILURE_RETRY(writev(uart_fd_, iov, 2));
        if (ret == -1) {
            if (errno == EAGAIN) {
                ALOGE("%s error writing to UART (%s)", __func__, strerror(errno));
                continue;
            }
        } else if (ret == 0) {
            ALOGE("%s zero bytes written - something went wrong...", __func__);
            break;
        }
        break;
    }
    return ret;
}

bool H4Protocol::IsIntelController(uint16_t vid, uint16_t pid) {
    if ((vid == INTEL_VID) && ((pid == INTEL_PID_8265) ||
                                (pid == INTEL_PID_3168)||
                                (pid == INTEL_PID_9260)||
                                (pid == INTEL_PID_9560)||
                                (pid == INTEL_PID_AX201)||
                                (pid == INTEL_PID_AX211)))
        return true;
    else
	return false;
}

int H4Protocol::GetUsbpath(void) {
    size_t count, i;
    int ret = 0, busnum, devnum;
    struct libusb_device **dev_list = NULL;
    struct libusb_context *ctx;
    uint16_t vid = 0, pid = 0;
    ALOGD(" Initializing GenericUSB (libusb-1.0)...\n");
    ret = libusb_init(&ctx);
    if (ret < 0) {
        ALOGE("libusb failed to initialize: %d\n", ret);
        return ret;
    }
    count = libusb_get_device_list(ctx, &dev_list);
    if (count <= 0) {
        ALOGE("Error getting USB device list: %s\n", strerror(count));
        goto exit;
    }
    for (i = 0; i < count; ++i) {
        struct libusb_device* dev = dev_list[i];
        busnum = libusb_get_bus_number(dev);
        devnum = libusb_get_device_address(dev);
        struct libusb_device_descriptor descriptor;
        ret = libusb_get_device_descriptor(dev, &descriptor);
        if (ret < 0)  {
            ALOGE("Error getting device descriptor %d ", ret);
            goto exit;
        }
        vid = descriptor.idVendor;
        pid = descriptor.idProduct;
        if (H4Protocol::IsIntelController(vid, pid)) {
            snprintf(dev_address, sizeof(dev_address), "/dev/bus/usb/%03d/%03d",
                                                       busnum, devnum);
            ALOGV("Value of BT device address = %s", dev_address);
            goto exit;
        }
    }
exit:
    libusb_free_device_list(dev_list, count);
    libusb_exit(ctx);
    return ret;
}

int H4Protocol::SendHandle(void) {
    int fd, ret = 0;
    fd = open(dev_address,O_WRONLY|O_NONBLOCK);
    if (fd < 0) {
        ALOGE("Fail to open USB device %s, value of fd= %d", dev_address, fd);
        return -1;
    } else {
        struct usbdevfs_ioctl   wrapper;
        wrapper.ifno = 1;
        wrapper.ioctl_code = USBDEVFS_IOCTL;
        wrapper.data = sco_handle;
        ret = ioctl(fd, USBDEVFS_IOCTL, &wrapper);
        if (ret < 0)
            ALOGE("Failed to send SCO handle err = %d", ret);
        close(fd);
        return ret;
    }
}

void H4Protocol::OnPacketReady() {
  int ret = 0;
  switch (hci_packet_type_) {
    case HCI_PACKET_TYPE_EVENT:
      if (hci_packetizer_.GetPacket() != NULL) {
          if ((hci_packetizer_.GetPacket())[0] == HCI_COMMAND_COMPLETE_EVT) {
              unsigned int cmd, lsb, msb;
              msb = hci_packetizer_.GetPacket()[4] ;
              lsb = hci_packetizer_.GetPacket()[3];
              cmd = msb << 8 | lsb ;
              if (cmd == HCI_RESET) {
                  event_cb_(hci_packetizer_.GetPacket());
                  hci_packet_type_ = HCI_PACKET_TYPE_UNKNOWN;
                  ret = H4Protocol::GetUsbpath();
                  if (ret < 0)
                      ALOGE("Failed to get the USB path for btusb-sound-card");
                  break;
              }
          } else if ((hci_packetizer_.GetPacket())[0] == HCI_ESCO_CONNECTION_COMP_EVT) {
              const unsigned char *handle = hci_packetizer_.GetPacket().data() + 3;
              memcpy(sco_handle, handle, 2);
              ALOGI("Value of SCO handle = %x, %x", handle[0], handle[1]);
              ret = H4Protocol::SendHandle();
              if (ret < 0)
                  ALOGE("Failed to send SCO handle to btusb-sound-card driver");
          }
      }

      event_cb_(hci_packetizer_.GetPacket());
      break;
    case HCI_PACKET_TYPE_ACL_DATA:
      acl_cb_(hci_packetizer_.GetPacket());
      break;
    case HCI_PACKET_TYPE_SCO_DATA:
      sco_cb_(hci_packetizer_.GetPacket());
      break;
    default:
      LOG_ALWAYS_FATAL("%s: Unimplemented packet type %d", __func__,
                       static_cast<int>(hci_packet_type_));
  }
  // Get ready for the next type byte.
  hci_packet_type_ = HCI_PACKET_TYPE_UNKNOWN;
}


typedef struct
{
    uint8_t         type;
    uint8_t         event;
    uint8_t         len;
    uint8_t         offset;
    uint16_t        layer_specific;
} BT_EVENT_HDR;

void H4Protocol::OnDataReady(int fd) {
    if (hci_packet_type_ == HCI_PACKET_TYPE_UNKNOWN) {
        /*
         * read full buffer. ACL max length is 2 bytes, and SCO max length is 2
         * byte. so taking 64K as buffer length.
         * Question : Why to read in single chunk rather than multiple reads,
         * which can give parameter length arriving in response ?
         * Answer: The multiple reads does not work with BT USB dongle. At least
         * with Bluetooth 2.0 supported USB dongle. After first read, either
         * firmware/kernel (do not know who is responsible - inputs ??) driver
         * discard the whole message and successive read results in forever
         * blocking loop. - Is there any other way to make it work with multiple
         * reads, do not know yet (it can eliminate need of this function) ?
         * Reading in single shot gives expected response.
         */
        const size_t max_plen = 64*1024;
        hidl_vec<uint8_t> tpkt;
        tpkt.resize(max_plen);
        size_t bytes_read = TEMP_FAILURE_RETRY(read(fd, tpkt.data(), max_plen));
        if (bytes_read == 0) {
            // This is only expected if the UART got closed when shutting down.
            ALOGE("%s: Unexpected EOF reading the packet type!", __func__);
            sleep(5);  // Expect to be shut down within 5 seconds.
            return;
        } else if (bytes_read < 0) {
            LOG_ALWAYS_FATAL("%s: Read packet type error: %s", __func__,
                         strerror(errno));
        }
        hci_packet_type_ = static_cast<HciPacketType>(tpkt.data()[0]);
        if (hci_packet_type_ != HCI_PACKET_TYPE_ACL_DATA &&
            hci_packet_type_ != HCI_PACKET_TYPE_SCO_DATA &&
            hci_packet_type_ != HCI_PACKET_TYPE_EVENT) {
          LOG_ALWAYS_FATAL("%s: Unimplemented packet type %d", __func__,
                           static_cast<int>(hci_packet_type_));
        } else {
            if(tpkt.data()[1] == HCI_COMMAND_COMPLETE_EVT) {
                ALOGD("%s Command complete event ncmds = %d",
                                                     __func__, tpkt.data()[3]);
                tpkt.data()[3] = 1;
		/* Disable Enhance setup synchronous connections*/
                BT_EVENT_HDR* hdr  = (BT_EVENT_HDR*)(tpkt.data());
                if( hdr->layer_specific == HCI_READ_LOCAL_SUPPORTED_CMDS)
                        tpkt.data()[36] &= ~((uint8_t)0x1 << 3);

            } else if (tpkt.data()[1] ==  HCI_COMMAND_STATUS_EVT) {
                ALOGV("%s Command status event ncmd = %d", __func__, tpkt.data()[4]);
                tpkt.data()[4] = 1;
            }

            hci_packetizer_.CbHciPacket(tpkt.data() + 1, bytes_read - 1);
        }
    }
}

}  // namespace hci
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
