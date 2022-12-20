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

install(
  FILES
    ${USAGE}/server.desktop.webrtc.conf
  DESTINATION
    ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}/)

install(
  FILES
    ${USAGE}/common/audio-pcm.conf
    ${USAGE}/common/controller.conf
    ${USAGE}/common/server-common.conf
    ${USAGE}/common/video-x264.conf
    ${USAGE}/common/video-x264-param.conf
    ${USAGE}/common/camera.conf
  DESTINATION
    ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}/common)
