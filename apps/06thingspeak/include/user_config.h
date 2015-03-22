#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#error Change WIFI_SSID and WIFI_PASSWORD to suit your network (and delete this line)
#define WIFI_SSID "CHANGEME"
#define WIFI_PASSWORD "CHANGEME"

#define THINGSPEAK_API_KEY "CHANGEME"
#define THINGSPEAK_TALKBACK_KEY "CHANGEME"

#define LED_GPIO 2
#define LED_GPIO_MUX PERIPHS_IO_MUX_GPIO2_U
#define LED_GPIO_FUNC FUNC_GPIO2
// GPIO is connected to low side of the LED, so 0=On
#define LED_ON  0
#define LED_OFF 1
#endif
