dnl BSD 3-Clause License
dnl
dnl Copyright (C) 2022, Intel Corporation
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

define(`CHROME_INSTALL_DEPS',`curl ca-certificates gpg dnl
  ifdef(`ENABLE_NODESOURCE_REPO',nodejs,npm) iproute2 ifdef(`DEVEL',`sudo wget')')

define(`INSTALL_CHROME',`dnl
RUN local_key=/usr/share/keyrings/chrome.gpg && \
  curl -s https://dl.google.com/linux/linux_signing_key.pub | gpg --dearmor | tee $local_key >/dev/null && \
  echo "deb [signed-by=$local_key arch=amd64] http://dl.google.com/linux/chrome/deb/ stable main" >> /etc/apt/sources.list && \
  apt-get update

INSTALL_PKGS(PKGS(google-chrome-stable))

# Create default container user <user>
RUN groupadd -r user && useradd -lrm -s /bin/bash -g user user
ifdef(`DEVEL',`dnl
RUN usermod -aG sudo user
RUN sed -i -e "s/%sudo.*/%sudo ALL=(ALL) NOPASSWD:ALL/g" /etc/sudoers'
)

RUN cd /home/user && \
  npm install lodash puppeteer-core png-quality

COPY tests/js/screenshot.js /home/user/
')

REG(CHROME)

include(end.m4)
