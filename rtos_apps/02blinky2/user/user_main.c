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

// see eagle_soc.h for these definitions
#define LED1_GPIO 2
#define LED1_GPIO_MUX PERIPHS_IO_MUX_GPIO2_U
#define LED1_GPIO_FUNC FUNC_GPIO2

#define LED2_GPIO 0
#define LED2_GPIO_MUX PERIPHS_IO_MUX_GPIO0_U
#define LED2_GPIO_FUNC FUNC_GPIO0

// This was defined in the old SDK.
#ifndef GPIO_OUTPUT_SET
#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
    gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)
#endif

/*
 * this task will blink an LED
 */
void blinky1(void *pvParameters)
{
	const portTickType xDelay = 500 / portTICK_RATE_MS;
	static uint8_t state=0;
	PIN_FUNC_SELECT(LED1_GPIO_MUX, LED1_GPIO_FUNC);
	for(;;)
	{
		GPIO_OUTPUT_SET(LED1_GPIO, state);
		state ^=1;
		vTaskDelay( xDelay);
	}
}

/*
 * this task will blink an LED
 */
void blinky2(void *pvParameters)
{
	const portTickType xDelay = 250 / portTICK_RATE_MS;
	static uint8_t state=0;
	PIN_FUNC_SELECT(LED2_GPIO_MUX, LED2_GPIO_FUNC);
	for(;;)
	{
		GPIO_OUTPUT_SET(LED2_GPIO, state);
		state ^=1;
		vTaskDelay( xDelay);
	}
}

/*
 * This is entry point for user code
 */
void ICACHE_FLASH_ATTR
user_init(void)
{
	xTaskCreate(blinky1, "bl1", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(blinky2, "bl2", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}


// vim: ts=4 sw=4 noexpandtab
