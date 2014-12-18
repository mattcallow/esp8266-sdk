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
 * this task will print the message
 */
void helloworld(void *pvParameters)
{
	const portTickType xDelay = 1000 / portTICK_RATE_MS;
	for(;;)
	{
		printf("Hello World!\n");
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
	xTaskCreate(helloworld, "hw", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}


// vim: ts=4 sw=4 noexpandtab
