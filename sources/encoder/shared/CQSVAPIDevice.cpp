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

#ifdef ENABLE_QSV

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
}

#include "CQSVAPIDevice.h"
#include "version.h"

CQSVAPIDevice *CQSVAPIDevice::m_pQSVapiDev = nullptr;
static CQSVAPIDevice::Maintainer maintainer;

CQSVAPIDevice::CQSVAPIDevice() : CTransLog(__func__) {
    int ret = 0;
    AVDictionary *dict = NULL;
    const char *dev_dri;

    Info("Libtrans version %s\nCopyright (C) 2017 Intel\n", LIBTRANS_VERSION);

    dev_dri = getenv("VAAPI_DEVICE");
    if (!dev_dri)
        dev_dri = "/dev/dri/renderD128";

    ret = av_dict_set(&dict, "child_device", dev_dri, 0);
    if (ret < 0) {
       Error("Unable to set child device. Msg: %s.\n", ErrToStr(ret).c_str());
       return;
    }

    // for QSV device (3-d argument) is mapped to MFX_IMPL_*, supported values are "hw", "hw_any", etc.
    ret = av_hwdevice_ctx_create(&m_pDev, AV_HWDEVICE_TYPE_QSV, "hw", dict, 0);
    if (ret < 0) {
        Error("Unable to create a valid QSV device. Msg: %s.\n", ErrToStr(ret).c_str());
        return;
    }
}

CQSVAPIDevice::~CQSVAPIDevice() {
    av_buffer_unref(&m_pDev);
    m_pQSVapiDev = nullptr;
}

CQSVAPIDevice *CQSVAPIDevice::getInstance() {
    if (!m_pQSVapiDev)
        m_pQSVapiDev = new CQSVAPIDevice();
    return m_pQSVapiDev;
}

AVBufferRef *CQSVAPIDevice::getQSVapiDev() {
    return m_pDev;
}

CQSVAPIDevice::Maintainer::Maintainer() {
    // avcodec_register_all();
    // avdevice_register_all();
    // avfilter_register_all();
    // av_register_all();
    // avformat_network_init();

    //getInstance();
}

CQSVAPIDevice::Maintainer::~Maintainer() {
    if (m_pQSVapiDev) {
        delete m_pQSVapiDev;
        m_pQSVapiDev = nullptr;
    }
}

#endif
