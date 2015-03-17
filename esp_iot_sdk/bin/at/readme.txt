There are two ways to download AT FW

(1)Old

download:
boot_v1.1.bin      0x00000
user1.512.old.bin  0x01000
blank.bin          0x3e000 & 0x7e000

(2)New ( Espressif recommended )

download:
boot_v1.2.bin      0x00000
user1.512.new.bin  0x01000
blank.bin          0x3e000 & 0x7e000


Update steps
1.Make sure TE(terminal equipment) is in sta or sta+ap mode
ex. AT+CWMODE=3
    OK
    
    AT+RST

2.Make sure TE got ip address
ex. AT+CWJAP="ssid","12345678"
    OK

    AT+CIFSR
    192.168.1.134

3.Let's update
ex. AT+CIUPDATE
    +CIPUPDATE:1    found server
    +CIPUPDATE:2    connect server
    +CIPUPDATE:3    got edition
    +CIPUPDATE:4    start start

    OK

note. If there are mistakes in the updating, then break update and print ERROR.