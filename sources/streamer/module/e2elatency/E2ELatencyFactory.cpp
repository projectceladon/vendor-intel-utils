// Copyright (C) 2022 Intel Corporation
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

#include  "E2ELatencyFactory.h"
#include  "E2ELatencyClient.h"
#include  "E2ELatencyServer.h"

/**
 * Create E2E Latency Client instance
 * Return Base pointer to E2E latency Client
 */
IL::E2ELatencyBase* CreateE2ELatencyClient()
{
    return new IL::E2ELatencyClient();
}

/**
 * Create E2E Latency Server instance
 * Return Base pointer to E2E latency Server
 */
IL::E2ELatencyBase* CreateE2ELatencyServer()
{
    return new IL::E2ELatencyServer();
}
