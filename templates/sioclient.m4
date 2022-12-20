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

DECLARE(`SIOCLIENT_VER',2.0.0)

define(`SIOCLIENT_BUILD_DEPS',cmake gcc g++ git libasio-dev libssl-dev libwebsocketpp-dev make pkg-config rapidjson-dev)
define(`SIOCLIENT_INSTALL_DEPS',openssl)

define(`BUILD_SIOCLIENT',`dnl
ARG SIOCLIENT_REPO=https://github.com/socketio/socket.io-client-cpp.git
RUN git clone $SIOCLIENT_REPO BUILD_HOME/sioclient && \
  cd BUILD_HOME/sioclient && \
  git checkout SIOCLIENT_VER && \
  mkdir build && cd build && \
  cmake \
    -DCMAKE_INSTALL_PREFIX=BUILD_PREFIX \
    -DCMAKE_INSTALL_LIBDIR=BUILD_LIBDIR \
    -DBUILD_SHARED_LIBS=ON \
    .. && \
  make -j$(nproc) && \
  make install && \
  make install DESTDIR=BUILD_DESTDIR

RUN { \
  echo "prefix=BUILD_PREFIX"; \
  echo "libdir=BUILD_LIBDIR"; \
  echo "includedir=BUILD_PREFIX/include"; \
  echo ""; \
  echo "Name: sioclient"; \
  echo "Description: Socket.IO C++ Client"; \
  echo "Version: SIOCLIENT_VER"; \
  echo ""; \
  echo "Libs: -L\${libdir} -lsioclient"; \
  echo "Cflags: -I\${includedir}"; \
  } > BUILD_HOME/sioclient/sioclient.pc
RUN mkdir -p BUILD_LIBDIR/pkgconfig && \
  cp BUILD_HOME/sioclient/sioclient.pc BUILD_LIBDIR/pkgconfig/ && \
  mkdir -p BUILD_DESTDIR/BUILD_LIBDIR/pkgconfig && \
  cp BUILD_HOME/sioclient/sioclient.pc BUILD_DESTDIR/BUILD_LIBDIR/pkgconfig/
')

REG(SIOCLIENT)

include(end.m4)
