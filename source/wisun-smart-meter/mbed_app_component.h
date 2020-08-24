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

#ifndef __MBED_APP_COMPONENT_H__
#define __MBED_APP_COMPONENT_H__

#if defined(PLATFORM_WISUN_SMART_METER) && (PLATFORM_WISUN_SMART_METER == 1)

#include "mbed.h"
#include "C12832.h"
#include "LM75B.h"
#include "mbed-trace/mbed_trace.h"
#include "platform/arm_hal_interrupt.h"

#define TRACE_GROUP "SMeter"
#define DBG tr_warn

#define MBED_CONF_APP_PINNAME_LCD_MOSI  D11
#define MBED_CONF_APP_PINNAME_LCD_MISO  D12 //PTC7, K64F
#define MBED_CONF_APP_PINNAME_LCD_SCK   D13
#define MBED_CONF_APP_PINNAME_LCD_RESET D3
#define MBED_CONF_APP_PINNAME_LCD_A0    D7
#define MBED_CONF_APP_PINNAME_LCD_NCS   D10

#define MBED_CONF_APP_PINNAME_POT1      A0
#define MBED_CONF_APP_PINNAME_POT2      A1

#define MBED_CONF_APP_PINNAME_JOYSTICK_FIRE D4
#define MBED_CONF_APP_PINNAME_SPEAKER   D6

#define MBED_CONF_APP_PINNAME_SDA       D14
#define MBED_CONF_APP_PINNAME_SCL       D15

#define MBED_CONF_APP_PINNAME_LED_R     D5
#define MBED_CONF_APP_PINNAME_LED_G     D9
#define MBED_CONF_APP_PINNAME_LED_B     D8

void mbed_app_driver_test();

float mbed_app_get_temperature();
float mbed_app_get_current();
float mbed_app_get_voltage();

void mbed_app_led_init();
void mbed_app_led_ctl(int enable, int color);

void mbed_app_lcd_init();
void mbed_app_lcd_clear();
void mbed_app_lcd_fresh(float tmp, float vol, float cur, float pwr);

#endif

#endif  //__MBED_APP_COMPONENT_H__