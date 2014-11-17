#!/bin/sh
# Load the user application and libraries into flash
echo "Remember to set GPIO0=LOW before starting"
../tools/esptool/esptool.py --port /dev/ttyUSB0 write_flash 0x0000 eagle.app.v6.flash.bin
../tools/esptool/esptool.py --port /dev/ttyUSB0 write_flash 0x40000 eagle.app.v6.irom0text.bin
