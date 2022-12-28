dnl BSD 3-Clause License
dnl
dnl Copyright (C) 2020-2022, Intel Corporation
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions are met:
dnl
dnl * Redistributions of source code must retain the above copyright notice, this
dnl   list of conditions and the following disclaimer.
dnl
dnl * Redistributions in binary form must reproduce the above copyright notice,
dnl   this list of conditions and the following disclaimer in the documentation
dnl   and/or other materials provided with the distribution.
dnl
dnl * Neither the name of the copyright holder nor the names of its
dnl   contributors may be used to endorse or promote products derived from
dnl   this software without specific prior written permission.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
dnl AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
dnl DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
dnl SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
dnl CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
dnl OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
dnl OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
dnl
include(begin.m4)

DECLARE(`LIBVHAL_SRC_REPO',https://github.com/projectceladon/libvhal-client.git)
DECLARE(`LIBVHAL_CLIENT_VER',3980f24)
DECLARE(`LIBVHAL_BUILD_EMU',OFF)

define(`LIBVHAL_CLIENT_BUILD_DEPS',`ca-certificates cmake gcc g++ git dnl
  ifelse(LIBVHAL_BUILD_EMU,ON,libyaml-cpp-dev) dnl
  make pkg-config xz-utils')

define(`LIBVHAL_CLIENT_INSTALL_DEPS',`dnl
  ifelse(LIBVHAL_BUILD_EMU,ON,libyaml-cpp0.7)')

define(`BUILD_LIBVHAL_CLIENT',`dnl
ARG LIBVHAL_CLIENT_REPO=LIBVHAL_SRC_REPO
RUN git clone $LIBVHAL_CLIENT_REPO BUILD_HOME/libvhal-client && \
  cd BUILD_HOME/libvhal-client && \
  git checkout LIBVHAL_CLIENT_VER && \
  mkdir -p build && cd build && \
  cmake -DCMAKE_INSTALL_PREFIX=BUILD_PREFIX \
        -DCMAKE_INSTALL_LIBDIR=BUILD_LIBDIR \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_EMULATOR_APP=LIBVHAL_BUILD_EMU \
        .. && \
  make -j$(nproc) && \
  make install DESTDIR=BUILD_DESTDIR && \
  make install
')

REG(LIBVHAL_CLIENT)

include(end.m4)
