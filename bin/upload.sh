#!/bin/sh
# Script to simplify loading the images into flash
#
PORT=""
FLASH_APP=0
FLASH_BLANK=0
FLASH_ROM=0
FLASH_INIT=0
PORTS="/dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyACM0 /dev/ttyACM1"

show_help()
{
	echo "Usage: $0 [-h|-?|-a|-b|-r|-i|-p port]"
	echo "\t-h|-?: Show this help"
	echo "\t-p port: Specify the serial port to use"
	echo "Default serial port is the first available from ${PORTS}"
	echo "\t-a: Flash application image"
	echo "\t-r: Flash ROM image"
	echo "\t-b: Flash blank image"
	echo "\t-i: Flash init image"
	echo "Default options are to flash the ROM and application image"
	echo "Remember to set GPIO0=LOW before starting"
}

while getopts "h?p:abri" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    p)  PORT=$OPTARG
        ;;
    a)  FLASH_APP=1
	;;
    b)  FLASH_BLANK=1
	;;
    r)  FLASH_ROM=1
	;;
    i)  FLASH_INIT=1
	;;
    esac
done

if [ "x${PORT}" = "x" ]; then
	for p in ${PORTS}
	do
		if [ -c ${p} ]; then
			PORT=${p}
			break;
		fi
	done
fi
if [ "x${PORT}" = "x" ]; then
	echo "Can't find a serial port to use! I tried ${PORTS}"
	echo "Please specify one with the -p option."
	show_help
	exit
fi

if [ ${FLASH_APP} -eq 0 -a ${FLASH_BLANK} -eq 0 -a ${FLASH_ROM} -eq 0 -a ${FLASH_INIT} -eq 0 ]; then
	FLASH_APP=1
	FLASH_ROM=1
fi
if [ ! -w ${PORT} ]; then
	echo "Can't use ${PORT} - please check that the file exists and is writable"
	show_help
	exit
fi
echo "using ${PORT}"

if [ $FLASH_ROM -eq 1 ]; then
	echo "Flashing ROM"
	../tools/esptool/esptool.py --port ${PORT} write_flash 0x40000 eagle.app.v6.irom0text.bin
fi
if [ $FLASH_APP -eq 1 ]; then
	echo "Flashing APP"
	../tools/esptool/esptool.py --port ${PORT} write_flash 0x0000 eagle.app.v6.flash.bin
fi

# flash this after SDK change
#../tools/esptool/esptool.py --port /dev/ttyUSB0 write_flash 0x7E000 blank.bin

# Init default data
#../tools/esptool/esptool.py --port /dev/ttyUSB0 write_flash 0x7c000 esp_init_data_default.bin


