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

define(`ICR_INSTALL_DEPS',`dnl
  iproute2 dnl
  ifdef(`BUILD_LIBVA2_UTILS',,vainfo) dnl
  ifdef(`DEVEL',`sudo wget')')

define(`INSTALL_ICR',`dnl
# Installing entrypoint helper scripts
COPY assets/demo-alive /usr/bin/
COPY assets/demo-bash /usr/bin/
COPY assets/hello-bash /usr/bin/
COPY assets/demo-setup BUILD_PREFIX/bin/

RUN { \
  echo "export DEMO_NAME=ga"; \
  echo "export DEMO_PREFIX=BUILD_PREFIX"; \
  echo "export MANPATH=\$DEMO_PREFIX/share/man:\$MANPATH"; \
  echo "export PATH=\$DEMO_PREFIX/bin:\$PATH"; \
  echo "export LIBVA_DRIVER_NAME=iHD"; \
  echo "export DEVICE=\${DEVICE:-/dev/dri/renderD128}"; \
} > /etc/demo.env

# Create default container user <user>
RUN groupadd -r user && useradd -lrm -s /bin/bash -g user user
ifdef(`DEVEL',`dnl
RUN usermod -aG sudo user
RUN sed -i -e "s/%sudo.*/%sudo ALL=(ALL) NOPASSWD:ALL/g" /etc/sudoers'
)

# Setting up environment common for all samples

# Check running container healthy status with:
#  docker inspect --format="{{json .State.Health}}" <container-id>
HEALTHCHECK CMD /usr/bin/demo-alive

# hello-bash is a default command which will be executed by demo-bash if
# user did not provide any arguments starting the container. Basically hello-bash
# will print welcome message and enter regular bash with correct environment.
CMD ["/usr/bin/hello-bash"]

# demo-bash will execute whatever command is provided by the user making
# sure that environment settings are correct.
ENTRYPOINT ["/usr/bin/demo-bash"]') # define(INSTALL_ICR)

REG(ICR)

include(end.m4)
