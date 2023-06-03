/*
 * Copyright 2020 Intel Corporation
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

#include <errno.h>
#include <fcntl.h>
#include <log/log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <trusty/tipc.h>
#include "trusty_tipc_cmd.h"
#include <unistd.h>

#define MAX_FILE_SIZE (16 * 1024)
/* Align with keybox fastboot cmd */
#define KB_HEAD_OFFSET     0
/* Align with KM TA */
static const char end_str[] = "#END_SIG#";
static int tee_fd = -1;

static ssize_t write_with_retry(int fd, const void *buf_, size_t size, off_t offset)
{
    ssize_t rc;
    const uint8_t *buf = buf_;

    while (size > 0) {
        rc = TEMP_FAILURE_RETRY(pwrite(fd, buf, size, offset));
        if (rc < 0)
            return rc;
        size -= rc;
        buf += rc;
        offset += rc;
    }

    rc = fsync(fd);
    if (rc < 0) {
        ALOGE("fsync for fd=%d failed: %s\n", fd, strerror(errno));
        return rc;
    }

    return 0;
}

static ssize_t read_with_retry(int fd, void *buf_, size_t size, off_t offset)
{
    ssize_t rc;
    size_t  rcnt = 0;
    uint8_t *buf = buf_;

    while (size > 0) {
        rc = TEMP_FAILURE_RETRY(pread(fd, buf, size, offset));
        if (rc < 0)
            return rc;
        if (rc == 0)
            break;
        size -= rc;
        buf += rc;
        offset += rc;
        rcnt += rc;
    }
    return rcnt;
}

static int read_keybox_from_teedata(const char *keybox_path, char *out, size_t *out_size)
{
    int rc;
    struct keybox_header header;

    rc = open(keybox_path, O_RDWR, 0);
    if (rc < 0) {
        ALOGE("%s: unable (%d) to open keybox file '%s': %s\n",
              __func__, errno, keybox_path, strerror(errno));
        return rc;
    }
    tee_fd = rc;

    rc = read_with_retry(tee_fd, &header, sizeof(struct keybox_header), KB_HEAD_OFFSET);
    if (rc < 0) {
        ALOGE("%s read failed.\n", __func__);
        return -1;
    }

    if (memcmp("MAGICKEYBOX", header.magic, sizeof(header.magic))) {
        ALOGD("Keybox not found. exit...\n");
        return -1;
    }

    ALOGI("keybox size is %u.\n", header.size);

    if (header.size > MAX_FILE_SIZE) {
        ALOGE("keybox size is too big %u.\n", header.size);
        return -1;
    }

    *out_size = header.size;
    read_with_retry(tee_fd, out, header.size, sizeof(struct keybox_header));

    /* Append the end string to the keybox ending as the flag of finishing transfer */
    memcpy(((uint8_t *)out + *out_size), end_str, sizeof(end_str));
    *out_size += sizeof(end_str);

    return 0;
}

bool provision_keybox(const char* trusty_devname, const char *keybox_path)
{
    int rc = -1;
    bool ret = false;
    keymaster_message_t *msg = NULL;
    size_t keybox_req_size = 0;
    size_t msg_size = 0;
    size_t file_size = 0;
    char buf[MAX_FILE_SIZE + sizeof(end_str)] = {0};
    char recv_buf[PAGE_SIZE] = {0};
    keybox_provision_req_t *keybox_req;
    int tipc_fd = -1;

    if (keybox_path == NULL || trusty_devname == NULL)
        return false;

    rc = read_keybox_from_teedata(keybox_path, buf, &file_size);
    if (rc < 0) {
        ALOGE("Retrieve keybox from teedata: FAIL\n");
        goto clean;
    }

    /* A resonable keybox size is limited to 16KB */
    if (file_size + sizeof(keybox_provision_req_t) + sizeof(keymaster_message_t) > MAX_FILE_SIZE) {
        ALOGE("Keybox size(%zu) is over-sized.\n", file_size);
        goto clean;
    }

    size_t max_transfer_size = PAGE_SIZE - sizeof(struct tipc_msg_hdr) - sizeof(keybox_provision_req_t) - sizeof(keymaster_message_t);
    size_t remaining_size = file_size;
    size_t transfer_size = 0;
    uint8_t* pbuf = (uint8_t*)buf;

    ALOGI("keybox max_transfer_size is %zu.\n", max_transfer_size);

    while (remaining_size)
    {
        /* Connect to Keymaster TA */
        tipc_fd = tipc_connect(trusty_devname, KEYMASTER_PORT);
        if (tipc_fd < 0) {
            ALOGE("keybox Connect to tipc device(%s): FAIL\n", trusty_devname);
            goto clean;
        }
        ALOGI("keybox Connect to tipc device: PASS. tipc_fd is %d.\n", tipc_fd);

        pbuf += transfer_size;
        transfer_size = remaining_size > max_transfer_size ? max_transfer_size : remaining_size;
        remaining_size -= transfer_size;

        ALOGI("keybox transfer_size is %zu, remaining_size is %zu.\n", transfer_size, remaining_size);

        keybox_req_size = sizeof(keybox_provision_req_t) + transfer_size;
        keybox_req = malloc(keybox_req_size);
        if (keybox_req == NULL)
            goto clean;

        msg_size = sizeof(keymaster_message_t) + keybox_req_size;
        msg = (keymaster_message_t *)malloc(msg_size);
        if (msg == NULL) {
            free(keybox_req);
            goto clean;
        }

        /* prepare Keybox request structure */
        keybox_req->keybox_size = transfer_size;
        memcpy(keybox_req->keybox, pbuf, transfer_size);

        /* Prepare message sent to TA */
        msg->cmd = KM_PROVISION_KEYBOX;
        memcpy(msg->payload, keybox_req, sizeof(keybox_provision_req_t) + transfer_size);

        /* Send message to Keymaster TA */
        rc = write(tipc_fd, msg, msg_size);

        memset(keybox_req, 0, keybox_req_size);
        free(keybox_req);
        memset(msg, 0, msg_size);
        free(msg);

        if (rc < 0) {
            ALOGE("keybox Send cmd(0x%X) to %s!: FAIL(%d)\n", KM_PROVISION_KEYBOX, KEYMASTER_PORT, rc);
            goto clean;
        }

        ALOGI("keybox Send message to KM TA: PASS.\n");

        /* READ RESULT */
        rc = read(tipc_fd, recv_buf, PAGE_SIZE);
        msg = (keymaster_message_t *)recv_buf;

        if (rc < 0) {
            ALOGE("keybox Get response for cmd (0x%X) to %s: FAIL(%d)\n", KM_PROVISION_KEYBOX, KEYMASTER_PORT, rc);
            goto clean;
        }

        if (rc < (int)sizeof(keymaster_message_t) + sizeof(keymaster_error_t)) {
            ALOGE("keybox Invalid response size (%d)!\n", rc);
            goto clean;
        }

        ALOGI("keybox keymaster_error_t size is %zu.\n", sizeof(keymaster_error_t));
        ALOGI("keybox rc is %d. 0x%016lX, cmd is 0x%08X, payload is 0x%08X.\n",
               rc, *(uint64_t *)recv_buf,  msg->cmd, *(keymaster_error_t *)msg->payload);

        if ((KM_PROVISION_KEYBOX | KEYMASTER_RESP_BIT | KEYMASTER_STOP_BIT) != msg->cmd) {
            ALOGE("keybox Invalid command (0x%X)!\n", msg->cmd);
            goto clean;
        }

        if (*(keymaster_error_t *)msg->payload != KM_ERROR_OK) {
            ALOGE("keybox Failure returned: %d.\n", *(keymaster_error_t *)msg->payload);
            goto clean;
        }

        tipc_close(tipc_fd);
        tipc_fd = -1;
    }

    ret = true;

clean:
    memset(buf, 0, sizeof(buf));
    if (tee_fd > 0) {
        write_with_retry(tee_fd, buf, sizeof(buf)  ,0);
        close(tee_fd);
        tee_fd = -1;
    }

    if (tipc_fd > 0) {
        tipc_close(tipc_fd);
        tipc_fd = -1;
    }

    if (ret)
        ALOGI("keybox Provisioning SUCCESSFUL.\n");
    else
        ALOGE("keybox Provisioning FAILED.\n");

    return ret;
}
