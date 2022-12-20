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

find_program(PROTOC protoc REQUIRED)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/gae2elatency.pb.cpp
  COMMAND ${PROTOC} -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} gae2elatency.proto
  COMMAND mv ${CMAKE_CURRENT_SOURCE_DIR}/gae2elatency.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/gae2elatency.pb.cpp
  VERBATIM
)

add_library(ILE2ELatency STATIC
  E2ELatencyClient.cpp
  E2ELatencyServer.cpp
  E2ELatencyFactory.cpp
  gae2elatency.pb.cpp
)

set_target_properties(ILE2ELatency PROPERTIES PREFIX "")

target_link_libraries(ILE2ELatency
  PRIVATE
    protobuf
)

install(TARGETS ILE2ELatency LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/)
