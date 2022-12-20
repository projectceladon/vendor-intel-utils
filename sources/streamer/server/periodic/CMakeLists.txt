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

add_executable(ga-server-periodic ga-server-periodic.cpp)

target_compile_definitions(ga-server-periodic
  PRIVATE
    GA_MODULE_PREFIX="${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/")

target_link_libraries(ga-server-periodic
  PRIVATE
    ga
    Threads::Threads)

install(TARGETS ga-server-periodic RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/)
