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

#ifndef CVAAPIDEVICE_H
#define CVAAPIDEVICE_H

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}

#include <memory>
#include "utils/CTransLog.h"

class CVAAPIDevice : private CTransLog {
public:
    static CVAAPIDevice *getInstance();
    AVBufferRef *getVaapiDev();

    class Maintainer {
    public:
        Maintainer();
        ~Maintainer();
    };

private:
    CVAAPIDevice();
    CVAAPIDevice(const CVAAPIDevice& orig);
    CVAAPIDevice& operator=(CVAAPIDevice &orig);
    ~CVAAPIDevice();

private:
    AVBufferRef *m_pDev;
    static CVAAPIDevice* m_pVaapiDev;
};

#endif /* CVAAPIDEVICE_H */

