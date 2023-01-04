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

#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_MICRO 0

#define TOSTRING(a) #a
#define VERSION_INT(a, b, c) ((a)<<16 | (b)<<8 | (c))
#define VERSION(a, b, c) TOSTRING(a.b.c)

#define LIBTRANS_VERSION VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO)
#define LIBTRANS_VERSION_INT VERSION_INT(VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO)

#endif /* VERSION_H */

