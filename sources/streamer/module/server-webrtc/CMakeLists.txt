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

find_package(nlohmann_json)

#pkg_check_modules(PKG_SDL2 REQUIRED sdl2 IMPORTED_TARGET)
pkg_check_modules(PKG_OWT_GS REQUIRED owt-gs IMPORTED_TARGET)
pkg_check_modules(PKG_SIOCLIENT REQUIRED sioclient IMPORTED_TARGET)
pkg_check_modules(PKG_LIBVHAL_CLIENT REQUIRED vhal-client IMPORTED_TARGET)

add_library(owt-deps INTERFACE)
target_link_libraries(owt-deps
  INTERFACE
    #PkgConfig::PKG_SDL2
    PkgConfig::PKG_OWT_GS
    PkgConfig::PKG_SIOCLIENT
    PkgConfig::PKG_LIBVHAL_CLIENT
)

set(srcs
  aic-vhal-client/audio-frame-generator.cpp
  aic-vhal-client/AudioPlayer.cpp
  aic-vhal-client/EncodedVideoDispatcher.cpp
  aic-vhal-client/RemoteStreamHandler.cpp
  aic-vhal-client/SensorHandler.cpp
  aic-vhal-client/CommandChannelHandler.cpp
  aic-vhal-client/CameraClientHandler.cpp
  aic-vhal-client/android-latency.cpp
  ga-controller-android.cpp
  ga-cursor.cpp
  ga-qos.cpp
  ga-video-input.cpp
  ics-p2p-client.cpp
  p2p-socket-signaling-channel.cpp
  server-webrtc.cpp
  )

add_library(server-webrtc SHARED ${srcs})
add_dependencies(server-webrtc nlohmann_json)

set_target_properties(server-webrtc PROPERTIES PREFIX "")

target_compile_definitions(server-webrtc
  PRIVATE
    GA_MODULE
    WEBRTC_LINUX
    WEBRTC_POSIX)

target_include_directories(server-webrtc
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/aic-vhal-client
    ${CMAKE_CURRENT_SOURCE_DIR}/../e2elatency
    )

target_link_libraries(server-webrtc
  PRIVATE
    owt-deps
    ga
    ILE2ELatency
    Threads::Threads)

install(TARGETS server-webrtc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/)
