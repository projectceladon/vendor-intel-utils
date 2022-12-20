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

define(`OWT_P2P_SERVER_VER',4.3.1)

define(`OWT_P2P_SERVER_BUILD_DEPS',ca-certificates wget)
define(`OWT_P2P_SERVER_INSTALL_DEPS',npm tree)

define(`BUILD_OWT_P2P_SERVER',`dnl
ARG OWT_REPO=https://github.com/open-webrtc-toolkit/owt-server-p2p/archive/v`'OWT_P2P_SERVER_VER.tar.gz
RUN mkdir -p BUILD_DESTDIR/BUILD_PREFIX/share && \
  cd BUILD_DESTDIR/BUILD_PREFIX/share && \
  wget -O - ${OWT_REPO} | tar xz
')

define(`INSTALL_OWT_P2P_SERVER',`dnl
RUN ls /opt/share/
RUN cd BUILD_PREFIX/share/owt-server-p2p-OWT_P2P_SERVER_VER && \
  npm install
EXPOSE 8095
EXPOSE 8096
')

REG(OWT_P2P_SERVER)

include(end.m4)
