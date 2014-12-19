#ifndef _UART_H_
#define _UART_H_

#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern xQueueHandle  uart_rx_queue;
extern volatile uint16_t uart_rx_overruns;
extern volatile uint16_t uart_rx_bytes;

int ICACHE_FLASH_ATTR uart_getchar_ms(int timeout);
#define uart_getchar() uart_getchar_ms(-1)
int ICACHE_FLASH_ATTR uart_rx_available(void);
void ICACHE_FLASH_ATTR uart_rx_init(void);
void ICACHE_FLASH_ATTR uart_set_baud(int uart, int baud);

#endif
// vim: ts=4 sw=4 noexpandtab
