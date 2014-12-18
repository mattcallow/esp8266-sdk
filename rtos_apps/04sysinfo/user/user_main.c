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
	The obligatory blinky demo using FreeRTOS
	Blink LEDs on GPIO pins 2 and 0
*/
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * this task will print some info about the system
 */
void sysinfo(void *pvParameters)
{
	const portTickType xDelay = 5000 / portTICK_RATE_MS;
	for(;;)
	{
		printf("System Info:\r\n");
		printf("Time=%u\r\n", system_get_time());
		printf("RTC time=%u\r\n", system_get_rtc_time());
		printf("Chip id=0x%x\r\n", system_get_chip_id());
		printf("Free heap size=%u\r\n", system_get_free_heap_size());
		printf("Mem info:\r\n");
		system_print_meminfo();
		printf("\r\n");
		vTaskDelay( xDelay);
	}
}

/*
 * This is entry point for user code
 */
void ICACHE_FLASH_ATTR
user_init(void)
{
	// unsure what the default bit rate is, so set to a knonw value
	uart_div_modify(UART0, UART_CLK_FREQ / (BIT_RATE_9600));
	// need to provide  a bigger stack size for this task - printf uses lots of stack
	xTaskCreate(sysinfo, "sys", 256, NULL, 2, NULL);
}


// vim: ts=4 sw=4 noexpandtab
