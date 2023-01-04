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
project(display-daemon)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "CMAKE_BUILD_TYPE is undefined. Set default build type ${CMAKE_BUILD_TYPE}.")
endif()

include(GNUInstallDirs)

find_library(M_LIB m)

Message(status "  " CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS "-Wall ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS -Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack ${CMAKE_EXE_LINKER_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS -Wl,--no-undefined,-z,relro,-z,now,-z,noexecstack ${CMAKE_SHARED_LINKER_FLAGS})

set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(PKG_LIBDRM REQUIRED libdrm>=2.4.98 IMPORTED_TARGET)
pkg_check_modules(PKG_LIBDRM_INTEL REQUIRED libdrm_intel>=2.4.98 IMPORTED_TARGET)

add_library(drm-deps INTERFACE)
target_link_libraries(drm-deps INTERFACE PkgConfig::PKG_LIBDRM_INTEL PkgConfig::PKG_LIBDRM)

pkg_check_modules(VHAL REQUIRED vhal-client IMPORTED_TARGET)

add_library(libvhal-client-deps INTERFACE)
target_link_libraries(libvhal-client-deps INTERFACE PkgConfig::VHAL)

pkg_check_modules(VPL REQUIRED vpl IMPORTED_TARGET)

add_library(libvpl-deps INTERFACE)
# See: https://gitlab.kitware.com/cmake/cmake/-/issues/17377
# cmake does not populate PkgConfig::VPL correctly since it enlists
# libs outside of -Lpath such as -ldl
target_include_directories(libvpl-deps INTERFACE ${VPL_INCLUDE_DIRS})
target_link_libraries(libvpl-deps INTERFACE ${VPL_LDFLAGS})

add_definitions("-DENABLE_QSV")

pkg_check_modules(AVCODEC REQUIRED libavcodec libavformat libavutil IMPORTED_TARGET)
pkg_check_modules(AVFILTER REQUIRED libavfilter IMPORTED_TARGET)

add_library(libavcodec-deps INTERFACE)
target_link_libraries(libavcodec-deps INTERFACE PkgConfig::AVCODEC)

add_library(libavfilter-deps INTERFACE)
target_link_libraries(libavfilter-deps INTERFACE PkgConfig::AVFILTER)

pkg_check_modules(VA REQUIRED libva IMPORTED_TARGET)

add_library(libva-deps INTERFACE)
target_link_libraries(libva-deps INTERFACE PkgConfig::VA)

option(UPSTREAM_FFMPEG "Build against upstream ffmpeg?" ON)
if (NOT UPSTREAM_FFMPEG)
  add_definitions(-DFFMPEG_v42)
endif()

execute_process(
    COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE git_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
if (NOT ${git_commit} MATCHES "^$")
    set( git_commit "${git_commit}" PARENT_SCOPE)
endif()

execute_process(
    COMMAND echo
    COMMAND cut -f 1 -d.
    COMMAND date "+%Y%m%d"
    OUTPUT_VARIABLE cur_date
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (DEFINED BUILD_NUM)
    set(build_num ${BUILD_NUM})
else()
    set(build_num "localbuild")
endif()
Message(status "  " build_num=${build_num})

if (DEFINED QUALIFIER)
    set(CI_QUALIFIER ${QUALIFIER})
endif()
Message(status "  " CI_QUALIFIER=${CI_QUALIFIER})

if (DEFINED QUALIFIER_NUM)
    set(CI_QUALIFIER_NUM ${QUALIFIER_NUM})
endif()
Message(status "  " CI_QUALIFIER_NUM=${CI_QUALIFIER_NUM})

configure_file(${CMAKE_CURRENT_LIST_DIR}/auto_version.h.in ${CMAKE_CURRENT_LIST_DIR}/auto_version.h @ONLY)

set(ICR_FFMPEG_DIR ffmpeg_cg_n4.2.2)
Message(status "  " ICR_FFMPEG_DIR=${ICR_FFMPEG_DIR})

Message(status "  " build icr_encoder!)
set (executable "icr_encoder")

set (renderer_src display_renderer.cpp display_video_renderer.cpp)

add_definitions(-DBUILD_FOR_HOST=1)

add_definitions(-Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-deprecated-declarations)

include_directories(${CMAKE_CURRENT_LIST_DIR})

add_executable( ${executable}
    cli.cpp
    main.cpp
    display_server.cpp
    display_server_vhal.cpp
    display_video_renderer.cpp
    )

target_link_libraries( ${executable}
    irr_encoder
    sock_util
    dl
    ${M_LIB}
    libvhal-client-deps
    libavcodec-deps
    libva-deps
    drm-deps)

install(TARGETS ${executable} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(shared)
add_subdirectory(sock_util)

