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

#ifndef __MBED_APPLICATION_SHIELD_H__
#define __MBED_APPLICATION_SHIELD_H__

#include "mbed.h"
#include "C12832.h"
#include "LM75B.h"

#define MBED_CONF_APP_PINNAME_LCD_MOSI  D11
#define MBED_CONF_APP_PINNAME_LCD_MISO  PB_4
#define MBED_CONF_APP_PINNAME_LCD_SCK   D13
#define MBED_CONF_APP_PINNAME_LCD_RESET D12
#define MBED_CONF_APP_PINNAME_LCD_A0    D7
#define MBED_CONF_APP_PINNAME_LCD_NCS   D10

#define MBED_CONF_APP_PINNAME_POT1      A0
#define MBED_CONF_APP_PINNAME_POT2      A1

#define MBED_CONF_APP_PINNAME_JOYSTICK_FIRE D4
#define MBED_CONF_APP_PINNAME_SPEAKER   D6

#define MBED_CONF_APP_PINNAME_SDA       D14
#define MBED_CONF_APP_PINNAME_SCL       D15


void mbed_app_function_test();

#if 0 //TBD
class MbedAppShiled
{
    typedef enum {
        STATE_IDLE,
        STATE_STARTED,
    } BlinkyState;

public:
    MbedAppShiled();

    ~MbedAppShiled();

    void init();
    bool start();
    void stop();

private:

};
#endif

#endif  //__MBED_APPLICATION_SHIELD_H__