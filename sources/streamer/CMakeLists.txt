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

cmake_minimum_required( VERSION 3.13 )

project(ga VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXE_LINKER_FLAGS -Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack ${CMAKE_EXE_LINKER_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS -Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack ${CMAKE_SHARED_LINKER_FLAGS})

if(NOT DEFINED USAGE)
  message(STATUS "No USAGE specified, default to: android-cg")
  set(USAGE android-cg)
  add_compile_definitions(ANDROID_CG)
endif()

include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "No CMAKE_BUILD_TYPE specified, default to: ${CMAKE_BUILD_TYPE}")
endif()

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(module)
add_subdirectory(server)
add_subdirectory(tests)

