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
	Gate control
	Run a command when a button is pressed
  
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

#define GET_LEN 128
#define HOST_LEN 80
typedef struct {
	char get_cmd[GET_LEN+1];
	char host[HOST_LEN+1];
	int16_t port;
	uint32_t valid;
} user_config_t;

static user_config_t user_config;
static uint8_t wifi_status=0;
#define BUFSIZE 200
#define DBG printf

// see eagle_soc.h for these definitions
#define LED_GPIO 2
#define LED_GPIO_MUX PERIPHS_IO_MUX_GPIO2_U
#define LED_GPIO_FUNC FUNC_GPIO2


#define BUTTON_GPIO 0
#define BUTTON_GPIO_MUX PERIPHS_IO_MUX_GPIO0_U
#define BUTTON_GPIO_FUNC FUNC_GPIO0

// This was defined in the old SDK.
#ifndef GPIO_OUTPUT_SET
#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
    gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)
#endif

#define LED_ON() GPIO_OUTPUT_SET(LED_GPIO, 1)
#define LED_OFF() GPIO_OUTPUT_SET(LED_GPIO, 0)

#define TASK_DELAY_MS(m) vTaskDelay(m/portTICK_RATE_MS)
#define PRIV_PARAM_START_SEC            0x3C
#define CONFIG_VALID 0xDEADBEEF


static ICACHE_FLASH_ATTR int
save_user_config(user_config_t *config)
{
	config->valid=CONFIG_VALID;
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
	DBG ("config->valid=0x%04x\r\n", config->valid);
	if (config->valid != CONFIG_VALID)
	{
		config->get_cmd[0] = '\0';
		config->host[0] = '\0';
		config->port = 80;
		return -1;
	}
	return 0;
}

static char data[BUFSIZE];
uint8_t send_command()
{
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
		return 1;
	}
	bzero(&remote_ip, sizeof(struct sockaddr_in));
	remote_ip.sin_family = AF_INET;
	if (netconn_gethostbyname(user_config.host, &(remote_ip.sin_addr.s_addr)) != ERR_OK)
	{
		close(sock);
		printf("DNS lookup for '%s' failed\n", user_config.host);
		return 2;
	}
	remote_ip.sin_port = htons(user_config.port);

	ret = connect(sock, (struct sockaddr *)(&remote_ip), sizeof(struct sockaddr));
	if (ret != 0)
	{
		printf("connect fail %d %d\n", ret, errno);
		close(sock);
		return 3;
	}
	char *p = data;
	strcpy(p, "GET ");
	p+=4;
	strcat(p, user_config.get_cmd);
	p+=strlen(user_config.get_cmd);
	ret = sprintf(p, " HTTP/1.1\r\nHost: %s\r\n\r\n", user_config.host);
	DBG(data);

	ret=write(sock, data, strlen(data));
	if (ret <0)
	{
		printf("send failed. ret=%d errno=%d\n", ret, errno);
		close(sock);
		return 4;
	}

	bzero(data, BUFSIZE);
	do
	{
		nbytes = read(sock , data, (BUFSIZE-1));
		if (nbytes > 0)
		{
			data[nbytes] = 0;
			DBG("%d bytes read [%s]\n\r", nbytes, data);
		}
	} while (nbytes == (BUFSIZE-1));

	if (nbytes < 0) {
		printf("read data fail! ret=%d, errno=%d\n", nbytes, errno);
	}
	DBG("Closing socket %d\n", sock);
	close(sock);
}

static ICACHE_FLASH_ATTR int
configure(void)
{
	int ch;
	int ret = wifi_station_set_auto_connect(0);
	TASK_DELAY_MS( 10 );
	DBG("wifi_station_set_auto_connect returns %d station_get_auto_connect now %d\r\n", ret, wifi_station_get_auto_connect());
    ret = wifi_set_opmode(NULL_MODE);
	TASK_DELAY_MS( 10 );
	DBG("wifi_set_opmode returns %d op_mode now %d\r\n", ret, wifi_get_opmode());
	do
	{
		printf("\r\nConfiguration:\r\n");
		printf("1: Wifi SSID [%s]\r\n", st_config.ssid);
		printf("2: Wifi Password [%s]\r\n", st_config.password);
		printf("3: HTTP host[%s]\r\n", user_config.host);
		printf("4: HTTP port[%d]\r\n", user_config.port);
		printf("5: HTTP path [%s]\r\n", user_config.get_cmd);
		printf("0: Exit configuration\r\n");
		ch = uart_getchar();
		switch (ch)
		{
		case '1':
			printf("Enter SSID: ");
			uart_gets(st_config.ssid, 32);
			break;
		case '2':
			printf("Enter Password: ");
			uart_gets(st_config.password, 64);
			break;
		case '3':
			printf("Enter host: ");
			uart_gets(user_config.host, HOST_LEN+1);
			break;
		case '4':
			printf("Enter port: ");
			char buf[6];
			uart_gets(buf, 6);
			user_config.port = atoi(buf);
			break;
		case '5':
			printf("Enter GET path: ");
			uart_gets(user_config.get_cmd, GET_LEN+1);
			break;
		case '0':
			DBG("setting config [%s] [%s]\n", st_config.ssid, st_config.password);
			wifi_station_set_config(&st_config);
			save_user_config(&user_config);
			break;
		default:
			printf("Invalid choice\r\n");
		}
	} while (ch != '0');
    ret=wifi_set_opmode(STATION_MODE);
	TASK_DELAY_MS( 10 );
	DBG("wifi_set_opmode returns %d op_mode now %d\r\n", ret, wifi_get_opmode());
	ret=wifi_station_set_auto_connect(1);
	TASK_DELAY_MS( 10 );
	DBG("wifi_station_set_auto_connect returns %d station_get_auto_connect now %d\r\n", ret, wifi_station_get_auto_connect());
	printf("System will now restart\r\n");
	system_restart();
}

#define READ_BUTTON() ((GPIO_REG_READ(GPIO_IN_ADDRESS)  >> (GPIO_ID_PIN(BUTTON_GPIO))) & 0x01)
static ICACHE_FLASH_ATTR
void check_button(void *pvParameters)
{
	for (;;) 
	{
		TASK_DELAY_MS(100);
		if (wifi_status != STATION_GOT_IP) continue;
		// check the button
		if (READ_BUTTON() == 0)
		{
			TASK_DELAY_MS(10); // de-bounce
			if (READ_BUTTON() == 0)
			{
				send_command();
				while(READ_BUTTON() ==0); // wait for button release
			}
		}
	}
}

static ICACHE_FLASH_ATTR void
check_connection(void *pvParameters)
{
	for(;;)
	{
		wifi_status = wifi_station_get_connect_status();
		//DBG("wifi_station_get_connect_status returns %d\r\n", wifi_status);
		if (wifi_status == STATION_GOT_IP)
		{
			LED_ON();
		}
		else
		{
			LED_OFF();
		}
		TASK_DELAY_MS(500);
	}
}

static ICACHE_FLASH_ATTR void
check_input(void *pvParameters)
{
	int ch;
	for(;;)
	{
		if (wifi_status == STATION_GOT_IP)
		{
			printf("Hit 'c' to configure, 't' to test\r\n");
		}
		else
		{
			printf("Hit 'c' to configure\r\n");
		}
		ch = uart_getchar(); // wait forever
		switch(ch)
		{
		case 'c':
		case 'C':
			configure();
			break;
		case 't':
		case 'T':
			if (wifi_status == STATION_GOT_IP) 
			{
					send_command();
			}
			else
			{
				printf("Not connected\r\n");
				TASK_DELAY_MS(500);
			}
			break;
		case -1: // This shouldn't happen?
			printf("Error reading serial data\r\n");
			break;
		default:
			printf("Invalid key %d %c\r\n", ch, ch);
			uart_rx_flush();
			TASK_DELAY_MS(500);
		}
    }
}


/*
 * This is entry point for user code
 */
void ICACHE_FLASH_ATTR
user_init(void)
{
	bool ret;
	PIN_FUNC_SELECT(LED_GPIO_MUX, LED_GPIO_FUNC);
	PIN_FUNC_SELECT(BUTTON_GPIO_MUX, BUTTON_GPIO_FUNC);
	// disable output drivers
    GPIO_REG_WRITE(GPIO_PIN_ADDR(BUTTON_GPIO), 0x04);
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 <<BUTTON_GPIO);

	LED_OFF();
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
	xTaskCreate(check_button, "button", 256, NULL, 3, NULL);
	xTaskCreate(check_input, "input", 256, NULL, 3, NULL);
	xTaskCreate(check_connection, "check", 256, NULL, 4, NULL);
}


// vim: ts=4 sw=4 noexpandtab
