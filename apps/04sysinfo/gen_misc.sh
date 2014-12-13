#!/bin/bash -x
make
if [ $? == 0 ];then
STARTDIR=$(pwd)
SDKDIR=${STARTDIR}/../../esp_iot_sdk
BINDIR=${SDKDIR}/bin
rm ${BINDIR}/eagle.app.v6.flash.bin ${BINDIR}/eagle.app.v6.irom0text.bin ${BINDIR}/eagle.app.v6.dump ${BINDIR}/eagle.app.v6.S

cd .output/eagle/debug/image

xt-objdump -x -s eagle.app.v6.out > ${BINDIR}/eagle.app.v6.dump
xt-objdump -S eagle.app.v6.out > ${BINDIR}/eagle.app.v6.S

xt-objcopy --only-section .text -O binary eagle.app.v6.out eagle.app.v6.text.bin
xt-objcopy --only-section .data -O binary eagle.app.v6.out eagle.app.v6.data.bin
xt-objcopy --only-section .rodata -O binary eagle.app.v6.out eagle.app.v6.rodata.bin
xt-objcopy --only-section .irom0.text -O binary eagle.app.v6.out eagle.app.v6.irom0text.bin

${SDKDIR}/tools/gen_appbin.py eagle.app.v6.out v6

cp eagle.app.v6.irom0text.bin ${BINDIR}/
cp eagle.app.v6.flash.bin ${BINDIR}/

cd ${STARTDIR}

else
echo "make error"
fi
