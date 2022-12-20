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

include(ubuntu.m4)

define(`NODESOURCE_URL',https://deb.nodesource.com)
DECLARE(`NODE_VER',node_14.x)

pushdef(`_install_ubuntu',`dnl
INSTALL_PKGS(PKGS(curl ca-certificates gpg))

ARG NODESOURCE_KEY_URL="NODESOURCE_URL/gpgkey/nodesource.gpg.key"
RUN local_key=/usr/share/keyrings/nodesource.gpg && \
  curl -s $NODESOURCE_KEY_URL | gpg --dearmor | tee $local_key >/dev/null && \
  echo "deb [signed-by=$local_key] NODESOURCE_URL/NODE_VER UBUNTU_CODENAME(OS_VERSION) main" >> /etc/apt/sources.list && \
  apt-get update
')

ifelse(OS_NAME:OS_VERSION,ubuntu:20.04,`define(`ENABLE_NODESOURCE_REPO',defn(`_install_ubuntu'))')
ifelse(OS_NAME:OS_VERSION,ubuntu:22.04,`define(`ENABLE_NODESOURCE_REPO',defn(`_install_ubuntu'))')

popdef(`_install_ubuntu')

ifdef(`ENABLE_NODESOURCE_REPO',,dnl
  `ERROR(`Nodesource .m4 template does not support OS_NAME:OS_VERSION')')

include(end.m4)dnl
