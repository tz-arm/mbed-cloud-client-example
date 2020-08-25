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

#if defined(PLATFORM_WISUN_SMART_METER) && (PLATFORM_WISUN_SMART_METER == 1)

static SimpleM2MClient *_client;
static M2MResource *_res_led_color;
static M2MResource *_res_led_enable;
static M2MResource *_res_alarm_enable;
static M2MResource *_res_alarm_duration;
static M2MResource *_res_temp_value;
//static M2MResource *_res_temp_sfreq;
static M2MResource *_res_cur_value;
//static M2MResource *_res_cur_sfreq;
static M2MResource *_res_vol_value;
//static M2MResource *_res_vol_sfreq;
static M2MResource *_res_pwr_ainst;
static M2MResource *_res_pwr_acuml;   
static M2MResource *_res_pwr_acuml_reset;    
static M2MResource *_res_pwr_sfreq;
static M2MResource *_res_pwr_control;

#ifdef WISUN_SMART_METER_NM
static M2MResource* _res_nm_iid;
#endif

static EventQueue *_event_queue_msg;
static EventQueue *_event_queue_alarm;

static Thread _thread_smeter(osPriorityLow);
//static Thread _thread_smeter(osPriorityNormal);

static Ticker _timer_sfreq;

static SmartMeterState _state = STATE_IDLE;

static float _cur;
static float _vol;
static float _pwr;
static float _tmp;
static float _pwr_sfreq;

#ifdef WISUN_SMART_METER_NM
static void _update_iid();
#endif
static void _update_resource();
static void _update_sensor();
static void _update_state(SmartMeterState state);

static void callback_pwrfreq_updated(const char *)
{
    tr_warn("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    float sfreq = _res_pwr_sfreq->get_value_float();
    if((sfreq<WISUN_SMART_METER_INTERVAL_MIN)||(sfreq>WISUN_SMART_METER_INTERVAL_MAX))
    {
        tr_error("[SMeter] The requested sampling freqency is out of scope.");
        return;
    }

    if(sfreq!=_pwr_sfreq)
    {
        _pwr_sfreq = sfreq;
        _res_pwr_sfreq->set_value_float(_pwr_sfreq);

        _timer_sfreq.detach();
        _timer_sfreq.attach(_event_queue_msg->event(&_update_resource), _pwr_sfreq);
        tr_warn("[SMeter][%s] Power sampling freqency has been updated to %.1f \r\n", __FUNCTION__, _pwr_sfreq);
    }
}

static void callback_pwrreset_updated(const char *)
{
    tr_warn("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);
    //_timer_sfreq.detach();
    _pwr = 0;
    _res_pwr_acuml->set_value(0);
    //_timer_sfreq.attach(_event_queue_msg->event(&_update_resource), _pwr_sfreq);
    tr_warn("[SMeter][%s] Cumulative active power has been reset.\n", __FUNCTION__);
}

static void callback_pwrctl_updated(const char *)
{
    tr_warn("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    if(_res_pwr_control->get_value_int())
    {
        _update_state(STATE_STARTED);
    }
    else
    {
        _update_state(STATE_POSTPONE);
    }

    tr_warn("[SMeter][%s] Cumulative active power has been reset.\n", __FUNCTION__);
}

static void callback_led_updated(const char *)
{
    tr_warn("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);

    int en = _res_led_enable->get_value_int();
    int col = _res_led_color->get_value_int();

    mbed_app_led_ctl(en,col);
}

static void callback_almen_updated(const char *)
{
    tr_warn("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);
}
static void callback_almdur_updated(const char *)
{
    tr_warn("[SMeter] callback %s has been triggered!\r\n", __FUNCTION__);
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

#ifdef WISUN_SMART_METER_NM
static void _update_iid()
{
    static SocketAddress sa;
    nsapi_error_t err;
    static uint8_t IIDs[8];
    uint8_t* ip_t;
    
    NetworkInterface* netif = (NetworkInterface*) mcc_platform_interface_get();
    err = netif->get_ip_address(&sa);
    ip_t = (uint8_t*)sa.get_ip_bytes();

    tr_warn("[SMeter][%s-%d] IP bytes: \n", __FUNCTION__,__LINE__);
    for(int i =0; i<16; i++)
    {
        printf("%2x ", ip_t[i]);
    }
    printf("\r\n");
    memcpy(IIDs,ip_t+8,8);

    if(_res_nm_iid!=NULL)
        _res_nm_iid->set_value_raw(IIDs,8);
}
#endif

static void _update_resource()
{
    static float tmp = 0;
    static float cur = 0;
    static float vol = 0;
    static float pwr_ainst = 0;
    static float pwr_acuml = 0;
    tr_warn("[SMeter][%s-%d] Resource Update \n", __FUNCTION__,__LINE__);

    if(tmp!=_tmp)
    {   
        tr_warn("[SMeter][%s-%d] TMP %f \n", __FUNCTION__,__LINE__,_tmp);
        tmp = _tmp;
        _res_temp_value->set_value_float(tmp);
    }

    if(cur!=_cur)
    {   
        tr_warn("[SMeter][%s-%d] CUR %f \n", __FUNCTION__,__LINE__,_cur);
        cur = _cur;
        _res_cur_value->set_value_float(cur);
    }

    if(vol!=_vol)
    {   
        tr_warn("[SMeter][%s-%d] VOL %f \n", __FUNCTION__,__LINE__,_vol);
        vol = _vol;
        _res_vol_value->set_value_float(vol);
    }  

/*
    if(pwr_ainst!=_cur*_vol)
    {   
        tr_warn("[SMeter][%s-%d]  \n", __FUNCTION__,__LINE__);
        pwr_ainst = _cur*_vol;
        _res_pwr_ainst->set_value_float(pwr_ainst);
    }
*/

    if(pwr_acuml!=_pwr)
    {   
        tr_warn("[SMeter][%s-%d] PWR %f\n", __FUNCTION__,__LINE__,_pwr);
        pwr_acuml = _pwr;
        _res_pwr_acuml->set_value_float(pwr_acuml);
    }

}

static void _update_sensor()
{
    tr_warn("[SMeter][%s-%d] Sensor Update \n", __FUNCTION__,__LINE__);
    while(_state!=STATE_STOPED)
    {
        _tmp = mbed_app_get_temperature();
        _vol = mbed_app_get_voltage();
        if(_state == STATE_STARTED)
        {
            _cur = mbed_app_get_current();
            _pwr += _cur*_vol/7200;
        }
        else
        {
            _cur = 0;
        }

        //tr_warn("[SMeter][%s-%d] %2.1f, %3.3f, %2.1f, %3.1f \n", __FUNCTION__,__LINE__, _tmp, _pwr, _cur, _vol);
        mbed_app_lcd_fresh(_tmp,_vol,_cur,_pwr);  
        ThisThread::sleep_for(1000); 
    }
}

static void _update_state(SmartMeterState state)
{
    if(_state==state)
    {
        tr_warn("[SMeter][%s-%d] No state update \n", __FUNCTION__,__LINE__);
        return;
    }

    if(state == STATE_CONNECT)
    {
        tr_warn("[SMeter][%s-%d] State Update to STATE_CONNECT\n", __FUNCTION__,__LINE__);
        _state = STATE_CONNECT;

        _res_led_enable->set_value(true);
        _res_led_color->set_value(WISUN_SMART_METER_LED_BIT_R);
        mbed_app_led_ctl(true,WISUN_SMART_METER_LED_BIT_R);
    }
    else if(state == STATE_STARTED)
    {
        tr_warn("[SMeter][%s-%d] State Update to STATE_STARTED\n", __FUNCTION__,__LINE__);
        _state = STATE_STARTED;       
        _res_led_enable->set_value(true);
        _res_led_color->set_value(WISUN_SMART_METER_LED_BIT_G);
        mbed_app_led_ctl(1,WISUN_SMART_METER_LED_BIT_G);
    }
    else if(state == STATE_POSTPONE)
    {
        tr_warn("[SMeter][%s-%d] State Update to STATE_POSTPONE\n", __FUNCTION__,__LINE__);
        _state = STATE_POSTPONE;
        _res_led_enable->set_value(true);
        _res_led_color->set_value(WISUN_SMART_METER_LED_BIT_B);
        mbed_app_led_ctl(1,WISUN_SMART_METER_LED_BIT_B);
    }
    else if(state == STATE_STOPED)
    {
        tr_warn("[SMeter][%s-%d] State Update to STATE_STOPED\n", __FUNCTION__,__LINE__);
        _res_led_enable->set_value(false);
        _state = STATE_STOPED;
    }
    else
    {
        tr_err("[SMeter][%s-%d] Unknow STate! \n", __FUNCTION__,__LINE__);
    }
}

void wisun_smart_meter_init_display()
{
    mbed_app_lcd_init();
    mbed_app_led_init();
}

// use references to encourage caller to pass this existing object
void wisun_smart_meter_init_client(SimpleM2MClient &client)
{
    tr_warn("[SMeter][%s-%d]  \n", __FUNCTION__,__LINE__);
    // Do not start if client has not been assigned.
    if (_client) return;

    _client = &client;
    _pwr = 0;
    _cur = mbed_app_get_current();
    _vol = mbed_app_get_voltage();
    _tmp = mbed_app_get_temperature();

    mbed_app_lcd_clear();
    mbed_app_lcd_fresh(_tmp,_vol,_cur,_pwr);

    //_cur_sfreq = WISUN_SMART_METER_INTERVAL_CUR;
    //_vol_sfreq = WISUN_SMART_METER_INTERVAL_VOL;
    //_tmp_sfreq = WISUN_SMART_METER_INTERVAL_TMP;
    _pwr_sfreq = WISUN_SMART_METER_INTERVAL_PWR;

    _res_temp_value = _client->add_cloud_resource(3303, 0, 5700, "[SMeter] ", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_vol_value = _client->add_cloud_resource(3316, 0, 5700, "[SMeter] ", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_cur_value= _client->add_cloud_resource(3317, 0, 5700, "[SMeter] - Sensor Value", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_led_enable = _client->add_cloud_resource(3311, 0, 5850, "[SMeter] ", M2MResourceInstance::BOOLEAN,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_led_updated, (void*)callback_notification_status);
    _res_led_color = _client->add_cloud_resource(3311, 0, 5706, "[SMeter] ", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_led_updated, (void*)callback_notification_status);
    _res_alarm_enable = _client->add_cloud_resource(3338, 0, 5850, "[SMeter] ", M2MResourceInstance::BOOLEAN,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_almen_updated, (void*)callback_notification_status);
    _res_alarm_duration = _client->add_cloud_resource(3338, 0, 5521, "[SMeter] ", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_almdur_updated, (void*)callback_notification_status);

    _res_pwr_ainst = _client->add_cloud_resource(3305, 0, 5800, "[SMeter] ", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_pwr_acuml = _client->add_cloud_resource(3305, 0, 5805, "[SMeter] ", M2MResourceInstance::FLOAT,M2MBase::GET_ALLOWED, 0, true, NULL, (void*)callback_notification_status);
    _res_pwr_sfreq = _client->add_cloud_resource(3305, 0, 6040, "[SMeter] - Sampling Freqency", M2MResourceInstance::INTEGER,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_pwrfreq_updated, (void*)callback_notification_status);
    _res_pwr_acuml_reset = _client->add_cloud_resource(3305, 0, 5822, "[SMeter] ", M2MResourceInstance::BOOLEAN,M2MBase::POST_ALLOWED, 0, true, (void*)callback_pwrreset_updated, (void*)callback_notification_status);

    _res_pwr_control = _client->add_cloud_resource(3312, 0, 5850, "[SMeter] - Power Control", M2MResourceInstance::BOOLEAN,M2MBase::GET_PUT_ALLOWED, 0, true, (void*)callback_pwrctl_updated, (void*)callback_notification_status);

#ifdef WISUN_SMART_METER_NM
    _res_nm_iid = _client->add_cloud_resource(26241, 0, 9, "[SMeter] - IID", M2MResourceInstance::OPAQUE,M2MBase::GET_ALLOWED, NULL, true, NULL, (void*)callback_notification_status);
#endif

    // Set the sample freqency to 1 second as default.
    _res_temp_value->set_value(_tmp);
    _res_vol_value->set_value(_cur);
    _res_cur_value->set_value(_vol);

    _res_pwr_sfreq->set_value(_pwr_sfreq);
    _res_pwr_ainst->set_value_float(_cur*_vol);
    _res_pwr_acuml->set_value_float(_pwr);

    _res_pwr_acuml_reset->set_value_float(0);

    _res_alarm_enable->set_value(0);
    _res_alarm_duration->set_value(WISUN_SMART_METER_ALARM_DURATION);
    _res_pwr_control->set_value(true);

    _update_iid();
    _update_state(STATE_CONNECT);
    _thread_smeter.start(_update_sensor);
}

void wisun_smart_meter_start()
{
    tr_warn("[SMeter][%s-%d]  \n", __FUNCTION__,__LINE__);
    _update_state(STATE_STARTED);

    _event_queue_msg = mbed_event_queue();
    _event_queue_alarm = mbed_highprio_event_queue();
 
    _timer_sfreq.attach(_event_queue_msg->event(&_update_resource), _pwr_sfreq);
}

void wisun_smart_meter_stop()
{
    _update_state(STATE_STOPED); 
    _timer_sfreq.detach();
}

#endif