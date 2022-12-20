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

find_package(GTest REQUIRED)

add_executable(irrv-client-test irrv_client_test.cpp
 ../module/irrv-receiver/CSendRecvMessage.cpp
 ../module/irrv-receiver/sock_client.cpp)

target_compile_definitions(irrv-client-test
  PRIVATE HOST_BUILD)

target_include_directories(irrv-client-test
  PRIVATE ../module/irrv-receiver)

target_link_libraries(irrv-client-test
  PRIVATE GTest::GTest GTest::Main ga Threads::Threads)

install(TARGETS irrv-client-test RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(irrv-client irrv_client.cpp
 ../module/irrv-receiver/CSendRecvMessage.cpp
 ../module/irrv-receiver/sock_client.cpp)

target_compile_definitions(irrv-client
  PRIVATE HOST_BUILD)

target_include_directories(irrv-client
  PRIVATE ../module/irrv-receiver)

target_link_libraries(irrv-client
  PRIVATE ga Threads::Threads)

install(TARGETS irrv-client RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

