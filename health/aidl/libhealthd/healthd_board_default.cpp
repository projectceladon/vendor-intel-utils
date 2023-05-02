/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <healthd/healthd.h>
#include <thread>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include "battery_notifypkt.h"
#include <cutils/klog.h>
#include <unistd.h>
#include <string.h>
#define HLOG_TAG "android.hardware.health@aidl.intel"
#define KLOG_LEVEL 6
#define HEALTH_PORT 14196

static std::thread vsock_thread;
static struct android::BatteryProperties s_props;
static bool is_vsock_present = false;
int healthd_board_battery_update(struct android::BatteryProperties *props);
int vsock_fd = -1;

static void parse_battery_status(uint8_t *status)
{
    if(!strncmp((char *)status, "Unknown", 7))
        s_props.batteryStatus = android::BATTERY_STATUS_UNKNOWN;
    else if(!strncmp((char *)status, "Charging", 8))
        s_props.batteryStatus = android::BATTERY_STATUS_CHARGING;
    else if(!strncmp((char *)status, "Discharging", 11))
        s_props.batteryStatus = android::BATTERY_STATUS_DISCHARGING;
    else if(!strncmp((char *)status, "Not charging", 12))
        s_props.batteryStatus = android::BATTERY_STATUS_NOT_CHARGING;
    else if(!strncmp((char *)status, "Full", 4))
        s_props.batteryStatus = android::BATTERY_STATUS_FULL;
}

static void update_battery_health(struct monitor_pkt *mpkt)
{
    if ((mpkt->charge_full == 0) || (mpkt->charge_full_design == 0)) {
        s_props.batteryHealth = android::BATTERY_HEALTH_DEAD;
    }
    else if ((((mpkt->charge_full)*100)/mpkt->charge_full_design) >= 80) {
        s_props.batteryHealth = android::BATTERY_HEALTH_GOOD;
    }
    else
        s_props.batteryHealth = android::BATTERY_HEALTH_UNKNOWN;
}

static void parse_battery_health(uint8_t *health)
{
    if(!strncmp((char *)health, "Unknown", 7))
        s_props.batteryHealth = android::BATTERY_HEALTH_UNKNOWN;
    else if(!strncmp((char *)health, "Good", 4))
        s_props.batteryHealth = android::BATTERY_HEALTH_GOOD;
    else if(!strncmp((char *)health, "Overheat", 8))
        s_props.batteryHealth = android::BATTERY_HEALTH_OVERHEAT;
    else if(!strncmp((char *)health, "Dead", 4))
        s_props.batteryHealth = android::BATTERY_HEALTH_DEAD;
    else if(!strncmp((char *)health, "Over voltage", 12))
        s_props.batteryHealth = android::BATTERY_HEALTH_OVER_VOLTAGE;
    else if(!strncmp((char *)health, "Unspecified failure", 18))
        s_props.batteryHealth = android::BATTERY_HEALTH_UNSPECIFIED_FAILURE;
    else if(!strncmp((char *)health, "Cold", 4))
        s_props.batteryHealth = android::BATTERY_HEALTH_COLD;
}

static void parse_battery_type(uint8_t *type)
{
    if (!strncmp((char *)type, "Mains", 7))
        s_props.chargerAcOnline = true;
    else if (!strncmp((char *)type, "USB", 3))
        s_props.chargerUsbOnline = true;
}

static void parse_init_properties(struct initial_pkt *ipkt)
{
    s_props.batteryTechnology = (char *)ipkt->technology;
    s_props.batteryPresent = (strncmp((char *)ipkt->present, "1", 1) == 0);
    parse_battery_type(ipkt->type);
}

static void parse_battery_properties(struct monitor_pkt *mpkt)
{
    s_props.batteryLevel = mpkt->capacity;
    s_props.batteryVoltage = mpkt->voltage_now;
    s_props.batteryTemperature = mpkt->temp;
    s_props.batteryFullCharge = mpkt->charge_full;
    s_props.batteryChargeCounter = mpkt->charge_now;
    parse_battery_status(mpkt->status);

    if (strcmp((const char*)mpkt->health,"\0") == 0)
        update_battery_health(mpkt);
    else
        parse_battery_health(mpkt->health);
}

static int connect_vsock(int *vsock_fd) {
    struct sockaddr_vm sa = {
        .svm_family = AF_VSOCK,
        .svm_cid = VMADDR_CID_HOST,
        .svm_port = HEALTH_PORT,
    };
    *vsock_fd = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (*vsock_fd < 0) {
        KLOG_WARNING(HLOG_TAG, "healthd socket init failed\n");
        return *vsock_fd;
    }
    if (connect(*vsock_fd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
            KLOG_WARNING(HLOG_TAG, "healthd connect failed\n");
            close(*vsock_fd);
	    return -1;
    }
    KLOG_INFO(HLOG_TAG, "healthd connect to cid(%d) port(%d)\n", sa.svm_cid, sa.svm_port);
    return 0;
}

static void recv_vsock() {
    char msgbuf[1024];
    struct header *head;
    struct monitor_pkt *mpkt;
    struct initial_pkt *ipkt;

    klog_set_level(KLOG_LEVEL);
    head = (struct header *)malloc(sizeof(struct header));
    if (!head)
        goto exit;
    mpkt = (struct monitor_pkt *)malloc(sizeof(struct monitor_pkt));
    if (!mpkt)
        goto free_head;
    ipkt = (struct initial_pkt *)malloc(sizeof(struct initial_pkt));
    if (!ipkt)
        goto free_mpkt;
    if (connect_vsock(&vsock_fd) == 0)
        is_vsock_present = true;
    while (is_vsock_present && errno != EBADF) {
        int ret;
        memset(msgbuf, 0, sizeof(msgbuf));
        ret =  recv(vsock_fd, msgbuf, sizeof(msgbuf), MSG_DONTWAIT);
        if (ret > 0) {
            memcpy(head, msgbuf, sizeof(struct header));
            if (head->notify_id == 1) {
                memcpy(ipkt, msgbuf + sizeof(struct header), sizeof(struct initial_pkt));
                parse_init_properties(ipkt);
                memcpy(mpkt, msgbuf + sizeof(struct header) + sizeof (struct initial_pkt),
                                                              sizeof(struct monitor_pkt));
                parse_battery_properties(mpkt);
            } else if (head->notify_id == 2) {
                memcpy(mpkt, msgbuf + sizeof(struct header), sizeof(struct monitor_pkt));
                parse_battery_properties(mpkt);
            }
        }
        sleep(1);
    }
    free(ipkt);
free_mpkt:
    free(mpkt);
free_head:
    free(head);
exit:
    KLOG_INFO(HLOG_TAG, "Exiting healthd recv loop...\n");
}

void healthd_board_init(struct healthd_config *config)
{
    config->periodic_chores_interval_fast = 60;
    config->periodic_chores_interval_slow = 60*10;

    vsock_thread = std::thread(recv_vsock);
    vsock_thread.detach();
}

int healthd_board_battery_update(struct android::BatteryProperties *props)
{
    // When batterylevel is 0, host OS would have shutdown.
    // When batterylevel is more than 100, host OS is corrupted.
    if (0 < s_props.batteryLevel && s_props.batteryLevel <= 100) {
        memcpy(props, &s_props, sizeof(struct android::BatteryProperties));
    } else {
	props->batteryStatus = android::BATTERY_STATUS_UNKNOWN;
	props->batteryHealth = android::BATTERY_HEALTH_UNKNOWN;
	props->batteryLevel = 0;
	props->batteryPresent= false;
	props->chargerAcOnline = true;
    }	    
    return 0;
}

bool get_battery_mediation_present(void) {
    return is_vsock_present;
}

