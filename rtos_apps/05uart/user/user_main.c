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
	Testing the uart receive functionallity
	Uses a interrupt routine and FreeRTOS Queue to handle rxed chars
*/
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "uart.h"

#define DBG printf
/*
 * this task will read the uart and echo back all characters entered
 */
void 
read_task(void *pvParameters)
{
	for(;;)
	{
		char c = uart_getchar();
		if (c == '\r')
		{
			os_putc('\n');
		}
		os_putc(c);
	}
}

/*
 * Print some status info
 */
void 
status_task(void *pvParameters)
{
	for(;;)
	{
		printf("uart_rx_bytes=%d, uart_rx_overruns=%d\r\n", uart_rx_bytes, uart_rx_overruns);
		vTaskDelay(10000 / portTICK_RATE_MS);
	}
}
/*
 * This is entry point for user code
 */
void ICACHE_FLASH_ATTR
user_init(void)
{
	portBASE_TYPE ret;
	// unsure what the default bit rate is, so set to a known value
	uart_div_modify(UART0, UART_CLK_FREQ / (BIT_RATE_9600));
	printf("Start\r\n");
	wifi_set_opmode(NULL_MODE);
	uart_rx_init();
	DBG("About to create task\r\n");
	xTaskHandle t;
	ret = xTaskCreate(read_task, "rx", 256, NULL, 2, &t);
	DBG("xTaskCreate returns %d handle is %d\n", ret, t);
	ret = xTaskCreate(status_task, "st", 256, NULL, 3, &t);
	DBG("xTaskCreate returns %d handle is %d\n", ret, t);
}


// vim: ts=4 sw=4 noexpandtab
