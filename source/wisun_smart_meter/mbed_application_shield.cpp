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

#include "mbed_application_shield.h"

//Using Arduino pin notation
//C12832(PinName mosi, PinName sck, PinName reset, PinName a0, PinName ncs, const char* name = "LCD");
C12832 lcd(MBED_CONF_APP_PINNAME_LCD_MOSI, 
            MBED_CONF_APP_PINNAME_LCD_MISO, 
            MBED_CONF_APP_PINNAME_LCD_SCK, 
            MBED_CONF_APP_PINNAME_LCD_RESET, 
            MBED_CONF_APP_PINNAME_LCD_A0, 
            MBED_CONF_APP_PINNAME_LCD_NCS);

AnalogIn pot1(MBED_CONF_APP_PINNAME_POT1);
AnalogIn pot2(MBED_CONF_APP_PINNAME_POT2);

DigitalIn fire(MBED_CONF_APP_PINNAME_JOYSTICK_FIRE);
PwmOut speaker(MBED_CONF_APP_PINNAME_SPEAKER);
LM75B sensor(MBED_CONF_APP_PINNAME_SDA,MBED_CONF_APP_PINNAME_SCL);


void mbed_app_function_test()
{
    int i;

    //LCD
    lcd.cls();
    lcd.locate(0,3);
    lcd.printf("mbed application shield");
    ThisThread::sleep_for(1000);

    while(!fire)
    {
        lcd.locate(0,15);
        lcd.printf("LCD Test Counting : %d",i++);
        ThisThread::sleep_for(1000);
    }

    lcd.cls();
    lcd.locate(0,3);
    lcd.printf("mbed application shield");
    ThisThread::sleep_for(1000);

    while(!fire)
    {
        lcd.locate(0,13);
        lcd.printf("POT TEST 1 = %.2f", (float)pot1);
        lcd.locate(0,23);
        lcd.printf("POT TEST 2 = %.2f", (float)pot2);
        ThisThread::sleep_for(10);
    }

    lcd.cls();
    lcd.locate(0,3);
    lcd.printf("mbed application shield");
    lcd.locate(0,15);
    lcd.printf("speaker test");
    ThisThread::sleep_for(1000);

    while(!fire)
    {
        for (float i=2000.0; i<10000.0; i+=100) {
            speaker.period(1.0/i);
            speaker = 0.5;
            ThisThread::sleep_for(10);
        }
        speaker = 0.0;
        ThisThread::sleep_for(1000);
    }

    lcd.cls();
    lcd.locate(0,3);
    lcd.printf("mbed application shield");
    ThisThread::sleep_for(1000);

    while(!fire)
    {
        lcd.locate(0,15);
        lcd.printf("Temp Sensor = %.1f\n", sensor.temp());
        ThisThread::sleep_for(100);
    }
}