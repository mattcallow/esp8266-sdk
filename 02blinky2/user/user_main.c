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
	The blinky demo using an os timer
*/
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>

// see eagle_soc.h for these definitions
#define LED_GPIO 2
#define LED_GPIO_MUX PERIPHS_IO_MUX_GPIO2_U
#define LED_GPIO_FUNC FUNC_GPIO2

#define DELAY 500000 /* microseconds */

LOCAL os_timer_t blink_timer;

LOCAL uint8_t led_state=0;
LOCAL void ICACHE_FLASH_ATTR
blink_cb(void *arg)
{
	GPIO_OUTPUT_SET(LED_GPIO, led_state);
	led_state ^=1;
}
/*
 * This is entry point for user code
 */
void user_init(void)
{
	// Configure pin as a GPIO
	PIN_FUNC_SELECT(LED_GPIO_MUX, LED_GPIO_FUNC);

	// Set up a timer to blink the LED
	// os_timer_disarm(ETSTimer *ptimer)
    os_timer_disarm(&blink_timer);
	// os_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg)
	os_timer_setfn(&blink_timer, (os_timer_func_t *)blink_cb, (void *)0);
	// void os_timer_arm(ETSTimer *ptimer,uint32_t milliseconds, bool repeat_flag)
	os_timer_arm(&blink_timer, 500, 1);
}

// vim: ts=4 sw=4 
