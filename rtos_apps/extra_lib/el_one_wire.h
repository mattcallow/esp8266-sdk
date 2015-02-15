#ifndef _EL_ONE_WIRE_H_
#define _EL_ONE_WIRE_H_
#include "esp_common.h"
#include "freertos/FreeRTOS.h"

#define ONEWIRE_SEARCH
#define ONEWIRE_DS18X20

// This was defined in the old SDK.
#ifndef GPIO_OUTPUT_SET
#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
    gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)
#endif

#ifndef GPIO_PIN_ADDR
#define GPIO_PIN_ADDR(i) (GPIO_PIN0_ADDRESS + i*4)
#endif

#ifndef GPIO_DIS_OUTPUT
#define GPIO_DIS_OUTPUT(gpio_no)        gpio_output_set(0,0,0, 1<<gpio_no)
#endif

#ifndef GPIO_INPUT_GET
#define GPIO_INPUT_GET(gpio_no)     ((gpio_input_get()>>gpio_no)&BIT0)
#endif


void ow_init(uint8_t pin, uint32_t mux, uint8_t func);
#ifdef ONEWIRE_SEARCH
uint8_t ow_search(uint8_t *newaddr);
void ow_reset_search();
#endif
uint8_t ow_reset(void);
void ow_select(const uint8_t rom[8]);
void ow_skip(void);
void ow_write(uint8_t v, uint8_t power);
void ow_write_bytes(const uint8_t *buf, uint16_t count, uint8_t power);
uint8_t ow_read(void);
void ow_read_bytes(uint8_t *buf, uint16_t count);
void ow_depower(void);
uint8_t ow_crc8(const uint8_t *addr, uint8_t len);
#ifdef ONEWIRE_DS18X20
void ow_ds18x20_convert(const uint8_t *addr, uint8_t power);
uint8_t ow_ds18x20_read_temperature(const uint8_t *addr, int16_t *temp);
#endif

#endif

// vim: ts=4 sw=4 noexpandtab
