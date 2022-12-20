// Copyright (C) 2022 Intel Corporation
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

#include "cursor.h"
#include "ga-common.h"
#include "encoder-common.h"

// Cursor Server Data
dpipe_t *pCursorPipe = NULL;
HANDLE hCursorServiceThreadHandle = NULL;
static unsigned int gServiceStarted = 0;
// Cursor Client Data
HANDLE hCursorClientThreadHandle = NULL;
static unsigned char gCursorData[MAX_CURSOR_SIZE];
static unsigned int cursorX =0;
static unsigned int cursorY = 0;
static struct sockaddr_in ctrlsin;
static bool bIsReadyforSend = false;
static bool bNeedNewCursorShape = false;
static unsigned int CursorReceiveSeqID = 0;
static bool bRestartCursorServer = false;
SOCKET slisten = -1;
#define TCP 0
//Below is the client implementation
#ifdef WIN32
static unsigned long
#else
static in_addr_t
#endif
name_resolve(const char *hostname) {
    struct in_addr addr;
    struct hostent *hostEnt;
    if ((addr.s_addr = inet_addr(hostname)) == INADDR_NONE) {
        if ((hostEnt = gethostbyname(hostname)) == NULL)
            return INADDR_NONE;
        bcopy(hostEnt->h_addr, (char *)&addr.s_addr, hostEnt->h_length);
    }
    return addr.s_addr;
}
#if TCP
// Main thread to handle the cursor data at client.
DWORD __stdcall ProcessThreadClient(LPVOID params)
{
    int  ret = 0;

    THREAD_INFO_S *pThreadInfo = (THREAD_INFO_S *)params;
    if (NULL == pThreadInfo) {
        ga_logger(Severity::ERR, "Invalid thread config\n");
        return -1;
    }

    //Start to connect to server
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0)
    {
        ga_logger(Severity::ERR, "Failed to initialize Winsock\n");
        return -1;
    }
    SOCKET  m_sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sclient == INVALID_SOCKET)
    {
        ga_logger(Severity::ERR, "Failed to create a socket");
        return -1;
    }
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(pThreadInfo->conf.ctrlport2);// pThreadInfo->nPort);
    if (pThreadInfo->conf.servername != NULL) {
        serAddr.sin_addr.S_un.S_addr = name_resolve(pThreadInfo->conf.servername);
        if (serAddr.sin_addr.S_un.S_addr == INADDR_NONE) {
            ga_logger(Severity::ERR, "Name resolution failed: %s\n", pThreadInfo->conf.servername);
            return -1;
        }
    }

    if (connect(m_sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        ga_logger(Severity::ERR, "connect error !\n");
        closesocket(m_sclient);
        return 0;
    }
    ga_logger(Severity::INFO, "Client connects to server success\n");
    unsigned int    totalLen = 10 * 1024 * 1024;
    unsigned char   *recvBuffer = (unsigned char *)malloc(totalLen);
    unsigned int    datainBuff = 0;
    unsigned int    offset = 0;
    MSG_REP_INFO_S  *repInfo = NULL;
    unsigned int nUpdated = 1;
    int curIndex = 0;
    while (1) {
        //Receive request from client
    start:

        if ((totalLen - offset - datainBuff) == 0) {
            memcpy(recvBuffer, recvBuffer+offset, datainBuff);
            offset = 0;
        }
        if (datainBuff > sizeof(MSG_REP_INFO_S)) {
            repInfo = (MSG_REP_INFO_S *)(recvBuffer + offset);
            if (repInfo->magic == 0x55AA55AA && ((repInfo->payloadLen + sizeof(MSG_REP_INFO_S)) <= datainBuff))
            {
                //A valid message found
                goto processMessage;

            }
        }
        ret = recv(m_sclient, (char *)(recvBuffer+offset+ datainBuff), (totalLen - offset - datainBuff), 0);
        if (ret > 0) {
            datainBuff += ret;
        }
        goto start;
        //TODO: now we get the cursor data, let's display it.

    processMessage:
        CURSOR_INFO *pCursorInfo = (CURSOR_INFO *)((unsigned char *)(recvBuffer + offset + sizeof(MSG_REP_INFO_S)));
        if (pCursorInfo->lenOfCursor) {
            memcpy(gCursorData, (unsigned char *)(recvBuffer + offset + sizeof(MSG_REP_INFO_S) + sizeof(CURSOR_INFO)), pCursorInfo->lenOfCursor);
        }
        cursorX = pCursorInfo->pos_x;
        cursorY = pCursorInfo->pos_y;

        offset += sizeof(MSG_REP_INFO_S) + repInfo->payloadLen;
        datainBuff -= (sizeof(MSG_REP_INFO_S) + repInfo->payloadLen);

    }
    closesocket(m_sclient);
    WSACleanup();
    ga_logger(Severity::INFO, "Exit current customer\n");
    return 0;
}
#else
// Main thread to handle the cursor data at client.
DWORD __stdcall ProcessThreadClient(LPVOID params)
{
    int  ret = 0;
    struct sockaddr_in xsin;
#ifdef WIN32
    int csinlen, xsinlen;
#else
    socklen_t csinlen, xsinlen;
#endif
    int clientaccepted = 0;
    //
    unsigned char buf[8192];
    int bufhead, buflen;
    THREAD_INFO_S *pThreadInfo = (THREAD_INFO_S *)params;
    if (NULL == pThreadInfo) {
        ga_logger(Severity::ERR, "Invalid thread config\n");
        return -1;
    }

    //Start to connect to server
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(sockVersion, &data) != 0)
    {
        ga_logger(Severity::ERR, "Failed to initialize Winsock\n");
        return -1;
    }

    SOCKET  m_sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sclient == INVALID_SOCKET)
    {
        ga_logger(Severity::ERR, "Failed to create a socket\n");
        return -1;
    }

    bzero(&ctrlsin, sizeof(struct sockaddr_in));
    ctrlsin.sin_family = AF_INET;
    ctrlsin.sin_port = htons(pThreadInfo->conf.ctrlport2);
    if (pThreadInfo->conf.servername != NULL) {
        ctrlsin.sin_addr.S_un.S_addr = name_resolve(pThreadInfo->conf.servername);
        if (ctrlsin.sin_addr.S_un.S_addr == INADDR_NONE) {
            ga_logger(Severity::ERR, "Name resolution failed: %s\n", pThreadInfo->conf.servername);
            return -1;
        }
    }
    csinlen = sizeof(ctrlsin);

restart:
    buflen = 0;
    MSG_REP_INFO_S handShake;
    handShake.magic  = 0x55aa55aa;
    handShake.msgHdr = HANDSHAKE;
    handShake.payloadLen = 0;

    char szBuf[1024] = { 0 };
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(m_sclient,SOL_SOCKET, SO_SNDTIMEO,(char *)&tv,sizeof(tv));
    ga_logger(Severity::INFO, " Cursor: try to send an handshake package\n");
    if ((buflen = sendto(m_sclient, (char*)&handShake, sizeof(handShake), 0, (struct sockaddr*) &ctrlsin, sizeof(ctrlsin))) < 0) {
        ga_logger(Severity::INFO, "controller client-send(udp): %s\n", strerror(errno));
        goto restart;
    }

    ga_logger(Severity::INFO, " Cursor: send handshake package success\n");
    buflen = recvfrom(m_sclient, (char*)buf, 8096, 0, (struct sockaddr*) &ctrlsin, &csinlen);
    if (buflen < sizeof(MSG_REP_INFO_S)) {
        ga_logger(Severity::INFO, " incomplete header\n");

    }
    MSG_REP_INFO_S *pMsg = (MSG_REP_INFO_S *)buf;
    if (pMsg->msgHdr != HANDSHAKE) {
        printf(" message isn't a handshake\n");
        goto restart;
    }

    while (true) {
        int msglen;
        //
        bufhead = 0;
        //

        bzero(&xsin, sizeof(xsin));
        xsinlen = sizeof(xsin);
        xsin.sin_family = AF_INET;
        buflen = recvfrom(m_sclient, (char*)buf, sizeof(buf), 0, (struct sockaddr*) &ctrlsin, &csinlen);
        if (buflen < sizeof(MSG_REP_INFO_S)) {
            continue;
        }
        //
        MSG_REP_INFO_S *pMsgheader = (MSG_REP_INFO_S *)(buf);
        msglen = pMsgheader->payloadLen;
        //
        if (msglen == 0) {
            ga_logger(Severity::WARNING, "controller server: WARNING - invalid message with size equal to zero!\n");
            continue;
        }

        if (buflen != (sizeof(MSG_REP_INFO_S) + msglen)) {
            ga_logger(Severity::INFO, "controller server: UDP msg size matched (expected %d, got %d).\n",
                msglen, buflen);
            continue;
        }

        CURSOR_INFO *pCursorInfo = (CURSOR_INFO *)((unsigned char *)(buf  + sizeof(MSG_REP_INFO_S)));
        if (pCursorInfo->lenOfCursor) {
            memcpy(gCursorData, (unsigned char *)(buf  + sizeof(MSG_REP_INFO_S) + sizeof(CURSOR_INFO)), pCursorInfo->lenOfCursor);
        }
        cursorX = pCursorInfo->pos_x;
        cursorY = pCursorInfo->pos_y;

        buflen = 0;
    }

    closesocket(m_sclient);
    WSACleanup();
    ga_logger(Severity::INFO, "Exit current customer\n");
    return 0;
}
#endif
THREAD_INFO_S threadClientInfo;
int start_cursor_client(struct RTSPConf *conf)
{

    memset(&threadClientInfo, 0, sizeof(threadClientInfo));
    if (NULL == conf) {
        ga_logger(Severity::ERR, "Invalid cursor config parameters\n");
        return -1;
    }
    memcpy(&(threadClientInfo.conf), conf, sizeof(struct RTSPConf));
    hCursorClientThreadHandle = CreateThread(NULL, 0, ProcessThreadClient, (&threadClientInfo), 0, NULL);
    return 0;
}

int get_cursor_pos(CURSOR_POS *pcurpos)
{
    if (NULL != pcurpos) {
        pcurpos->pos_x = cursorX;
        pcurpos->pos_y = cursorY;
        return 0;
    }
    return -1;
}

int set_cursor_pos(CURSOR_POS pos)
{
    cursorX = pos.pos_x;
    cursorY = pos.pos_y;

    return 0;
}

unsigned char *get_cursor_data()
{
    return gCursorData;
}

int set_cursor_data(unsigned char *pBuffer, unsigned int len)
{
    return 0;
}
#if TCP
DWORD __stdcall ProcessServerThread(LPVOID params)
{
    int ret = 0;
    THREAD_INFO_S *pThreadInfo = (THREAD_INFO_S *)params;
    if (NULL == pThreadInfo) {
        ga_logger(Severity::ERR, "ProcessThread: invalid ThreadInfomation\n");
        return -1;
    }

    //Initialize winSocket
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0)
    {
        ga_logger(Severity::ERR, "ProcessThread: WSAStartup failed\n");
        return -1;
    }

    //Create Socket
    SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slisten == INVALID_SOCKET)
    {
        ga_logger(Severity::ERR, "Failed to create socket for cursor service!");
        return -1;
    }

    //bind IP/PORT
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(pThreadInfo->conf.ctrlport2);  //TODO:read from configuration file
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        ga_logger(Severity::ERR, "Failed to bind error !");
        return -1;
    }

    // start to listen on client connection
    if (listen(slisten, 5) == SOCKET_ERROR)
    {
        ga_logger(Severity::INFO, "Failed to listen error !");
        return 0;
    }

    // handle cursor data
    SOCKET sClient;
    sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    unsigned char *pMessage = (unsigned char *)malloc(1024*1024);
restart:
    memset(pMessage, 0, 1024*1024);
    bIsReadyforSend = false;
    ga_logger(Severity::INFO, "CURSOR_SRV:waiting for a new connection...\n");
    sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
    if (sClient == INVALID_SOCKET)
    {
        ga_logger(Severity::ERR, "accept error !");
        return -1;
    }
    ga_logger(Severity::INFO, "CURSOR_SRV: A new client connected...\n");

    ga_module_t *m = encoder_get_vencoder();
    ga_ioctl_buffer_t mb;
    int err = 0;
    if ((err = ga_module_ioctl(m, GA_IOCTL_REQUEST_NEW_CURSOR, sizeof(mb), &mb)) < 0) {
        ga_logger(Severity::INFO, "Failed to insert a key frame%s, err=%d\n", m->name, err);
    }
    bIsReadyforSend = true;
    while (1) {
        //Receive request from client
        MSG_REQ_INFO_S reqInfo;
        MSG_REP_INFO_S repInfo;
        memset(&reqInfo, 0, sizeof(reqInfo));
        //ga_logger(Severity::INFO, "start to wait a new frame ...\n");
        dpipe_buffer_t *data = dpipe_load(pCursorPipe, nullptr);
        //ga_logger(Severity::INFO, "Get a new frame ...\n");
        CURSOR_INFO *pCursorInfo = (CURSOR_INFO *)(data->pointer);
        repInfo.msgHdr     = HANDSHAKE_RESP;
        repInfo.magic      = 0x55AA55AA;
        repInfo.payloadLen = sizeof(CURSOR_INFO) + pCursorInfo->lenOfCursor;// 32 * 32 * 4;

        memcpy(pMessage, (unsigned char *)&repInfo, sizeof(repInfo));
        memcpy(pMessage+ sizeof(repInfo), data->pointer, repInfo.payloadLen);

        dpipe_put(pCursorPipe, data);
        //ga_logger(Severity::INFO, "Sent a new frame ... lent\n");
        int error = send(sClient, (char *)(pMessage), sizeof(repInfo) + repInfo.payloadLen, 0);
        if (error <= 0) {
            closesocket(sClient);
            goto restart;
        }
    }

    closesocket(slisten);
    WSACleanup();
    printf("Exit current customer\n");
    return 0;
}
#else
DWORD __stdcall ProcessServerThread(LPVOID params)
{
    int ret = 0;

    struct sockaddr_in xsin;
#ifdef WIN32
    int xsinlen;
#else
    socklen_t xsinlen;
#endif
    int clientaccepted = 0;
    //
    unsigned char buf[8192];

    THREAD_INFO_S *pThreadInfo = (THREAD_INFO_S *)params;
    if (NULL == pThreadInfo) {
        ga_logger(Severity::ERR, "ProcessThread: invalid ThreadInfomation\n");
        return -1;
    }
    unsigned char* pMessage = (unsigned char*)malloc(1024 * 1024);
    if (NULL == pMessage) {
        ga_logger(Severity::ERR, "CURSOR_SRV: failed to allocate memory\n");
        return -1;
    }

restart:
    ga_logger(Severity::INFO, "CURSOR_SRV: Sever thread started \n");
    //Initialize winSocket
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0)
    {
        ga_logger(Severity::ERR, "ProcessThread: WSAStartup failed\n");
        free(pMessage);
        return -1;
    }

    //Create Socket
    slisten = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (slisten == INVALID_SOCKET)
    {
        ga_logger(Severity::ERR, "Failed to create socket for cursor service!");
        free(pMessage);
        return -1;
    }

    bzero(&ctrlsin, sizeof(struct sockaddr_in));
    ctrlsin.sin_family = AF_INET;
    ctrlsin.sin_port = htons(pThreadInfo->conf.ctrlport2);
    ctrlsin.sin_addr.S_un.S_addr = INADDR_ANY;

    ga_logger(Severity::INFO, "CURSOR_SRV: server port: %d \n", pThreadInfo->conf.ctrlport2);

    // bind for either TCP of UDP
    if (bind(slisten, (struct sockaddr*) &ctrlsin, sizeof(ctrlsin)) < 0) {
        ga_logger(Severity::ERR, "CURSOR_SRV: controller server-bind: %s\n", strerror(errno));
        free(pMessage);
        return -1;
    }

    ga_logger(Severity::INFO, "CURSOR_SRV: start to receive handshake message from client \n");

    //Wait an handshake from client;
    while (gServiceStarted) {
        ga_logger(Severity::INFO, "CURSOR_SRV: start to receive handshake message from client \n");
        int buflen = 0;

        bzero(&xsin, sizeof(xsin));
        xsinlen = sizeof(xsin);
        //xsin.sin_family = AF_INET;

        ga_logger(Severity::INFO, "CURSOR_SRV: waiting for new data\n");
        buflen = recvfrom(slisten, (char*)buf, 8096, 0, (struct sockaddr*) &xsin, &xsinlen);
        if (buflen < sizeof(MSG_REP_INFO_S)) {
            printf("CURSOR_SRV:  receive a message from client, but incomplete buflen=%d \n", buflen);
            continue;
        }
        //
        ga_logger(Severity::INFO, "CURSOR_SRV: Get for new data\n");
        MSG_REP_INFO_S *pMsg = (MSG_REP_INFO_S *)buf;
        if (pMsg->msgHdr == HANDSHAKE) {
            if (clientaccepted == 0) {
                ga_logger(Severity::INFO, "CURSOR_SRV: A valid client hand shake accept\n");
                clientaccepted = 1;

            }
            ga_logger(Severity::INFO, "CURSOR_SRV:  send back the message to client\n");
            if ((buflen = sendto(slisten, (char *)(buf), sizeof(MSG_REP_INFO_S), 0, (struct sockaddr*) &xsin, xsinlen)) < 0) {
                ga_logger(Severity::INFO, "controller client-send(udp): %s\n", strerror(errno));
                continue;
            }
            break;
        }
        ga_logger(Severity::INFO, "CURSOR_SRV: receive an invalid message\n");


    }

    ga_logger(Severity::INFO, "CURSOR_SRV: start to handle cursor message\n");
    bNeedNewCursorShape = true;

    ga_module_t *m = encoder_get_vencoder();
    ga_ioctl_buffer_t mb;
    int err = 0;
    if ((err = ga_module_ioctl(m, GA_IOCTL_REQUEST_NEW_CURSOR, sizeof(mb), &mb)) < 0) {
        ga_logger(Severity::INFO, "Failed to insert a key frame%s, err=%d\n", m->name, err);
    }
    // start to handle the keyboard mouse
    while (gServiceStarted) {

        //Receive request from client
        MSG_REQ_INFO_S reqInfo;
        MSG_REP_INFO_S repInfo;
        int wlen;
        memset(&reqInfo, 0, sizeof(reqInfo));
        //ga_logger(Severity::INFO, "start to wait a new frame ...\n");
        dpipe_buffer_t *data = dpipe_load(pCursorPipe, nullptr);
        //ga_logger(Severity::INFO, "Get a new frame ...\n");
        CURSOR_INFO *pCursorInfo = (CURSOR_INFO *)(data->pointer);
        repInfo.msgHdr = HANDSHAKE_RESP;
        repInfo.magic = 0x55AA55AA;
        repInfo.payloadLen = sizeof(CURSOR_INFO) + pCursorInfo->lenOfCursor;// 32 * 32 * 4;

        memcpy(pMessage, (unsigned char*)& repInfo, sizeof(repInfo));
        memcpy(pMessage + sizeof(repInfo), data->pointer, repInfo.payloadLen);

        dpipe_put(pCursorPipe, data);

        if ((wlen = sendto(slisten, (char *)(pMessage), sizeof(repInfo) + repInfo.payloadLen, 0, (struct sockaddr*) &xsin, sizeof(xsin))) < 0) {
            ga_logger(Severity::INFO, "controller client-send(udp): %s\n", strerror(errno));

            closesocket(slisten);
            WSACleanup();
            goto restart;
        }

        if (bRestartCursorServer) {
            closesocket(slisten);
            WSACleanup();

            ga_logger(Severity::INFO, "Cursor: A qos restart  called \n");
            bRestartCursorServer = false;
            goto restart;
        }
        //send(sClient, (char *)(pMessage), sizeof(repInfo) + repInfo.payloadLen, 0);
    }
    closesocket(slisten);
    WSACleanup();
    printf("Exit current customer\n");
    free(pMessage);
    return 0;
}

#endif
THREAD_INFO_S threadServerInfo;
// Start a cursor service
int start_cursor_service(struct RTSPConf *conf)
{
    memset(&threadServerInfo,0x0,sizeof(threadServerInfo));
    if (NULL == conf)
    {
        ga_logger(Severity::ERR, "Failed to start a cursor service due to lack of valid configuration\n");
        return -1;
    }

    // Create a thread to handle the cursor data
    memcpy(&(threadServerInfo.conf), conf, sizeof(struct RTSPConf));
    hCursorServiceThreadHandle = CreateThread(NULL, 0, ProcessServerThread, (&threadServerInfo), 0, NULL);

    //Create a dpipe
    if ((pCursorPipe = dpipe_create(0, "Cursor", CURSOR_POOLSIZE, /*sizeof(AVPicture)*/320 * 320 * 4)) == NULL) {
        ga_logger(Severity::ERR, "cursor: cannot create pipeline.\n");
        return -1;
    }
    dpipe_buffer_t *data = NULL;
    for (data = pCursorPipe->in; data != NULL; data = data->next) {
        memset(data->pointer, 0, 320 * 320 * 4 );
    }

    gServiceStarted = 1;

    return 0;
}

void restart_cursor_service()
{
    if (gServiceStarted) {
        gServiceStarted = 0;
        if (slisten != -1) {
            closesocket(slisten);
        }
        WSACleanup();
        hCursorServiceThreadHandle = CreateThread(NULL, 0, ProcessServerThread, (&threadServerInfo), 0, NULL);
        gServiceStarted = 1;
    }

}
void stop_cursor_service()
{
}
// Queue the cursor data into the server
int queue_cursor(qcsCursorInfoData ciStruct, unsigned char *pBuffer, int nLen, int waitForvideo)
{
    CURSOR_INFO          cursorInfo{};
    cursorInfo.isVisible = (BYTE)ciStruct.isVisible;
    cursorInfo.type = (BYTE)ciStruct.isColored? 2: 1;
    cursorInfo.pos_x     = ciStruct.framePos.x;
    cursorInfo.pos_y     = ciStruct.framePos.y;
    cursorInfo.hotSpot_x = ciStruct.hotSpot.x;
    cursorInfo.hotSpot_y = ciStruct.hotSpot.y;
    cursorInfo.width     = ciStruct.width;
    cursorInfo.height    = ciStruct.height;
    cursorInfo.pitch     = ciStruct.pitch;

    cursorInfo.srcRect.left = ciStruct.srcRect.left;
    cursorInfo.srcRect.right = ciStruct.srcRect.right;
    cursorInfo.srcRect.top = ciStruct.srcRect.top;
    cursorInfo.srcRect.bottom = ciStruct.srcRect.bottom;

    cursorInfo.dstRect.left = ciStruct.dstRect.left;
    cursorInfo.dstRect.right = ciStruct.dstRect.right;
    cursorInfo.dstRect.top = ciStruct.dstRect.top;
    cursorInfo.dstRect.bottom = ciStruct.dstRect.bottom;
    cursorInfo.waitforvideo = waitForvideo;

    //cursorInfo.src
    cursorInfo.lenOfCursor = nLen;

    if (cursorInfo.lenOfCursor == 0) {
            //    exit(-1);
    }
    if (gServiceStarted == 1)
    {   // RTSP client is connected.
        dpipe_buffer_t* data = dpipe_get(pCursorPipe);
        if (NULL != data) {
            unsigned char* dstframe = (unsigned char*)data->pointer;
            memcpy((unsigned char*)dstframe, (unsigned char*)& cursorInfo, sizeof(cursorInfo));

            if (pBuffer != NULL) {
                memcpy((unsigned char*)(dstframe + sizeof(CURSOR_INFO)), pBuffer, nLen);
            }
            dpipe_store(pCursorPipe, data);
        }
    }

     //ga_logger(Severity::INFO, "Doesn't find a available buffer\n");
    std::shared_ptr<CURSOR_DATA> cursorData = std::make_shared<CURSOR_DATA>();
    cursorData->cursorInfo = CURSOR_INFO(cursorInfo);  // Copy cursor info for WebRTC channel.
    if (pBuffer) {
        memcpy(cursorData->cursorData, pBuffer, nLen);
        cursorData->cursorDataUpdate = true;
    }
    else {
        cursorData->cursorDataUpdate = false;
    }
    encoder_send_cursor(cursorData, nullptr);

    if (bNeedNewCursorShape)
    {
        bNeedNewCursorShape = false;
        return 1;
    }
    return 0;
}
