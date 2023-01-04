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

#ifndef CQSVAPIDEVICE_H
#define CQSVAPIDEVICE_H

#ifdef ENABLE_QSV

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_qsv.h>
}

#include <memory>
#include "utils/CTransLog.h"

class CQSVAPIDevice : private CTransLog {
public:
    static CQSVAPIDevice *getInstance();
    AVBufferRef *getQSVapiDev();
    class Maintainer {
    public:
        Maintainer();
        ~Maintainer();
    };

private:
    CQSVAPIDevice();
    CQSVAPIDevice(const CQSVAPIDevice& orig);
    CQSVAPIDevice& operator=(CQSVAPIDevice &orig);
    ~CQSVAPIDevice();

private:
    AVBufferRef *m_pDev;
    static CQSVAPIDevice* m_pQSVapiDev;
};

#endif
#endif /* CQSVAPIDEVICE_H */

