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
#include <espconn.h>
#include "driver/uart.h"


#define DELAY 15000 /* milliseconds */

#define DBG os_printf

LOCAL os_timer_t poll_timer;
LOCAL os_timer_t nw_close_timer;
LOCAL void ICACHE_FLASH_ATTR connect_to_network(void);


static struct espconn nwconn;
static esp_tcp conntcp;
static struct ip_info ipconfig;

LOCAL void ICACHE_FLASH_ATTR
nw_close_cb(void * arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;
    DBG("nw_close_cb\n");
	espconn_disconnect(p_nwconn);
}
/*
 * Data sent callback. 
 * Nothing to do...
 */
LOCAL void ICACHE_FLASH_ATTR
nw_sent_cb(void *arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_sent_cb\n");
}

/*
 * Data recvieved callback. 
 */
LOCAL void ICACHE_FLASH_ATTR
nw_recv_cb(void *arg, char *data, unsigned short len)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_recv_cb len=%u\n", len);
#ifdef THINGSPEAK_TALKBACK_KEY
	switch (len)
	{
	case 3:
		if (*data == 'O' && *(data+1) == 'F' && *(data+2) == 'F')
		{
			DBG("Switch Off");
			GPIO_OUTPUT_SET(LED_GPIO, LED_OFF);
		}
		break;
	case 2:
		if (*data == 'O' && *(data+1) == 'N')
		{
			DBG("Switch On");
			GPIO_OUTPUT_SET(LED_GPIO, LED_ON);
		}
		break;
	default:
		break;
	}
#else
	char *p=data;
	int i;
	for (i=0;i<len;i++)
	{
		if ((i % 16) == 0)
		{
			os_printf("\n0x");
		}
		os_printf("%02x(", *p);
		if (*p >=32 && *p <=127)
		{
			os_printf("%c",*p);
		}
		else
		{
			os_printf(".");
		}
		os_printf(") ");
	}
	os_printf("\n");
#endif
	// Start a timer to close the connection
    os_timer_disarm(&nw_close_timer);
	os_timer_setfn(&nw_close_timer, nw_close_cb, arg);
	os_timer_arm(&nw_close_timer, 100, 0);
}

/*
 * Connect callback. 
 * At this point, the connection to the remote server is established
 * Time to send some data
 */
LOCAL void ICACHE_FLASH_ATTR
nw_connect_cb(void *arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;
	static char data[128];

	uint32_t systime = system_get_time()/1000000;
    DBG("nw_connect_cb\n");

    espconn_regist_recvcb(p_nwconn, nw_recv_cb);
    espconn_regist_sentcb(p_nwconn, nw_sent_cb);
	// construct a basic GET request and send it
#ifdef THINGSPEAK_TALKBACK_KEY
	os_sprintf(data, "GET /update?api_key=%s&talkback_key=%s&field1=%lu&status=my ip:%d.%d.%d.%d\r\n\r\n", THINGSPEAK_API_KEY, THINGSPEAK_TALKBACK_KEY, systime, IP2STR(&ipconfig.ip));
#else
	os_sprintf(data, "GET /update?key=%s&field1=%lu&status=my ip:%d.%d.%d.%d\r\n\r\n", THINGSPEAK_API_KEY, systime, IP2STR(&ipconfig.ip));
#endif
	DBG(data);
	DBG("system_get_time() returns %lu\n", systime);
	espconn_sent(p_nwconn, data, os_strlen(data));
}

/*
 * Re-Connect callback. 
 * Do nothing?
 */
LOCAL void ICACHE_FLASH_ATTR
nw_reconnect_cb(void *arg, int8_t errno)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_reconnect_cb errno=%d\n", errno);
}

/*
 * Dis-Connect callback. 
 * Do nothing?
 */
LOCAL void ICACHE_FLASH_ATTR
nw_disconnect_cb(void *arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_disconnect_cb\n");
}


LOCAL void ICACHE_FLASH_ATTR
poll_cb(void * arg)
{
	uint8_t status = wifi_station_get_connect_status();
	os_printf("wifi_station_get_connect_status returns %d\n", status);
	if (status != STATION_GOT_IP)
	{
		connect_to_network();
	}
	status = wifi_station_get_connect_status();
	os_printf("wifi_station_get_connect_status now %d\n", status);
	if (status != STATION_GOT_IP)
	{
		os_printf("Failed to connect to network! Will retry later\n");
		return;
	}
    wifi_get_ip_info(STATION_IF, &ipconfig);
	// start the connection process
	// First set up the espconn structure
	const char thingspeak_ip[4] = {184, 106, 153, 149};
	nwconn.type = ESPCONN_TCP;
	nwconn.proto.tcp = &conntcp;
	os_memcpy(conntcp.remote_ip, thingspeak_ip, 4);
	conntcp.remote_port = 80;
	conntcp.local_port = espconn_port();
	espconn_regist_connectcb(&nwconn, nw_connect_cb);
	espconn_regist_disconcb(&nwconn, nw_disconnect_cb);
	espconn_regist_reconcb(&nwconn, nw_reconnect_cb);
	espconn_connect(&nwconn);
}

LOCAL void ICACHE_FLASH_ATTR
connect_to_network(void)
{
	static struct station_config config;
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
#ifdef THINGSPEAK_TALKBACK_KEY
	// Configure pin as a GPIO
	PIN_FUNC_SELECT(LED_GPIO_MUX, LED_GPIO_FUNC);
#endif

	/*
	 Don't try to connect the network here. It won't work!
	 Needs to be done in a callback
	*/
	// Set up a timer to send the message
    os_timer_disarm(&poll_timer);
	os_timer_setfn(&poll_timer, poll_cb, (void *)0);
	os_timer_arm(&poll_timer, DELAY, 1); 
}

// vim: ts=4 sw=4 
