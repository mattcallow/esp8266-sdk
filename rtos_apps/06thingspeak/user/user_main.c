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
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/err.h"

#include "extralib.h"
#include "user_config.h"

static struct ip_info ipconfig;
static struct station_config st_config;

#define TS_KEY_LEN 16
#define OW_ADDR_LEN 8
#define MAX_FIELDS 8
typedef struct {
	char thingspeak_key[TS_KEY_LEN+1];
	char talkback_key[TS_KEY_LEN+1];
	uint8_t fields[MAX_FIELDS][OW_ADDR_LEN];
	uint32_t valid;
} user_config_t;

static user_config_t user_config;
#define BUFSIZE 256
#define DBG printf

// see eagle_soc.h for these definitions
#define LED_GPIO 2
#define LED_GPIO_MUX PERIPHS_IO_MUX_GPIO2_U
#define LED_GPIO_FUNC FUNC_GPIO2

// This was defined in the old SDK.
#ifndef GPIO_OUTPUT_SET
#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
    gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)
#endif

#define PRIV_PARAM_START_SEC            0x3C
#define CONFIG_VALID 0xDEADBEEF


static ICACHE_FLASH_ATTR int
save_user_config(user_config_t *config)
{
	config->valid = CONFIG_VALID;
    spi_flash_erase_sector(PRIV_PARAM_START_SEC);
    spi_flash_write((PRIV_PARAM_START_SEC) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)config, sizeof(user_config_t));
	return 0;
}


static ICACHE_FLASH_ATTR int
read_user_config(user_config_t *config)
{
	DBG ("size to read is %d\n\r", sizeof(user_config_t));
    spi_flash_read((PRIV_PARAM_START_SEC) * SPI_FLASH_SEC_SIZE,
                        (uint32 *)config, sizeof(user_config_t));
	DBG ("valid flag is 0x%x\r\n", config->valid);
	if (config->valid != CONFIG_VALID)
	{
		config->thingspeak_key[0] = '\0';
		config->talkback_key[0] = '\0';
		return -1;
	}
	return 0;
}


/*
 * field is 1 to 8
 */
static ICACHE_FLASH_ATTR void
map_field(uint8_t field)
{
	char buf[2*OW_ADDR_LEN+1];
	int i;
	printf("Enter address for field %d:", field);
	uart_gets(buf, 17);
	if (strlen(buf) == 0)
	{
		for (i=0;i<OW_ADDR_LEN;i++)
		{
			user_config.fields[field-1][i] = 0;
		}
	}
	else if (strlen(buf) != 2*OW_ADDR_LEN)
	{
		printf("Invalid address\r\n");
	}
	else
	{
		for (i=OW_ADDR_LEN-1;i>=0;i--)
		{
			user_config.fields[field-1][i] = strtol(&buf[i*2], NULL, 16);
			buf[i*2] = '\0';
		}
	}
}

static ICACHE_FLASH_ATTR int
configure(void)
{
	int ch;
	int ret = wifi_station_set_auto_connect(0);
	vTaskDelay( 10 );
	DBG("wifi_station_set_auto_connect returns %d station_get_auto_connect now %d\r\n", ret, wifi_station_get_auto_connect());
    ret = wifi_set_opmode(NULL_MODE);
	vTaskDelay( 10 );
	DBG("wifi_set_opmode returns %d op_mode now %d\r\n", ret, wifi_get_opmode());
	do
	{
		printf("\r\nConfiguration:\r\n");
		printf("s: Wifi SSID [%s]\r\n", st_config.ssid);
		printf("p: Wifi Password [%s]\r\n", st_config.password);
		printf("a: ThingSpeak API key [%s]\r\n", user_config.thingspeak_key);
		printf("t: Thingspeak talkback key [%s]\r\n", user_config.talkback_key);
		printf("l: List 1-wire devices\r\n");
		printf("1-8: Map thingspeak field to 1-wire address\r\n");
		printf("x: Exit configuration\r\n");
		ch = uart_getchar();
		switch (ch)
		{
		case 's':
			printf("Enter SSID: ");
			uart_gets(st_config.ssid, 32);
			break;
		case 'p':
			printf("Enter Password: ");
			uart_gets(st_config.password, 64);
			break;
		case 'a':
			printf("Enter thingspeak key: ");
			uart_gets(user_config.thingspeak_key, TS_KEY_LEN+1);
			break;
		case 't':
			printf("Enter talkback key: ");
			uart_gets(user_config.talkback_key, TS_KEY_LEN+1);
			break;
		case 'l':
			ow_reset();
			ow_reset_search();
			printf("Searching 1-wire bus\r\n");
			uint8_t addr[8];
			int count=0;
			while(ow_search(addr))
			{
				printf("%2d: %02x%02x%02x%02x%02x%02x%02x%02x %02x ", ++count, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7], ow_crc8(addr, 8));
				switch(addr[0])
				{
				case 0x10:
					printf("DS18S20  ");
					break;
				case 0x28:
					printf("DS18B20  ");
					break;
				default:
					printf("*UNKNOWN*");
					break;
				}
				int i;
				for (i=0;i<8;i++)
				{
					if (!memcmp(addr, user_config.fields[i], 8))
					{
						printf(" Mapped to thingspeak field %d", i+1);
						break;
					}
				}
				printf("\r\n");
			}
			printf("Done %d devices found\r\n", count);
			break;
		case 'x':
		case 'X':
			DBG("setting config [%s] [%s]\n", st_config.ssid, st_config.password);
			wifi_station_set_config(&st_config);
			save_user_config(&user_config);
			break;
		default:
			if (ch >'0' && ch < '9') map_field(ch-'0');
			else printf("Invalid choice\r\n");
		}
	} while (ch != 'x' && ch != 'X');
    wifi_set_opmode(STATION_MODE);
	wifi_station_set_auto_connect(1);
}

static ICACHE_FLASH_ATTR int
check_configure(int ms)
{
	printf("Hit 'c' to configure\r\n");
	int ch = uart_getchar_ms(ms);
	if (ch == 'c' || ch == 'C')
	{
		return configure();
	}
	return 0;
}

/*
 * this task will print some info about the system
 */
void send_data(void *pvParameters)
{
	const portTickType sendDelay = (1000 * 60) ; // 1 minute
	const portTickType connectDelay = 1000 ; // 1 second
	int ch;
	for(;;)
	{
		printf("System Info:\r\n");
		printf("Time=%u\r\n", system_get_time());
		printf("RTC time=%u\r\n", system_get_rtc_time());
		printf("Chip id=0x%x\r\n", system_get_chip_id());
		printf("Free heap size=%u\r\n", system_get_free_heap_size());
		uint8_t ow_status = ow_reset();
		DBG("ow_reset returns %d\n\r", ow_status);
		uint8_t status = wifi_station_get_connect_status();
		DBG("wifi_station_get_connect_status returns %d\r\n", status);
		if (status != 5)
		{
			check_configure(connectDelay);
			continue;
		}
		// OK, we are connected. Time to send some data
        int nbytes;
        int sock;
		int ret;
		uint32_t systime = system_get_time()/1000000;

        struct sockaddr_in local_ip;
        struct sockaddr_in remote_ip;

        sock = socket(PF_INET, SOCK_STREAM, 0);

        if (sock == -1) {
            close(sock);
            printf("Socket create failed\n");
			check_configure(sendDelay);
            continue;
        }
        bzero(&remote_ip, sizeof(struct sockaddr_in));
        remote_ip.sin_family = AF_INET;
		if (netconn_gethostbyname(SERVER_HOST, &(remote_ip.sin_addr.s_addr)) != ERR_OK)
		{
            close(sock);
            printf("DNS lookup for '%s' failed\n", SERVER_HOST);
			check_configure(sendDelay);
            continue;
		}
        //remote_ip.sin_addr.s_addr = inet_addr(SERVER_IP);
        remote_ip.sin_port = htons(SERVER_PORT);

        ret =connect(sock, (struct sockaddr *)(&remote_ip), sizeof(struct sockaddr));
		if (ret != 0)
		{
            printf("connect fail %d %d\n", ret, errno);
            close(sock);
			check_configure(sendDelay);
            continue;
        }
        char *data = (char *)zalloc(BUFSIZE);
		int16_t temp[MAX_FIELDS];
		int i;
		for (i=0;i<8;i++)
		{	
			if (ow_crc8(user_config.fields[i],8) == 0)
			{
				DBG("Starting conversion for field %d\r\n", i+1);
				ow_ds18x20_convert(user_config.fields[i], 0);
			}
		}
		vTaskDelay( 750 / portTICK_RATE_MS);
		for (i=0;i<8;i++)
		{	
			if (ow_crc8(user_config.fields[i],8) == 0)
			{
				DBG("Reading value for field %d\r\n", i+1);
				if (ret=ow_ds18x20_read_temperature(user_config.fields[i], &temp[i]))
				{
					printf("ow_ds18x20_read_temperature() failed. Ret=%d\r\n", ret);
				}
			}
		}

#ifdef THINGSPEAK_TALKBACK_KEY
		//sprintf(data, "GET /update?api_key=%s&talkback_key=%s&field1=%lu&status=my ip:%d.%d.%d.%d\r\n\r\n", THINGSPEAK_API_KEY, THINGSPEAK_TALKBACK_KEY, systime, IP2STR(&ipconfig.ip));
#else
		//sprintf(data, "GET /update?key=%s&field1=%lu&status=my ip:%d.%d.%d.%d\r\n\r\n", THINGSPEAK_API_KEY, systime, IP2STR(&ipconfig.ip));
		char *p = data;
		ret = sprintf(p, "GET /update?key=%s&", user_config.thingspeak_key);
		p+=ret;
		for (i=0;i<8;i++)
		{
			if (ow_crc8(user_config.fields[i],8) == 0)
			{
				ret = sprintf(p, "field%d=%d.%d&", i+1, temp[i]/10, temp[i]%10);
				p+=ret;
			}
		}
		 //THINGSPEAK_API_KEY "&" THINGSPEAK_FIELD "=%d 
		ret = sprintf(p, " HTTP/1.1\r\nHost: %s\r\n\r\n", SERVER_HOST);
#endif
		DBG(data);

        ret=write(sock, data, strlen(data));
		if (ret <0)
		{
            printf("send failed. ret=%d errno=%d\n", ret, errno);
			close(sock);
			free(data);
			check_configure(sendDelay);
			continue;
        }

        bzero(data, BUFSIZE);
		do
		{
			nbytes = read(sock , data, (BUFSIZE-1));
			if (nbytes > 0)
			{
				data[nbytes] = 0;
				DBG("%d bytes read\n\r[%s]\n\r", nbytes, data);
			}
        } while (nbytes == (BUFSIZE-1));
        free(data);

        if (nbytes < 0) {
            printf("read data fail! ret=%d, errno=%d\n", nbytes, errno);
        }
		DBG("Closing socket %d\n", sock);
		close(sock);
		check_configure(sendDelay);
    }
}

static void ICACHE_FLASH_ATTR
null_print(char c)
{
}


/*
 * This is entry point for user code
 */
void ICACHE_FLASH_ATTR
user_init(void)
{
	bool ret;
	// unsure what the default bit rate is, so set to a known value
	uart_set_baud(UART0, BIT_RATE_9600);
	uart_rx_init();
	printf("Start\r\n");
	wifi_station_get_config(&st_config);
	if (!read_user_config(&user_config))
	{
		ret = wifi_set_opmode(STATION_MODE);
		DBG("wifi_set_opmode returns %d op_mode now %d\r\n", ret, wifi_get_opmode());
		wifi_station_set_auto_connect(1);
	}
	else
	{
		printf ("No valid config\r\n");
	}
	ow_init(0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
#ifdef THINGSPEAK_TALKBACK_KEY
	PIN_FUNC_SELECT(LED_GPIO_MUX, LED_GPIO_FUNC);
#endif
	xTaskCreate(send_data, "send", 256, NULL, 2, NULL);
}


// vim: ts=4 sw=4 noexpandtab
