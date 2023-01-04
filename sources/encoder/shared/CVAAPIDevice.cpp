// Copyright (C) 2018-2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
}

#include "CVAAPIDevice.h"
#include "version.h"

CVAAPIDevice *CVAAPIDevice::m_pVaapiDev = nullptr;
static CVAAPIDevice::Maintainer maintainer;

CVAAPIDevice::CVAAPIDevice() : CTransLog(__func__) {
    int ret = 0;
    const char *dev_dri;

    Info("Libtrans version %s\nCopyright (C) 2017 Intel\n", LIBTRANS_VERSION);

    dev_dri = getenv("VAAPI_DEVICE");
    if (!dev_dri)
        dev_dri = "/dev/dri/renderD128";

    ret = av_hwdevice_ctx_create(&m_pDev, AV_HWDEVICE_TYPE_VAAPI, dev_dri, nullptr, 0);
    if (ret < 0) {
        Error("Unable to create a valid VAAPI device. Msg: %s.\n", ErrToStr(ret).c_str());
        return;
    }
}

CVAAPIDevice::~CVAAPIDevice() {
    av_buffer_unref(&m_pDev);
}

CVAAPIDevice *CVAAPIDevice::getInstance() {
    if (!m_pVaapiDev)
        m_pVaapiDev = new CVAAPIDevice();
    return m_pVaapiDev;
}

AVBufferRef *CVAAPIDevice::getVaapiDev() {
    return m_pDev;
}

CVAAPIDevice::Maintainer::Maintainer() {
    // avcodec_register_all();
    // avdevice_register_all();
    // avfilter_register_all();
    // av_register_all();
    // avformat_network_init();

    //getInstance();
}

CVAAPIDevice::Maintainer::~Maintainer() {
    if (m_pVaapiDev) {
        delete m_pVaapiDev;
        m_pVaapiDev = nullptr;
    }
}
