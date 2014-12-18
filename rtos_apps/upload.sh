#!/bin/sh

BINDIR=../esp_iot_rtos_sdk/bin
TOOLDIR=${BINDIR}/../tools
ESPTOOL=../esptool/esptool.py
TTY=/dev/ttyUSB0
${ESPTOOL} --port ${TTY} write_flash 0x00000 ${BINDIR}/eagle.app.v6.flash.bin 0x40000 ${BINDIR}/eagle.app.v6.irom0text.bin
#${ESPTOOL} --port ${TTY} write_flash 0x7E000 ${BINDIR}/blank.bin 0x00000 ${BINDIR}/eagle.app.v6.flash.bin 0x40000 ${BINDIR}/eagle.app.v6.irom0text.bin
#${ESPTOOL} --port ${TTY} write_flash 0x7e000 ${BINDIR}/blank.bin

