/*
The MIT License (MIT)

Copyright (c) 2014 Matt Callow

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/*
	Thingspeak demo
*/
#include <user_interface.h>
#include <osapi.h>
#include <c_types.h>
#include <mem.h>
#include <os_type.h>
#include <gpio.h>
#include "driver/uart.h"

#define DELAY 60000 /* milliseconds */

LOCAL os_timer_t poll_timer;

LOCAL void ICACHE_FLASH_ATTR
poll_cb(void)
{
	uint8_t status = wifi_station_get_connect_status();
	os_printf("wifi_station_get_connect_status returns %d\n", status);

}

LOCAL void ICACHE_FLASH_ATTR
connect_to_network(void)
{
	struct station_config config;
	bool ret;
	os_printf("Connecting to network\n");
    ETS_UART_INTR_DISABLE();
	ret = wifi_set_opmode(STATION_MODE);
	ETS_UART_INTR_ENABLE();
	os_printf("wifi_set_opmode returns %d, opmode now %d\n", ret, wifi_get_opmode());
	uint8_t status = wifi_station_get_connect_status();
	os_printf("wifi_station_get_connect_status returns %d\n", status);
	os_printf("disconnecting\n");
	ret = wifi_station_disconnect();
	os_printf("setting SSID to %s\n", WIFI_SSID);
	os_strcpy(config.ssid, WIFI_SSID);
	os_printf("setting password\n");
	os_strcpy(config.password, WIFI_PASSWORD);
	os_printf("setting config [%s] [%s]\n", config.ssid, config.password);
    ETS_UART_INTR_DISABLE();
	wifi_station_set_config(&config);
	ETS_UART_INTR_ENABLE();
	os_printf("connecting\n");
	ret = wifi_station_connect();
	os_printf("wifi_station_connect returns %d\n", ret);
	
}
/*
 * This is entry point for user code
 */
void user_init(void)
{
	// Configure the UART
	uart_init(BIT_RATE_9600,0);
	// enable system messages
	system_set_os_print(1);

	connect_to_network();
	// Set up a timer to send the message
	// os_timer_disarm(ETSTimer *ptimer)
    os_timer_disarm(&poll_timer);
	// os_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg)
	os_timer_setfn(&poll_timer, poll_cb, (void *)0);
	// void os_timer_arm(ETSTimer *ptimer,uint32_t milliseconds, bool repeat_flag)
	os_timer_arm(&poll_timer, DELAY, 1); 
}

// vim: ts=4 sw=4 
