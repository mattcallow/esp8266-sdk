# esp8266-sdk

Apps written using the esp8266 SDK from EspressIf
The SDK files are also included. These are (c) EspressIf, and available from their forum
http://bbs.espressif.com/

There are two SDKs; esp_iot_sdk which is the original one, and esl_iot_rtos_sdk which is the FreeRTOS one

Currently (March 2015) the FreeRTOS one doesn't seem to be updated by espressif.

There are are two application directories; apps - which uses the original SDK, and rtos_apps which uses the FreeRTOS SDK

## Building the apps

### original SDK

Just run the makefile in any of the app/ subdirectories
To flash the image, run 'make && make upload'
This will ensure that the images are built and then try to upload them via /dev/ttyUSB0
To specify a different port, use 'make upload PORT=/dev/ttyACM0' or whatever your tty is


