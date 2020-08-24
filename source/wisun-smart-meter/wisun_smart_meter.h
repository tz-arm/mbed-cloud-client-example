// ----------------------------------------------------------------------------
// Copyright 2019-2020 ARM Ltd.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------

#ifndef __WISUN_SMART_METER_H__
#define __WISUN_SMART_METER_H__

#if defined(PLATFORM_WISUN_SMART_METER) && (PLATFORM_WISUN_SMART_METER == 1)

#include <stdlib.h>     /* strtol */
#include <string.h>

#include "mbed.h"
#include "mbed-trace/mbed_trace.h"
#include "simplem2mclient.h"
#include "mbed_app_component.h"

#include "mbed-trace/mbed_trace.h"

#define WISUN_SMART_METER_INTERVAL_MIN  1
#define WISUN_SMART_METER_INTERVAL_MAX  20

#define WISUN_SMART_METER_INTERVAL_PWR  10
#define WISUN_SMART_METER_ALARM_DURATION    5

#define WISUN_SMART_METER_LED_BIT_R 4
#define WISUN_SMART_METER_LED_BIT_G 2
#define WISUN_SMART_METER_LED_BIT_B 1

typedef enum {
    STATE_IDLE,
    STATE_CONNECT,
    STATE_STARTED,
    STATE_POSTPONE,
    STATE_STOPED
} SmartMeterState;

//SmartMeterState _state;
void wisun_smart_meter_init_client(SimpleM2MClient &client);
void wisun_smart_meter_init_display();
void wisun_smart_meter_start();
void wisun_smart_meter_stop();
void wisun_smart_meter_state();

#endif

#endif //__WISUN_SMART_METER_H__