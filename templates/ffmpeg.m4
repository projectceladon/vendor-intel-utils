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

DECLARE(`FFMPEG_VER',`d79c240')
dnl Using github mirror to avoid pulling from different locations which minimizes
dnl dnl impact on the CI in case of outages.
DECLARE(`FFMPEG_REPO_URL',https://github.com/FFmpeg/FFmpeg.git)
DECLARE(`FFMPEG_ENABLE_MFX',`2.x')

define(`FFMPEG_BUILD_DEPS',`ca-certificates gcc g++ git dnl
  ifdef(`BUILD_MSDK',,ifelse(FFMPEG_ENABLE_MFX,1.x,libmfx-dev)) dnl
  ifdef(`BUILD_ONEVPL',,ifelse(FFMPEG_ENABLE_MFX,2.x,libvpl-dev)) dnl
  ifdef(`BUILD_LIBVA2',,libva-dev) dnl
  make patch pkg-config xz-utils yasm')

define(`FFMPEG_INSTALL_DEPS',`dnl
  ifdef(`BUILD_MEDIA_DRIVER',,intel-media-va-driver-non-free libigfxcmrt7) dnl
  ifdef(`BUILD_MSDK',,libmfx1) dnl
  ifdef(`BUILD_ONEVPLGPU',,ifelse(FFMPEG_ENABLE_MFX,2.x,libmfxgen1)) dnl
  ifdef(`BUILD_ONEVPL',,ifelse(FFMPEG_ENABLE_MFX,2.x,libvpl2)) dnl
  ifdef(`BUILD_LIBVA2',,libva-drm2) dnl
  ')

define(`BUILD_FFMPEG',
ARG FFMPEG_REPO=FFMPEG_REPO_URL
RUN git clone $FFMPEG_REPO BUILD_HOME/ffmpeg && \
  cd BUILD_HOME/ffmpeg && \
  git checkout FFMPEG_VER
ifdef(`FFMPEG_PATCH_PATH',`PATCH(BUILD_HOME/ffmpeg,FFMPEG_PATCH_PATH)')dnl

RUN cd BUILD_HOME/ffmpeg && \
  ./configure \
  --prefix=BUILD_PREFIX \
  --libdir=BUILD_LIBDIR \
  --disable-static \
  --disable-doc \
  --enable-shared \
ifelse(FFMPEG_ENABLE_MFX,1.x,`dnl
  --enable-libmfx \
',ifelse(FFMPEG_ENABLE_MFX,2.x,`dnl
  --enable-libvpl \
'))dnl
  && make -j $(nproc --all) \
  && make install DESTDIR=BUILD_DESTDIR \
  && make install
) # define(BUILD_FFMPEG)

REG(FFMPEG)

include(end.m4)
