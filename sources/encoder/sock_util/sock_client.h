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

#ifndef SOCK_CLIENT_H
#define SOCK_CLIENT_H

#include "sock_util.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _t_sock_client{
		int     type;
        pthread_mutex_t mutex;
		int		socketfd;
		char	path[SOCK_MAX_PATH_LEN];
	} sock_client_t;


	sock_client_t*  sock_client_init(int type, const char* server_path, int port);
	void sock_client_close(sock_client_t* client);

	sock_conn_status_t sock_client_check_connect(sock_client_t* client, int timeout_ms);

	int sock_client_send(sock_client_t* client, const void* data, size_t datalen);
	int sock_client_recv(sock_client_t* client, void* data, size_t datalen);
	int sock_client_send_fd(sock_client_t* client, int* pfd, size_t fdlen);
	int sock_client_recv_fd(sock_client_t* client, int* pfd, size_t fdlen);


#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* SOCK_CLIENT_H */
