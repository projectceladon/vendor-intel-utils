# Copyright (C) 2020-2022 Intel Corporation
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

file(GLOB_RECURSE srcs ${CMAKE_SOURCE_DIR}/templates/*.m4)

set(M4FLAGS ${M4FLAGS} -E
  -I ${CMAKE_SOURCE_DIR}/templates
  -I ${CMAKE_SOURCE_DIR}/templates/m4docker/system
  -I ${CMAKE_SOURCE_DIR}/templates/m4docker/components
  -D DEVEL)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile
  DEPENDS ${srcs}
    ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.m4
  COMMAND ${M4} ${M4FLAGS} -D OS_NAME=ubuntu -D OS_VERSION=22.04
    ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.m4 > ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile
  VERBATIM
  )

add_custom_target(p2p-ubuntu22.04-dockerfiles ALL DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)

