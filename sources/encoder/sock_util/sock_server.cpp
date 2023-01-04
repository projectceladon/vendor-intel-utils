// Copyright (C) 2018-2022 Intel Corporation
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include "sock_server.h"

int sock_server_find_empty_slot(sock_server_t* server)
{
	int id=0;

	for(id=0; id<SOCK_MAX_CLIENTS; ++id)
	{
		if(-1==server->client_slots[id])
		{
			break;
		}
	}

	return id;
}

sock_server_t* sock_server_init(int type, const char *sock_name, const char *id, int port)
{

	int socketfd = 0;

	sock_server_t* server = NULL;
	int flag = 0;
	int i=0;
	int socket_domain = 0;
	int ret = 0;
	int reuse = 1;
	char sock_path[108];

	sock_log("sock_server_init(%s, %s, %s, %d) ...\n", type == SOCK_CONN_TYPE_INET_SOCK?"INET":"UNIX", sock_name, id, port);


	if(SOCK_CONN_TYPE_INET_SOCK != type) {
		socket_domain = AF_UNIX;
	}
	else {
		socket_domain = AF_INET;
	}
	socketfd=socket(socket_domain, SOCK_STREAM, 0);
	if(socketfd < 0)
	{
		sock_log("sock error: create server socket failed!\n");
		return NULL;
	}
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
		sock_log("sock error: setsockopt(SO_REUSEADDR) failed!\n");
		close(socketfd);
		return NULL;
	}

#if 1
	flag=1;
	ret = ioctl(socketfd, FIONBIO, &flag);
	if(ret < 0) {
		sock_log("sock error: set server socket to FIONBIO failed!\n");
		close(socketfd);
		return NULL;
	}
#endif

	if(SOCK_CONN_TYPE_INET_SOCK != type) {
		struct sockaddr_un serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sun_family = AF_UNIX;
		if (id == NULL)
			sprintf(sock_path,"%s", sock_name);
		else
			sprintf(sock_path,"%s%s", sock_name, id);

		if(SOCK_CONN_TYPE_ABS_SOCK == type) {
			serv_addr.sun_path[0] = 0; /* Make abstract */
		}
		else {
			unlink(sock_path);
			strcpy(serv_addr.sun_path, sock_path);
		}
		sock_log("%s: %d : bind to %s\n", __func__, __LINE__, serv_addr.sun_path);
		ret = bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_un));
	}
	else {
		struct sockaddr_in  serv_addr;
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(port);
		ret = bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));
	}
	if(ret < 0) {
		sock_log("sock error : bind server socket failed! error = %d(%s) \n", errno, strerror(errno));
		close(socketfd);
		return NULL;
	}

	if (SOCK_CONN_TYPE_UNIX_SOCK == type) {
		ret = chmod(sock_path, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
		if(ret < 0)
		{
			sock_log("sock error: chmod %s to 777 failed!\n", sock_path);
			// close(socketfd);
			//return NULL;
		}
	}

	if(listen(socketfd, SOCK_MAX_CLIENTS)<0) {
		sock_log("sock error: listen server socket failed!\n");
		close(socketfd);
		return NULL;
	}

	server=(sock_server_t*)malloc(sizeof(sock_server_t));
	if(NULL==server) {
		sock_log("sock error: create sock_server_t instance failed!\n");
		close(socketfd);
		return NULL;
	}
	memset(server, 0, sizeof(sock_server_t));
	server->type = type;
	server->socketfd=socketfd;

	if(SOCK_CONN_TYPE_INET_SOCK != type) {
		strcpy(server->path, sock_path);
	}

	for(i=0; i<SOCK_MAX_CLIENTS; ++i) {
		server->client_slots[i]=-1;
	}

	if (id != nullptr)
	{
		sock_log("sock_server_init(%s, %s, %d) returns %p \n", type == SOCK_CONN_TYPE_INET_SOCK?"INET":"UNIX", id, port, server);
	}
	return server;

}

void sock_server_close (sock_server_t* server)
{
	int id=0;

	sock_log("sock_server_close() ...\n");

	if(NULL != server) {
		close(server->socketfd);

		for(id=0; id<SOCK_MAX_CLIENTS; ++id) {
			if(-1 != server->client_slots[id]) {
				close(server->client_slots[id]);
				server->client_slots[id]=-1;
			}
		}

		if(SOCK_CONN_TYPE_UNIX_SOCK == server->type) {
			unlink(server->path);
		}

		free(server);
		server=NULL;
	}

	sock_log("sock_server_close() successful.\n");
}

int sock_server_has_newconn(sock_server_t* server, int timeout_ms)
{
	bool result=SOCK_FALSE;
	int nsel=0;
	fd_set rfds, wfds, efds;
	struct timeval timeout;

	if (!server) {
		return SOCK_FALSE;
	}

#if DEBUG_SOCK_SERVER
	// sock_log("sock_server_has_newconn() ...\n");
#endif

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);
	FD_SET(server->socketfd, &rfds);
	timeout.tv_sec = 0;
	timeout.tv_usec = timeout_ms * 1000;
	nsel=select(server->socketfd+1, &rfds, NULL, NULL, &timeout);
	switch (nsel) {
		case -1:
			sock_log("sock error: select failed!\n");
			result=SOCK_FALSE;
			break;

		case 0:
			result=SOCK_FALSE;
			break;

		default:
			if(FD_ISSET(server->socketfd, &rfds)) {
				if(sock_server_find_empty_slot(server)<SOCK_MAX_CLIENTS) {
					sock_log("sock server [%s] has new connection.\n", server->path);
					result=SOCK_TRUE;
				}
				else {
					int clientfd=0;

					if(SOCK_CONN_TYPE_INET_SOCK != server->type) {
						struct sockaddr_un client_addr;
						socklen_t addr_len = sizeof(struct sockaddr_un);
						clientfd=accept(server->socketfd, (struct sockaddr*)&client_addr, &addr_len);
					}
					else {
						struct sockaddr_in client_addr;
						socklen_t addr_len = sizeof(struct sockaddr_in);
						clientfd=accept(server->socketfd, (struct sockaddr*)&client_addr, &addr_len);
					}
					close(clientfd);

					sock_log("sock server has new connection, but client_slots is full!\n");
					result=SOCK_FALSE;
				}
			}
			break;
	}

#if DEBUG_SOCK_SERVER
	// sock_log("sock_server_has_newconn() result = %d, nsel = %d\n", result, nsel);
#endif
	return result;
}


sock_client_proxy_t* sock_server_create_client (sock_server_t* server)
{
	int flag=0;
	int	id=0;
	int clientfd=0;
	sock_client_proxy_t* p_client=NULL;
	int ret = 0;

#if DEBUG_SOCK_SERVER
	sock_log("sock_server_create_client() ...\n");
#endif

	if (!server) {
		return NULL;
	}

	if(SOCK_CONN_TYPE_INET_SOCK != server->type) {
		struct sockaddr_un client_addr_un;
		socklen_t addr_len = sizeof(struct sockaddr_un);
		clientfd=accept(server->socketfd, (struct sockaddr*)&client_addr_un, &addr_len);
	}
	else {
		struct sockaddr_in client_addr_in;
		socklen_t addr_len = sizeof(struct sockaddr_in);
		clientfd=accept(server->socketfd, (struct sockaddr*)&client_addr_in, &addr_len);
	}
	if(clientfd < 0) {
		sock_log("sock error: accept socketfd failed!\n");
		return NULL;
	}

#if 1
	flag=1;
	ret = ioctl(clientfd, FIONBIO, &flag);
	if(ret < 0) {
		sock_log("sock error: set client socket to FIONBIO failed!\n");
		close(clientfd);
		return NULL;
	}
#endif

	id=sock_server_find_empty_slot(server);
	if(id >=SOCK_MAX_CLIENTS) {
		sock_log("sock error: the client_slots is full!\n");
		close(clientfd);
		return NULL;
	}

	p_client=(sock_client_proxy_t*)malloc(sizeof(sock_client_proxy_t));
	if(NULL==p_client) {
		sock_log("sock error: malloc sock_client_proxy_t failed!\n");
		close(clientfd);
		return NULL;
	}

	server->client_slots[id]=clientfd;
	p_client->id=id;

#if DEBUG_SOCK_SERVER
	sock_log("sock_server_create_client() successful, client id is %d\n", id);
#endif

	return p_client;
}


void sock_server_close_client(sock_server_t* server, sock_client_proxy_t* p_client)
{

#if DEBUG_SOCK_SERVER
	sock_log("sock_server_close_client() ...\n");
#endif

	if (!server) {
		return;
	}

	if (!p_client) {
		return;
	}

	if(-1!=server->client_slots[p_client->id])
	{
		close(server->client_slots[p_client->id]);
		server->client_slots[p_client->id]=-1;
	}

	free(p_client);

#if DEBUG_SOCK_SERVER
	sock_log("sock_server_close_client() completed.\n");
#endif
}

int sock_server_clients_readable(sock_server_t* server, int timeout_ms)
{
	int result=SOCK_FALSE;

	int i=0;
	int maxfd=0;
	int nsel=0;

	struct timeval timeout;

#if DEBUG_SOCK_SERVER
	// sock_log("sock_server_clients_readable() ...\n");
#endif

	if (!server) {
		return result;
	}

	FD_ZERO(&(server->rfds));
	for(i=0; i<SOCK_MAX_CLIENTS; ++i)
	{
		if(server->client_slots[i]!=-1)
		{
			FD_SET(server->client_slots[i], &(server->rfds));
			maxfd= (maxfd > server->client_slots[i]) ? maxfd : server->client_slots[i];
		}
	}

	timeout.tv_sec = 0;
	timeout.tv_usec = timeout_ms * 1000;

	nsel=select(maxfd+1, &(server->rfds), NULL, NULL, &timeout);
	switch (nsel) {
		case -1:
			sock_log("sock error: check clients readable select failed!\n");
			result=SOCK_FALSE;
			break;

		case 0:
			result=SOCK_FALSE;
			break;

		default:
			result=SOCK_TRUE;
			break;
	}

#if DEBUG_SOCK_SERVER
	// sock_log("sock_server_clients_readable : result is %d, nsel = %d\n", result, nsel);
#endif

	return result;
}

sock_conn_status_t sock_server_check_connect(sock_server_t* server, const sock_client_proxy_t* p_client)
{
	int nread=0;
	sock_conn_status_t result=normal;
	int clientfd=0;

#if 0
	sock_log("sock_server_check_connect() : client %d\n", p_client->id);
#endif

	if (!server) {
		return result;
	}

	if(!p_client) {
		return result;
	}




	clientfd=server->client_slots[p_client->id];
	if(FD_ISSET(clientfd, &server->rfds))
	{
		ioctl(clientfd, FIONREAD, &nread);
		if(nread!=0)
		{
			result=readable;
		}
		else
		{
			result=disconnect;
		}
	}
	else
	{
		result=normal;
	}

#if 0
	switch(result){
		case disconnect:
			sock_log("sock_server_check_connect() : client %d is disconnect\n", p_client->id);
			break;
		case readable:
			sock_log("sock_server_check_connect() : client %d is readable\n", p_client->id);
			break;
		default:
			sock_log("sock_server_check_connect() : client %d is normal\n", p_client->id);
	}
#endif

	return result;
}

int sock_server_send(sock_server_t* server, const sock_client_proxy_t* sender,  const void* data, size_t datalen)
{
    int ret = 0;

    if (!server) {
        return -1;
    }

    if(!sender) {
        return -1;
    }

#if DEBUG_SOCK_SERVER
    sock_log("sock_server_send(client %d, %p,  %d bytes)\n", sender->id, data, (int)datalen);
#endif

    unsigned char*     p_src = (unsigned char*)data;
    size_t             left_len = datalen;
    int                do_retry  = 0;

    bool timeout = false;
    int64_t begin_time = sock_get_currtime();
    do {
        ret = send(server->client_slots[sender->id], p_src, left_len, MSG_NOSIGNAL);
#if DEBUG_SOCK_CLIENT
        if (ret != (int) left_len) {
            sock_log("sock_server_send() sent %d bytes != expect %d bytes, error = %d (%s)\n",
                    ret, (int) left_len, errno, strerror(errno));
        }
#endif

        if (ret > 0) {
            left_len    -= ret;
            p_src       += ret;

            if (left_len > 0) {
                do_retry = 1;
            }
        }
        else {
            if ((errno == EINTR) || (errno == EAGAIN)) {
                do_retry = 1;
            }
            else {
                sock_log("sock_server_send() expect %d bytes, sent %d bytes, left %d bytes, error = %d (%s), timeout(%d)\n",
                         (int)datalen, (int)(datalen - left_len), (int)left_len, errno, strerror(errno), timeout);
                return ret;
            }
        }

        if(do_retry) {
            sock_usleep(SOCK_USLEEP_TIME);
            timeout = sock_timeout(begin_time, SOCK_TIMEOUT_TIME);
            if(timeout) {
                sock_log("sock_server_send() expect %d bytes, sent %d bytes, left %d bytes, error = %d (%s), timeout(%d)\n",
                         (int)datalen, (int)(datalen - left_len), (int)left_len, errno, strerror(errno), timeout);
                do_retry = 0;
            }
        }
    } while ((left_len > 0) && (do_retry != 0));

    return (int)(datalen - left_len);

}


int sock_server_recv(sock_server_t* server, const sock_client_proxy_t* receiver, void* data, size_t datalen,int one_shot)
{
    int ret = 0;

    if (!server) {
        return -1;
    }

    if(!receiver) {
        return -1;
    }

#if DEBUG_SOCK_SERVER
    sock_log("sock_server_recv(client %d, %p,  %d bytes)\n", receiver->id, data, (int)datalen);
#endif

    unsigned char*     p_dst = (unsigned char*)data;
    size_t             left_len = datalen;
    int                do_retry  = 0;

    bool timeout = false;
    int64_t begin_time = sock_get_currtime();
    do {
        ret = recv(server->client_slots[receiver->id], p_dst, left_len, MSG_WAITALL);
#if DEBUG_SOCK_CLIENT
        if (ret != (int) left_len) {
            sock_log("sock_server_recv() recv %d bytes != expect %d bytes, error = %d (%s)\n", ret,
                    (int) left_len, errno, strerror(errno));
        }
#endif

        if (ret > 0) {
            left_len    -= ret;
            p_dst       += ret;

            if ((left_len > 0) && !one_shot) {
                do_retry = 1;
            }
        }
        else {
            if ((errno == EINTR) || (errno == EAGAIN)) {
                if(!one_shot) {
                    do_retry = 1;
                }
            }
            else {
                sock_log("sock_server_recv() expect %d bytes, recevied %d bytes, left %d bytes, error = %d (%s), timeout(%d)\n",
                         (int)datalen, (int)(datalen - left_len), (int)left_len, errno, strerror(errno), timeout);
                return ret;
            }
        }

        if(do_retry) {
            sock_usleep(SOCK_USLEEP_TIME);
            timeout = sock_timeout(begin_time, SOCK_TIMEOUT_TIME);
            if(timeout) {
                sock_log("sock_server_recv() expect %d bytes, recevied %d bytes, left %d bytes, error = %d (%s), timeout(%d)\n",
                         (int)datalen, (int)(datalen - left_len), (int)left_len, errno, strerror(errno), timeout);
                do_retry = 0;
            }
        }
    }while ((left_len > 0) && (do_retry != 0));

    return (int)(datalen - left_len);

}


int sock_server_send_fd(sock_server_t* server, const sock_client_proxy_t* sender, int* pfd, size_t fdlen)
{
	int ret = 0;
	int count = 0;
	int i = 0;
	struct msghdr msg;
	struct cmsghdr *p_cmsg;
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(fdlen * sizeof(int))];
	int *p_fds= NULL;
	int sdata[4] = {0x88};
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	p_cmsg = CMSG_FIRSTHDR(&msg);
	if (p_cmsg == NULL) {
		sock_log("%s : %d : no msg hdr\n", __func__, __LINE__);
		ret = -1;
	}
	else {
		p_cmsg->cmsg_level = SOL_SOCKET;
		p_cmsg->cmsg_type = SCM_RIGHTS;
		p_cmsg->cmsg_len = CMSG_LEN(fdlen * sizeof(int));
		p_fds = (int *)CMSG_DATA(p_cmsg);
	}

#if DEBUG_SOCK_SERVER
	sock_log("sock_server_send_fd(cliend %d, pfd=%p, %d)\n", sender->id, pfd, (int)fdlen);
#endif

#if DEBUG_SOCK_SERVER
	for (i = 0; i< (int)fdlen; i++){
		sock_log("pfd[%d]=%d\n", i, pfd[i]);
		if(i > 8) {
			break;
		}

	}
#endif

	if (!server) {
		ret = -1;
		return ret;
	}

	if (p_fds) {
		for (i = 0; i < (int)fdlen; i++) {
			p_fds[i] = pfd[i];
		}
	}

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	vec.iov_base = &sdata;
	vec.iov_len = 16;

	do {
		count = sendmsg(server->client_slots[sender->id], &msg, MSG_NOSIGNAL);
		if (count < 0) {
#if DEBUG_SOCK_SERVER
			sock_log("%s : %d : sendmsg failed, count = %d, error = %d (%s)\n", __func__, __LINE__, count, errno, strerror(errno));
#endif
		}
	} while ((count < 0) && ((errno == EINTR) || (errno == EAGAIN)));

	if (count < 0) {
		sock_log("%s : %d : sendmsg failed, count = %d, error = %d (%s)\n", __func__, __LINE__, count, errno, strerror(errno));
		ret = -1;
	}

	return ret;
}

int sock_server_recv_fd(sock_server_t* server, const sock_client_proxy_t* receiver, int* pfd, size_t fdlen)
{
	int ret = 0;
	int count = 0;
	int i = 0;
	struct msghdr msg;
	int rdata[4] = {0x0};
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(fdlen*sizeof(int))];
	struct cmsghdr *p_cmsg;
	int *p_fds;
	vec.iov_base = rdata;
	vec.iov_len = 16;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;


#if DEBUG_SOCK_SERVER
	sock_log("sock_server_recv_fd(cliend %d, pfd=%p, %d)\n", receiver->id, pfd, (int)fdlen);
#endif

	if (!server) {
		ret = -1;
		return ret;
	}

	p_fds = (int *)CMSG_DATA(CMSG_FIRSTHDR(&msg));
	if (p_fds != nullptr) {
		*p_fds = -1;
	}

	do {
		count = recvmsg(server->client_slots[receiver->id], &msg, MSG_WAITALL);
		if (count < 0) {
#if DEBUG_SOCK_SERVER
			sock_log("%s : %d : recvmsg failed, count = %d, error = %d (%s)\n", __func__, __LINE__, count, errno, strerror(errno));
#endif
		}
	} while ((count < 0) && ((errno == EINTR) || (errno == EAGAIN)));


	if (count < 0) {
		sock_log("%s : %d : recvmsg failed, count = %d, error = %d (%s)\n", __func__, __LINE__, count, errno, strerror(errno));
		ret = -1;
	}
	else {

		p_cmsg = CMSG_FIRSTHDR(&msg);
		if (p_cmsg == NULL) {
			sock_log("%s : %d : no msg hdr\n", __func__, __LINE__);
			ret = -1;
		}
		else {
			p_fds = (int *)CMSG_DATA(p_cmsg);
			for (i = 0; i < (int)fdlen; i++) {
				pfd[i] = p_fds[i];
			}
		}
	}

#if DEBUG_SOCK_SERVER
	for (i = 0; i< (int)fdlen; i++){
		sock_log("pfd[%d]=%d\n", i, pfd[i]);
		if(i > 8) {
			break;
		}

	}
#endif

	return ret;
}

const char* sock_server_get_addr(sock_server_t* server, const char* ifname)
{
	const char* addr = "";
	if(server) {
		if (server->type != SOCK_CONN_TYPE_INET_SOCK) {
			addr = server->path;
		}
		else {

			struct ifreq ifr;
			int ret = 0;

			memset(server->path, 0, SOCK_MAX_PATH_LEN);
			if (strlen(ifname) > (sizeof(ifr.ifr_name) - 1)) {
				sock_log("set ifr_name failed\n");
			}
			else {
				strcpy(ifr.ifr_name, ifname);
			}
			ret = ioctl(server->socketfd, SIOCGIFADDR, &ifr);
			if ( ret < 0) {
				sock_log("%s : %d : ioctl SIOCGIFADDR failed, ret = %d, error = %d(%s)\n", __func__, __LINE__, ret, errno, strerror(errno));
			}
			else {
				char* in_addr = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
				if (strlen(in_addr) > (sizeof(server->path) - 1)) {
					sock_log("set server path failed\n");
				}
				else {
					strcpy(server->path, in_addr);
				}
			}
			addr = server->path;
		}
	}


#if DEBUG_SOCK_SERVER
	sock_log("%s : %d : addr is %s\n", __func__, __LINE__, addr);
#endif

	return addr;
}
