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

#ifndef SOCK_SERVER_H
#define SOCK_SERVER_H


#include "sock_util.h"


#define SOCK_MAX_CLIENTS   512


typedef struct _t_sock_server{
	int         type;
	int			socketfd;
	char		path[SOCK_MAX_PATH_LEN];
	int         client_slots[SOCK_MAX_CLIENTS];
	fd_set      rfds;
} sock_server_t;

typedef struct _t_sock_proxy_client{
	int id;
}sock_client_proxy_t;



#ifdef __cplusplus
extern "C" {
#endif

	sock_server_t* sock_server_init(int type, const char *sock_name, const char *id, int port);
	void sock_server_close (sock_server_t* server);

	int sock_server_has_newconn(sock_server_t* server, int timeout_ms);

	sock_client_proxy_t* sock_server_create_client (sock_server_t* server);
	void sock_server_close_client(sock_server_t* server, sock_client_proxy_t* p_client);

	int sock_server_clients_readable(sock_server_t* server, int timeout_ms);
	sock_conn_status_t sock_server_check_connect(sock_server_t* server, const sock_client_proxy_t* p_client);

	int sock_server_send(sock_server_t* server, const sock_client_proxy_t* sender,  const void* data, size_t datalen);
	int sock_server_recv(sock_server_t* server, const sock_client_proxy_t* receiver, void* data, size_t datalen,int one_shot=0);
	int sock_server_send_fd(sock_server_t* server, const sock_client_proxy_t* sender, int* pfd, size_t fdlen);
	int sock_server_recv_fd(sock_server_t* server, const sock_client_proxy_t* receiver, int* pfd, size_t fdlen);

	const char* sock_server_get_addr(sock_server_t* server, const char* ifname);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* SOCK_SERVER_H */
