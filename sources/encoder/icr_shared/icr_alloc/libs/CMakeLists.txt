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

INCLUDE_DIRECTORIES(../include)

aux_source_directory(. DIR_LIB_SRCS)
add_definitions(-DBUILD_FOR_HOST=1)
add_definitions (-D_GNU_SOURCE)
add_library (icr_alloc SHARED ${DIR_LIB_SRCS})

target_link_libraries(icr_alloc PUBLIC drm-deps Threads::Threads)
target_link_options(icr_alloc PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../icr_alloc.map)

install(TARGETS icr_alloc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

