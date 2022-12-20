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

#include "QoSMgt.h"
#include "encoder-common.h"
#include <mutex>
#include <queue>

using namespace std;

// Cursor Server Data
dpipe_t *pQosPipe = NULL;
HANDLE hQosServiceThreadHandle = NULL;
static unsigned int gQosServiceStarted = 0;
// Cursor Client Data
HANDLE hQosClientThreadHandle = NULL;
static struct sockaddr_in ctrlsin;
queue<QosClientInfo> qosclientQueue;
static std::mutex queue_mutex;
static bool bRestartQosServer = false;
//#define TCP 0
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
#ifdef TCP

// Main thread to handle the cursor data at client.
DWORD __stdcall ProcessQosThreadClient(LPVOID params)
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
    serAddr.sin_port = htons(pThreadInfo->conf.ctrlport2+1);// pThreadInfo->nPort);
    if (pThreadInfo->conf.servername != NULL) {
        serAddr.sin_addr.S_un.S_addr = name_resolve(pThreadInfo->conf.servername);
        if (serAddr.sin_addr.S_un.S_addr == INADDR_NONE) {
            ga_logger(Severity::ERR, "Name resolution failed: %s\n", pThreadInfo->conf.servername);
            return -1;
        }
    }

    if (connect(m_sclient, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {
        ga_logger(Severity::INFO, "connect error !\n");
        closesocket(m_sclient);
        return 0;
    }
    ga_logger(Severity::INFO, "\n Client connects to server success\n");
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
            memcpy(recvBuffer, recvBuffer + offset, datainBuff);
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
        ret = recv(m_sclient, (char *)(recvBuffer + offset + datainBuff), (totalLen - offset - datainBuff), 0);
        if (ret > 0) {
            datainBuff += ret;
        }
        goto start;
        //TODO: now we get the cursor data, let's display it.
    processMessage:
            QosInfo *pQosInfo = (QosInfo *)((unsigned char *)(recvBuffer + offset + sizeof(MSG_REP_INFO_S)));

            //std::cout << "Frame no: " << pQosInfo->frameno << "\n";
            //std::cout << "Capture latency: " << pQosInfo->capturetime << " ms\n";
            //std::cout << "Encode latency:  " << pQosInfo->encodetime << " ms\n";
            //std::cout << "frame size:  " << pQosInfo->framesize << " ms\n";
            //    std::cout << "Event: " << (cln_event_time2.tv_sec << "s "<<  cln_event_time2.tv_usec<< " us\n";
            //printf("YYYYYYYYYYYYYYYY report mouse event=%lds, %ldus\r\n", pQosInfo->eventime.tv_sec, pQosInfo->eventime.tv_usec);
            QosInfoFull   qosInfoFull;
            if (!qosclientQueue.empty()) {
                QosClientInfo qosClientInfo;
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    qosClientInfo = qosclientQueue.front();
                    qosclientQueue.pop();
                }
                qosInfoFull.capturetime = pQosInfo->capturetime;
                qosInfoFull.encodetime = pQosInfo->encodetime;
                qosInfoFull.frameno = pQosInfo->frameno;
                qosInfoFull.framesize = pQosInfo->framesize;
                qosInfoFull.netlatency = qosClientInfo.netlatency;
                qosInfoFull.decodeSubmit = qosClientInfo.decodeSubmit;
                qosInfoFull.eventtime = pQosInfo->eventime;

            }
            // Enqueue the cursor into display thread;
            dpipe_buffer_t *data = NULL;
            unsigned char *dstframe = NULL;
            dpipe_t *pipeQos = dpipe_lookup("Qos-0");
            if (pipeQos) {
                data = dpipe_get(pipeQos);
                dstframe = (unsigned char *)data->pointer;
                memcpy(dstframe,&qosInfoFull, sizeof(QosInfoFull));
                dpipe_store(pipeQos, data);
            }



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
DWORD __stdcall ProcessQosThreadClient(LPVOID params)
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
        ga_logger(Severity::ERR, "Failed to create a socket");
        return -1;
    }

    bzero(&ctrlsin, sizeof(struct sockaddr_in));
    ctrlsin.sin_family = AF_INET;
    ctrlsin.sin_port = htons(pThreadInfo->conf.ctrlport2+1);
    if (pThreadInfo->conf.servername != NULL) {
        ctrlsin.sin_addr.S_un.S_addr = name_resolve(pThreadInfo->conf.servername);
        if (ctrlsin.sin_addr.S_un.S_addr == INADDR_NONE) {
            ga_logger(Severity::ERR, "Name resolution failed: %s\n", pThreadInfo->conf.servername);
            return -1;
        }
        printf("Qos Client: pThreadInfo->conf.servername =%s\n", pThreadInfo->conf.servername);
    }
    csinlen = sizeof(ctrlsin);

restart:


    buflen = 0;
    MSG_REP_INFO_S handShake;
    handShake.magic  = 0x55aa55aa;
    handShake.msgHdr = HANDSHAKE;
    handShake.payloadLen = 0;

    ga_logger(Severity::INFO, "Qos Client: try to send an handshake package\n");
    if ((buflen = sendto(m_sclient, (char*)&handShake, sizeof(handShake), 0, (struct sockaddr*) &ctrlsin, sizeof(ctrlsin))) < 0) {
        ga_logger(Severity::INFO, "controller client-send(udp): %s\n", strerror(errno));
        goto restart;
    }

    ga_logger(Severity::INFO, "Qos Client: send handshake package success\n");
    buflen = recvfrom(m_sclient, (char*)buf, 8096, 0, (struct sockaddr*) &ctrlsin, &csinlen);
    if (buflen < sizeof(MSG_REP_INFO_S)) {
        ga_logger(Severity::INFO, " incomplete header\n");
        goto restart;

    }
    MSG_REP_INFO_S *pMsg = (MSG_REP_INFO_S *)buf;
    if (pMsg->msgHdr != HANDSHAKE) {
        printf("Qos Client: message isn't a handshake\r\n");
        goto restart;
    }
    printf("Qos Client: start to process QoS Message\r\n");
    //nNetTimeout = 0;
 //iResult = setsockopt(m_sclient, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));
//    if (iResult == SOCKET_ERROR) {
    //    printf("setsockopt for SO_KEEPALIVE failed with error: %u\n", WSAGetLastError());
    //}
    //else
    //    printf("Set SO_KEEPALIVE: ON\n");
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

        QosInfo *pQosInfo = (QosInfo *)((unsigned char *)(buf + sizeof(MSG_REP_INFO_S)));

        //std::cout << "Frame no: " << pQosInfo->frameno << "\n";
        //std::cout << "Capture latency: " << pQosInfo->capturetime << " ms\n";
        //std::cout << "Encode latency:  " << pQosInfo->encodetime << " ms\n";
        //std::cout << "frame size:  " << pQosInfo->framesize << " ms\n";
        //    std::cout << "Event: " << (cln_event_time2.tv_sec << "s "<<  cln_event_time2.tv_usec<< " us\n";
        //printf("YYYYYYYYYYYYYYYY report mouse event=%lds, %ldus\r\n", pQosInfo->eventime.tv_sec, pQosInfo->eventime.tv_usec);
        QosInfoFull   qosInfoFull;
        memset(&qosInfoFull, 0, sizeof(QosInfoFull));
        qosInfoFull.capturetime = pQosInfo->capturetime;
        qosInfoFull.encodetime  = pQosInfo->encodetime;
        qosInfoFull.frameno     = pQosInfo->frameno;
        qosInfoFull.framesize   = pQosInfo->framesize;
        qosInfoFull.eventtime   = pQosInfo->eventime;
        qosInfoFull.captureFps  = pQosInfo->captureFps;
        qosInfoFull.bitrate     = pQosInfo->bitrate;

        /*
        QosClientInfo qosClientInfo;
        pthread_mutex_lock(&queue_mutex);
        if (!qosclientQueue.empty()) {
            qosClientInfo = qosclientQueue.front();
            qosclientQueue.pop();
            qosInfoFull.netlatency   = qosClientInfo.netlatency;
            qosInfoFull.decodeSubmit = qosClientInfo.decodeSubmit;
        }
        pthread_mutex_unlock(&queue_mutex);*/

        // Enqueue the cursor into display thread;
        dpipe_buffer_t *data = NULL;
        unsigned char *dstframe = NULL;
        dpipe_t *pipeQos = dpipe_lookup("Qos-0");
        if (pipeQos) {
            data = dpipe_get(pipeQos);
            dstframe = (unsigned char *)data->pointer;
            memcpy(dstframe, &qosInfoFull, sizeof(QosInfoFull));
            dpipe_store(pipeQos, data);
        }

        buflen = 0;
    }

    closesocket(m_sclient);
    WSACleanup();
    ga_logger(Severity::INFO, "Exit current customer\n");
    return 0;
}
#endif
THREAD_INFO_S qosThreadClientInfo;
int start_qos_client(struct RTSPConf *conf)
{

    while (!qosclientQueue.empty()) qosclientQueue.pop(); //Initialize the usage

    memset(&qosThreadClientInfo, 0, sizeof(qosThreadClientInfo));
    if (NULL == conf) {
        ga_logger(Severity::ERR, "Invalid cursor config parameters\n");
        return -1;
    }
    memcpy(&(qosThreadClientInfo.conf), conf, sizeof(struct RTSPConf));
    hQosClientThreadHandle = CreateThread(NULL, 0, ProcessQosThreadClient, (&qosThreadClientInfo), 0, NULL);
    return 0;
}

void queue_qos_client_info(QosClientInfo qosClientInfo)
{
    std::lock_guard<std::mutex> lock(queue_mutex);
    qosclientQueue.push(qosClientInfo);
}


#ifdef TCP
DWORD __stdcall ProcessQosServerThread(LPVOID params)
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
    sin.sin_port = htons(pThreadInfo->conf.ctrlport2+1);  //TODO:read from configuration file
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

    ga_logger(Severity::INFO, "Qos_SRV: waiting for a new connection...\n");
    sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
    if (sClient == INVALID_SOCKET)
    {
        ga_logger(Severity::INFO, "accept error !");
        return -1;
    }
    ga_logger(Severity::INFO, "Qos_SRV: A new client connected...\n");
    unsigned char *pMessage = (unsigned char *)malloc(1024 * 1024);
    while (1) {
        //Receive request from client
        MSG_REQ_INFO_S reqInfo;
        MSG_REP_INFO_S repInfo;
        memset(&reqInfo, 0, sizeof(reqInfo));
        //ga_logger(Severity::INFO, "start to wait a new frame ...\n");
        dpipe_buffer_t *data = dpipe_load(pQosPipe, nullptr);
        //ga_logger(Severity::INFO, "Get a new frame ...\n");
        QosInfo *pQosInfo = (QosInfo *)(data->pointer);
        repInfo.msgHdr     = HANDSHAKE_RESP;
        repInfo.magic      = 0x55AA55AA;
        repInfo.payloadLen = sizeof(QosInfo) ;// 32 * 32 * 4;

        memcpy(pMessage, (unsigned char *)&repInfo, sizeof(repInfo));
        memcpy(pMessage + sizeof(repInfo), data->pointer, repInfo.payloadLen);

        dpipe_put(pQosPipe, data);
        //ga_logger(Severity::INFO, "Sent a new frame ... lent\n");
        send(sClient, (char *)(pMessage), sizeof(repInfo) + repInfo.payloadLen, 0);
    }
    closesocket(sClient);
    closesocket(slisten);
    WSACleanup();
    printf("Exit current customer\r\n");
    return 0;
}
#else
DWORD __stdcall ProcessQosServerThread(LPVOID params)
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
    if (NULL ==  pMessage) {
        ga_logger(Severity::ERR, "QoS_SRV: failed to allocate memory\n");
        return -1;
    }

restart:
    ga_logger(Severity::INFO, "QoS_SRV: sever thread started \n");
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
    SOCKET slisten = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (slisten == INVALID_SOCKET)
    {
        ga_logger(Severity::ERR, "Failed to create socket for cursor service!");
        free(pMessage);
        return -1;
    }

    bzero(&ctrlsin, sizeof(struct sockaddr_in));
    ctrlsin.sin_family = AF_INET;
    ctrlsin.sin_port = htons(pThreadInfo->conf.ctrlport2+1);
    ctrlsin.sin_addr.S_un.S_addr = INADDR_ANY;

    ga_logger(Severity::INFO, "QOS_SRV: server port: %d\n", pThreadInfo->conf.ctrlport2+1);

    // bind for either TCP of UDP
    if (bind(slisten, (struct sockaddr*) &ctrlsin, sizeof(ctrlsin)) < 0) {
        ga_logger(Severity::ERR, "QOS_SRV: controller server-bind: %s\n", strerror(errno));
        free(pMessage);
        return -1;
    }
    ga_logger(Severity::INFO, "QOS_SRV:  start to receive handshake message from client\n");

    //Wait an handshake from client;
    memset(buf,0,8192);

    while (true) {

        int buflen = 0;

        bzero(&xsin, sizeof(xsin));
        xsinlen = sizeof(xsin);
        //xsin.sin_family = AF_INET;

        ga_logger(Severity::INFO, "QOS_SRV: waiting for new data\n");
        buflen = recvfrom(slisten, (char*)buf, 8096, 0, (struct sockaddr*) &xsin, &xsinlen);
        if (buflen < sizeof(MSG_REP_INFO_S)) {
            printf("CURSOR_SRV: receive a message from client, but incomplete buflen=%d \n", buflen);
            continue;
        }
        //
        ga_logger(Severity::INFO, "QOS_SRV: Get for new data\n");
        MSG_REP_INFO_S *pMsg = (MSG_REP_INFO_S *)buf;
        if (pMsg->msgHdr == HANDSHAKE) {
            if (clientaccepted == 0) {
                ga_logger(Severity::INFO, "QOS_SRV: A valid client hand shake accept\n");
                clientaccepted = 1;

            }

            ga_logger(Severity::INFO, "QOS_SRV: send back the message to client \n");
            if ((buflen = sendto(slisten, (char *)(buf), sizeof(MSG_REP_INFO_S), 0, (struct sockaddr*) &xsin, xsinlen)) < 0) {
                ga_logger(Severity::INFO, "controller client-send(udp): %s\n", strerror(errno));
                continue;
            }
            break;
        }

        ga_logger(Severity::INFO, "QOS_SRV: receive an invalid message \n");
    }

    // start to handle the keyboard mouse
    while (1)
    {
        //Receive request from client
        MSG_REQ_INFO_S reqInfo;
        MSG_REP_INFO_S repInfo;
        int wlen;
        memset(&reqInfo, 0, sizeof(reqInfo));
        //ga_logger(Severity::INFO, "start to wait a new frame ...\n");
        dpipe_buffer_t *data = dpipe_load(pQosPipe, nullptr);
        //ga_logger(Severity::INFO, "Get a new frame ...\n");
        QosInfo *pQosInfo = (QosInfo *)(data->pointer);
        repInfo.msgHdr     = HANDSHAKE_RESP;
        repInfo.magic      = 0x55AA55AA;
        repInfo.payloadLen = sizeof(QosInfo);// 32 * 32 * 4;

        memset(pMessage, 0, 1024 * 1024);
        memcpy(pMessage, (unsigned char*)& repInfo, sizeof(repInfo));
        memcpy(pMessage + sizeof(repInfo), data->pointer, repInfo.payloadLen);

        dpipe_put(pQosPipe, data);

        //ga_logger(Severity::INFO, "QOS_SRV： send a new report\n");
        if ((wlen = sendto(slisten, (char *)(pMessage), sizeof(repInfo) + repInfo.payloadLen, 0, (struct sockaddr*) &xsin, sizeof(xsin))) < 0) {
            ga_logger(Severity::INFO, "QoS: controller client-send(udp): %s\n", strerror(errno));
            goto restart;
        }
        if (bRestartQosServer) {
            closesocket(slisten);
            WSACleanup();

            ga_logger(Severity::INFO, "QoS: A qos restart  called \n");
            bRestartQosServer = false;
            goto restart;
        }
        //send(sClient, (char *)(pMessage), sizeof(repInfo) + repInfo.payloadLen, 0);
    }

    free(pMessage);
    ga_logger(Severity::INFO, "Exit current customer\n");
    return 0;
}

#endif
THREAD_INFO_S qosThreadServerInfo;
// Start a cursor service
int start_qos_service(struct RTSPConf *conf)
{
    memset(&qosThreadServerInfo, 0x0, sizeof(qosThreadServerInfo));
    if (NULL == conf)
    {
        ga_logger(Severity::ERR, "Failed to start a cursor service due to lack of valid configuration\n");
        return -1;
    }

    // Create a thread to handle the cursor data
    memcpy(&(qosThreadServerInfo.conf), conf, sizeof(struct RTSPConf));
    hQosServiceThreadHandle = CreateThread(NULL, 0, ProcessQosServerThread, (&qosThreadServerInfo), 0, NULL);

    //Create a dpipe
    if ((pQosPipe = dpipe_create(0, "Qos", QOS_POOLSIZE, /*sizeof(AVPicture)*/320 * 320 * 4)) == NULL) {
        ga_logger(Severity::ERR, "cursor: cannot create pipeline.\n");
        return -1;
    }
    dpipe_buffer_t *data = NULL;
    for (data = pQosPipe->in; data != NULL; data = data->next) {
        memset(data->pointer, 0, 320 * 320 * 4);
    }

    gQosServiceStarted = 1;

    return 0;
}

void restart_qos_service()
{
    bRestartQosServer = true;
}
void stop_qos_service()
{
    TerminateThread(hQosServiceThreadHandle, 0);
    WSACleanup();
}

// Queue the cursor data into the server
int queue_qos(QosInfo qosinfo)
{
    if (gQosServiceStarted == 1)
    {
        dpipe_buffer_t *data = dpipe_get(pQosPipe);
        if (NULL != data)
        {
            unsigned char *dstframe = (unsigned char *)data->pointer;
            memcpy((unsigned char *)dstframe, (unsigned char *)(&qosinfo), sizeof(qosinfo));
            dpipe_store(pQosPipe, data);
        }
        else {
            //ga_logger(Severity::INFO, "Doesn't find a available buffer\n");
        }
    }

    std::shared_ptr<QosInfo> sQosInfo = std::make_shared<QosInfo>(qosinfo);
    encoder_send_qos(sQosInfo);
    return 0;
}
