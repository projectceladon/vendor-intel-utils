# Copyright (C) 2022 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

add_definitions(-DLOG_TAG=\"sock_util\" -DBUILD_FOR_HOST=1)

add_library(sock_util SHARED
  sock_client.cpp
  sock_server.cpp
  sock_util.cpp
)

target_include_directories(sock_util INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_options(sock_util PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/sock_util.map)

install(TARGETS sock_util LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

