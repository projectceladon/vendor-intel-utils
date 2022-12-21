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

include(defs.m4)dnl
define(`INTEL_GFX_FLAVOR_NAME',flex)
include(begin.m4)
define(`LIBVHAL_BUILD_EMU',ON)
include(intel-gfx.m4)
include(ffmpeg.m4)
include(libvhal-client.m4)
include(intel-cloud-encoder.m4)
include(icr.m4)
include(end.m4)
PREAMBLE

ARG IMAGE=OS_NAME:OS_VERSION
FROM $IMAGE AS base

ENABLE_INTEL_GFX_REPO

FROM base as build

BUILD_ALL()dnl
CLEANUP()dnl

# Ok, here goes the final image end-user will actually see
FROM base

LABEL vendor="Intel Corporation"

INSTALL_ALL(runtime,build)

USER user
WORKDIR /home/user
