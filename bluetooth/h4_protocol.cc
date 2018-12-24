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
#include "esco_parameters.h"
#include "hcidefs.h"

#define LOG_TAG "android.hardware.bluetooth-hci-h4"

typedef uint8_t UINT8;
typedef uint16_t UINT16;

#define UINT16_TO_STREAM(p, u16) {*(p)++ = (UINT8)(u16); *(p)++ = (UINT8)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (UINT8)(u8);}
#define STREAM_TO_UINT8(u8, p)   {u8 = (UINT8)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16, p) {u16 = ((UINT16)(*(p)) + (((UINT16)(*((p) + 1))) << 8)); (p) += 2;}

#define T2_MAXIMUM_LATENCY                        0x000D
#define HCIC_PARAM_SIZE_ENH_ACC_ESCO_CONN         63

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

    if (type == HCI_PACKET_TYPE_COMMAND) {
        uint8_t* p;
        void* r;
        uint8_t* q;
        uint16_t command;
        uint8_t coding_format;
        /* Marvell specific  Configuration */
        const uint16_t input_coded_data_size = 8;
        const uint16_t output_coded_data_size = 8;
        const uint8_t input_transport_unit_size = 16;
        const uint8_t output_transport_unit_size = 16;
        const uint8_t packet_size = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_ENH_ACC_ESCO_CONN;
        const uint16_t max_latency_ms = 13;

        p = (uint8_t*)data;
        STREAM_TO_UINT16(command, p);
        p += 15; // Increment stream pinter to point coding format byte
        STREAM_TO_UINT8(coding_format, p);

        if ((command == HCI_ENH_ACCEPT_ESCO_CONNECTION) &&
                                           (coding_format == ESCO_CODING_FORMAT_MSBC)) {
            ALOGV("%s accept esco", __func__);
            q = (uint8_t*)malloc(packet_size);

            if (q == NULL) {
                ALOGE("%s Memory allocation for SCO config parameters failed", __func__);
            } else {
                r = q;
                memcpy(q, data, packet_size);
                q += 49; // Increment stream pointer to point to input_coded_data_size
                UINT16_TO_STREAM(q, input_coded_data_size);
                UINT16_TO_STREAM(q, output_coded_data_size);
                q += 6; // Increment stream pointer to point to input_transport_unit_size
                UINT8_TO_STREAM(q, input_transport_unit_size);
                UINT8_TO_STREAM(q, output_transport_unit_size);
                UINT16_TO_STREAM(q, max_latency_ms);
                /* Write T2 specific Settings */
                UINT16_TO_STREAM(q, (ESCO_PKT_TYPES_MASK_EV3 | ESCO_PKT_TYPES_MASK_NO_3_EV3 |
                      ESCO_PKT_TYPES_MASK_NO_2_EV5 | ESCO_PKT_TYPES_MASK_NO_3_EV5));

                iov[1].iov_base = r;
            }
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
            free (q);
            return ret;
        }
    }

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

void H4Protocol::OnPacketReady() {
  switch (hci_packet_type_) {
    case HCI_PACKET_TYPE_EVENT:
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
                ALOGV("%s Command complete event ncmds = %d", __func__, tpkt.data()[3]);
                tpkt.data()[3] = 1;
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
