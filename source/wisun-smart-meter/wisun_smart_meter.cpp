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

#include "wisun_smart_meter.h"

static SimpleM2MClient *_client;
static M2MResource *_res_led_color;
static M2MResource *_res_led_enable;
static M2MResource *_res_alarm_enable;
static M2MResource *_res_alarm_duration;
static M2MResource *_res_temp_value;
static M2MResource *_res_temp_sfreq;
static M2MResource *_res_cur_value;
static M2MResource *_res_cur_sfreq;
static M2MResource *_res_vol_value;
static M2MResource *_res_vol_sfreq;
static M2MResource *_res_pwr_ainst;
static M2MResource *_res_pwr_acuml;   
static M2MResource *_res_pwr_acuml_reset;    
static M2MResource *_res_pwr_sfreq;

static EventQueue *_event_queue_msg;
static EventQueue *_event_queue_alarm;

static Ticker _timer_pwr;
static Ticker _timer_tmp;
static Ticker _timer_vol;
static Ticker _timer_cur;

static float _cur;
static float _vol;
static float _pwr;
static float _tmp;
static float _cur_sfreq;
static float _vol_sfreq;
static float _pwr_sfreq;
static float _tmp_sfreq;

static void _init_display();
static void _update_current();
static void _update_voltage();
static void _update_temporature();
static void _update_power();

static void callback_tempfreq_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    float sfreq = _res_temp_sfreq->get_value_float();
    if((sfreq<WISUN_SMART_METER_INTERVAL_MIN)||(sfreq>WISUN_SMART_METER_INTERVAL_MIN))
    {
        tr_error("[SMeter] The requested sampling freqency is out of scope.");
        return;
    }

    if(sfreq!=_tmp_sfreq)
    {
        _tmp_sfreq = sfreq;
        _timer_tmp.detach();
        _timer_tmp.attach(_event_queue_msg->event(&_update_temporature), _tmp_sfreq);

        tr_info("[SMeter][%s] Temp sampling freqency has been updated to %.1f \r\n", __FUNCTION__, _tmp_sfreq);
    }
}

static void callback_curfreq_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    float sfreq = _res_cur_sfreq->get_value_float();
    if((sfreq<WISUN_SMART_METER_INTERVAL_MIN)||(sfreq>WISUN_SMART_METER_INTERVAL_MIN))
    {
        tr_error("[SMeter] The requested sampling freqency is out of scope.");
        return;
    }

    if(sfreq!=_cur_sfreq)
    {
        _cur_sfreq = sfreq;
        _timer_tmp.detach();
        _timer_tmp.attach(_event_queue_msg->event(&_update_current), _cur_sfreq);

        tr_info("[SMeter][%s] current sampling freqency has been updated to %.1f \r\n", __FUNCTION__, _cur_sfreq);
    }

}

static void callback_volfreq_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    float sfreq = _res_temp_sfreq->get_value_float();
    if((sfreq<WISUN_SMART_METER_INTERVAL_MIN)||(sfreq>WISUN_SMART_METER_INTERVAL_MIN))
    {
        tr_error("[SMeter] The requested sampling freqency is out of scope.");
        return;
    }

    if(sfreq!=_vol_sfreq)
    {
        _vol_sfreq = sfreq;
        _timer_tmp.detach();
        _timer_tmp.attach(_event_queue_msg->event(&_update_voltage), _vol_sfreq);

        tr_info("[SMeter][%s] Voltage sampling freqency has been updated to %.1f \r\n", __FUNCTION__, _vol_sfreq);
    }
}

static void callback_pwrfreq_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    float sfreq = _res_pwr_sfreq->get_value_float();
    if((sfreq<WISUN_SMART_METER_INTERVAL_MIN)||(sfreq>WISUN_SMART_METER_INTERVAL_MIN))
    {
        tr_error("[SMeter] The requested sampling freqency is out of scope.");
        return;
    }

    if(sfreq!=_pwr_sfreq)
    {
        _pwr_sfreq = sfreq;
        _timer_tmp.detach();
        _timer_tmp.attach(_event_queue_msg->event(&_update_voltage), _pwr_sfreq);

        tr_info("[SMeter][%s] Power sampling freqency has been updated to %.1f \r\n", __FUNCTION__, _pwr_sfreq);
    }
}

static void callback_pwrreset_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    _timer_pwr.detach();
    _pwr = 0;
    _res_pwr_acuml->set_value(_pwr);
    _timer_pwr.attach(_event_queue_msg->event(&_update_power), WISUN_SMART_METER_INTERVAL_PWR);

    tr_info("[SMeter][%s] Cumulative active power has been reset.\n", __FUNCTION__, _pwr_sfreq);
}

static void callback_leden_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);
}
static void callback_ledcol_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);
}
static void callback_almen_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);
}
static void callback_almdur_updated(const char *)
{
    tr_debug("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);
}

static void callback_notification_status(const M2MBase& object,
                            const M2MBase::MessageDeliveryStatus status,
                            const M2MBase::MessageType /*type*/)
{
    switch(status) {
        case M2MBase::MESSAGE_STATUS_BUILD_ERROR:
            printf("Message status callback: (%s) error when building CoAP message\r\n", object.uri_path());
            break;
        case M2MBase::MESSAGE_STATUS_RESEND_QUEUE_FULL:
            printf("Message status callback: (%s) CoAP resend queue full\r\n", object.uri_path());
            break;
        case M2MBase::MESSAGE_STATUS_SENT:
            printf("Message status callback: (%s) Message sent to server\r\n", object.uri_path());
            break;
        case M2MBase::MESSAGE_STATUS_DELIVERED:
            printf("Message status callback: (%s) Message delivered\r\n", object.uri_path());
            break;
        case M2MBase::MESSAGE_STATUS_SEND_FAILED:
            printf("Message status callback: (%s) Message sending failed\r\n", object.uri_path());
            break;
        case M2MBase::MESSAGE_STATUS_SUBSCRIBED:
            printf("Message status callback: (%s) subscribed\r\n", object.uri_path());
            break;
        case M2MBase::MESSAGE_STATUS_UNSUBSCRIBED:
            printf("Message status callback: (%s) subscription removed\r\n", object.uri_path());
            break;
        case M2MBase::MESSAGE_STATUS_REJECTED:
            printf("Message status callback: (%s) server has rejected the message\r\n", object.uri_path());
            break;
        default:
            break;
    }
}

static void _update_temporature()
{
    static float tmpvalue;
    tmpvalue = mbed_app_get_temperature();
    if(tmpvalue!=_tmp)
    {
        _tmp = tmpvalue;
    
        //if((_client->is_client_registered())&&(_res_temp_value!=NULL))
        if(_res_temp_value!=NULL)
        {
            _res_temp_value->set_value(_tmp);
        }

        mbed_app_lcd_fresh(_tmp,_vol,_cur,_pwr);
    }
}

static void _update_current()
{
    static float curvalue;
    curvalue = mbed_app_get_current();
    if(curvalue!=_cur)
    {
        _cur = curvalue;
    
        if(_res_cur_value!=NULL)
        {
            _res_cur_value->set_value(_cur);
        }

        mbed_app_lcd_fresh(_tmp,_vol,_cur,_pwr);
    }
}

static void _update_voltage()
{
    static float volvalue;
    volvalue = mbed_app_get_voltage();
    if(volvalue!=_vol)
    {
        _vol = volvalue;
    
        if(_res_vol_value!=NULL)
        {
            _res_vol_value->set_value(_vol);
        }

        mbed_app_lcd_fresh(_tmp,_vol,_cur,_pwr);
    }
}

static void _update_power()
{
    float pwr_ainst = _vol*_cur;
    
    // 
    _pwr += pwr_ainst*_pwr_sfreq;
   
    if((_res_pwr_ainst!=NULL)&&(_res_pwr_acuml!=NULL))
    {
        _res_pwr_ainst->set_value(pwr_ainst);
        _res_pwr_acuml->set_value(_pwr);
    }

    mbed_app_lcd_fresh(_tmp,_vol,_cur,_pwr);
}

static void _init_display()
{
    mbed_app_lcd_init();
}


// use references to encourage caller to pass this existing object
void wisun_smart_meter_init(SimpleM2MClient &client)
{
    printf("# wisun_smart_meter_init >> 1\n");
    // Do not start if client has not been assigned.
    if (_client) return;
    _client = &client;
    printf("# wisun_smart_meter_init >> 2\n");

    _pwr = 0;
    _cur = mbed_app_get_current();
    _vol = mbed_app_get_voltage();
    _tmp = mbed_app_get_temperature();

    _init_display();
    _update_current();
    _update_voltage();
    _update_temporature();

    _cur_sfreq = WISUN_SMART_METER_INTERVAL_CUR;
    _vol_sfreq = WISUN_SMART_METER_INTERVAL_VOL;
    _pwr_sfreq = WISUN_SMART_METER_INTERVAL_PWR;
    _tmp_sfreq = WISUN_SMART_METER_INTERVAL_TMP;

}

void wisun_smart_meter_start()
{
    _res_temp_value = _client->add_cloud_resource(3303, 0, 5700, "SMeter: ", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_temp_sfreq = _client->add_cloud_resource(3303, 0, 6040, "SMeter: ", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_tempfreq_updated, (void*)callback_notification_status);
    _res_cur_value = _client->add_cloud_resource(3316, 0, 5700, "SMeter: ", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_cur_sfreq = _client->add_cloud_resource(3316, 0, 6040, "SMeter: ", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_curfreq_updated, (void*)callback_notification_status);
    _res_vol_value = _client->add_cloud_resource(3317, 0, 5700, "SMeter: ", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_vol_sfreq = _client->add_cloud_resource(3317, 0, 6040, "SMeter: ", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_volfreq_updated, (void*)callback_notification_status);
    _res_led_enable = _client->add_cloud_resource(3311, 0, 5850, "SMeter: ", M2MResourceInstance::BOOLEAN,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_leden_updated, (void*)callback_notification_status);
    _res_led_color = _client->add_cloud_resource(3311, 0, 5706, "SMeter: ", M2MResourceInstance::STRING,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_ledcol_updated, (void*)callback_notification_status);
    _res_alarm_enable = _client->add_cloud_resource(3338, 0, 5850, "SMeter: ", M2MResourceInstance::BOOLEAN,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_almen_updated, (void*)callback_notification_status);
    _res_alarm_duration = _client->add_cloud_resource(3338, 0, 5521, "SMeter: ", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_almdur_updated, (void*)callback_notification_status);

    _res_pwr_ainst = _client->add_cloud_resource(3305, 0, 5800, "SMeter: ", M2MResourceInstance::STRING,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_pwr_acuml = _client->add_cloud_resource(3305, 0, 5805, "SMeter: ", M2MResourceInstance::BOOLEAN,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_pwr_sfreq = _client->add_cloud_resource(3305, 0, 5822, "SMeter: ", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_pwrfreq_updated, (void*)callback_notification_status);
    _res_pwr_acuml_reset = _client->add_cloud_resource(3305, 0, 6040, "SMeter: ", M2MResourceInstance::FLOAT,M2MBase::POST_ALLOWED, 0, true, (void*)callback_pwrreset_updated, (void*)callback_notification_status);

    // Set the sample freqency to 1 second as default.
    _res_temp_sfreq->set_value(_tmp_sfreq);
    _res_cur_sfreq->set_value(_cur_sfreq);
    _res_vol_sfreq->set_value(_vol_sfreq);
    _res_pwr_sfreq->set_value(_pwr_sfreq);

    _res_pwr_ainst->set_value(0);
    _res_pwr_acuml->set_value(0);
    _res_pwr_acuml_reset->set_value(0);

    _res_led_enable->set_value(0);
    _res_alarm_enable->set_value(0);
    _res_alarm_duration->set_value(WISUN_SMART_METER_ALARM_DURATION);

    _event_queue_msg = mbed_event_queue();
    _event_queue_alarm = mbed_highprio_event_queue();

    _event_queue_msg->dispatch_forever();
    _event_queue_alarm->dispatch_forever();

    _timer_pwr.attach(_event_queue_msg->event(&_update_power), _pwr_sfreq);
    _timer_tmp.attach(_event_queue_msg->event(&_update_temporature), _tmp_sfreq);
    _timer_vol.attach(_event_queue_msg->event(&_update_voltage), _vol_sfreq);
    _timer_cur.attach(_event_queue_msg->event(&_update_current), _cur_sfreq);
}


void wisun_smart_meter_stop()
{
    _timer_pwr.detach();
    _timer_tmp.detach();
    _timer_vol.detach();
    _timer_cur.detach();
}
