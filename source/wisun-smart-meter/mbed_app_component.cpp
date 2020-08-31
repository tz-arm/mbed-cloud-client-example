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

#include "mbed_app_component.h"

#if defined(PLATFORM_WISUN_SMART_METER) && (PLATFORM_WISUN_SMART_METER == 1)

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


PwmOut led_r (MBED_CONF_APP_PINNAME_LED_R);
PwmOut led_g (MBED_CONF_APP_PINNAME_LED_G);
DigitalOut led_b (MBED_CONF_APP_PINNAME_LED_B);

void mbed_app_lcd_init()
{
    DBG(" mbed_app_lcd_ini >>>\n");
    
    lcd.cls();
    lcd.invert(0);
    ThisThread::sleep_for(100);
    lcd.invert(1);
    ThisThread::sleep_for(100);  
    lcd.invert(0); 
    ThisThread::sleep_for(100);
    lcd.invert(1);

    lcd.cls();
    lcd.locate(15,10);
    lcd.printf("WISUN-SMART-METER");
}

void mbed_app_lcd_clear()
{
    lcd.cls();
}

void mbed_app_lcd_fresh(float tmp, float vol, float cur, float pwr)
{
    platform_enter_critical();
    lcd.locate(0,5);
    lcd.printf(" TMP: %2.1f PWR: %2.4f Wh",tmp, pwr/1000);
    lcd.locate(0,18);
    lcd.printf(" CUR: %2.1fA VOL: %3.1fV\n",cur,vol);
    platform_exit_critical();
}

void mbed_app_led_init()
{
    led_r.period(2);
    led_g.period(2);
    led_r = 0.5;
    led_b = 1;
    led_g = 1;
}

void mbed_app_led_ctl(int enable, int color)
{
    //DBG("\n#mbed_app_led_ctl Enable %x, Color: %d - (R %d,G %d,B %d)\n",enable, color,(color&0x4)?1:0,(color&0x2)?1:0,(color&0x1)?1:0);
    if(enable)
    {
        led_r = (color&0x4)?0:1;
        led_g = (color&0x2)?0:1;
        led_b = (color&0x1)?0:1;
    }
    else
    {
        led_r = 1;
        led_g = 1;
        led_b = 1;
    }
}

float mbed_app_get_temperature()
{
    //lcd.printf("Temp Sensor = %.1f\n", sensor.temp());
    return sensor.temp();
}

float mbed_app_get_current()
{
    // Simulate Current value (0~40A)
    return (float)(pot1*5);
}

float mbed_app_get_voltage()
{
    // Simulate Votage value (0~250V)
    return (float)(pot2*50+200);
}


#ifdef DRIVER_TEST
void mbed_app_driver_test()
{
    int i = 0;

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
#endif

#endif