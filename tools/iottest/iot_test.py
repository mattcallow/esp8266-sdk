#!/usr/bin/env python

# Test application for ESP8266 IoT sample

import urllib2
import httplib
import sys, time
import traceback
from socket import *
from Tkinter import  *

class WifiSwitch:

    def __init__(self):
        self.set_ip("192.168.4.1")
    
    def get_wifi_config(self):
        try:
            url = "http://" + self.ip + "/config?command=wifi"
            tConfig.delete(1.0, END)
            self.status ("Sending request to %s" % url)    
            f = urllib2.urlopen(url, None ,5)
            tConfig.insert(END, f.read())
            self.status ("Data sent")
        except:
            self.status(traceback.format_exc(), True)
        
    def set_wifi_config(self):
        try:
            url = "http://" + self.ip + "/config?command=wifi"
            data = tConfig.get(1.0, END)
            self.status ("Sending %s to %s" % (data, url))    
            f = urllib2.urlopen(url, data ,5)
            self.status ( f.read())
            self.status ("Data sent")
            self.get_wifi_config()
        except:
            self.status(traceback.format_exc(), True)

    def switch(self, state):
        try:
            url = "http://" + self.ip + "/config?command=switch"
            data="""{
        "status":%d
        }""" % state

            self.status ("Sending %s to %s" % (data, url))    
            f = urllib2.urlopen(url, data,5)
            self.status(f.read())
            self.status ("Data sent")
        except:
            self.status(traceback.format_exc(), True)

    def on(self):
        self.switch(1)

    def off(self):
        self.switch(0)

    def discover(self):
        MYPORT = 1025

        lDevices.delete(0, END)
        lDevices.update()
        s = socket(AF_INET, SOCK_DGRAM)
        s.bind(('', 0))
        s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)

        data = "Are You Espressif IOT Smart Device?"
        start = time.time()
        self.status("Discovery started at %f" % start)
        s.sendto(data, ('<broadcast>', MYPORT))

        TIMEOUT=2
        t = time.time()
        s.settimeout(1)
        while (time.time()-t < TIMEOUT):
            try:
                resp = s.recv(80)
                end = time.time()
                self.status ("response in %f" % (end - start))
                lDevices.insert(END, resp)
                lDevices.update()
            except timeout:
                pass
        self.status ("Done")

    def set_ip(self, address):
        self.ip = address

    def status(self, s, error=False):
        if (error):
            tStatus.insert(END, "ERROR:" + s + "\r\n", "error")
        else:
            tStatus.insert(END, s + "\r\n")
        tStatus.update()

switch = WifiSwitch()

# Create the UI
NUM_COLS=8
row=0
top = Tk()

title = Label(top, text="ESP8266 IoT test app")
title.grid(row=row, column=0,columnspan=NUM_COLS)
row=row+1

bDiscover=Button(top, text="Discover Devices", command=switch.discover).grid(row=row, column=0, sticky=NSEW)
fDevices=Frame(top)
sDevices = Scrollbar(fDevices)
sDevices.pack(side=RIGHT, fill=Y)
lDevices = Listbox(fDevices, height=4, width=40, yscrollcommand=sDevices.set)
sDevices.config(command=lDevices.yview)
lDevices.pack(fill=BOTH)
fDevices.grid(row=row, column=1, columnspan=7, sticky=EW)
row=row+1

Button(top, text="Set IP", command=lambda: switch.set_ip(eIp.get())).grid(row=row, column=0, sticky=NSEW)
eIp = Entry(top)
eIp.insert(0, switch.ip)
eIp.grid(row=row, column=1, columnspan=7, sticky=EW)
row=row+1

fConfig=Frame(top)
sConfig=Scrollbar(fConfig)
tConfig = Text(top, height=10)
tConfig.grid(row=row, column=1, columnspan=7, rowspan=2, sticky=EW)
Button(top, text="Read wifi config", command=switch.get_wifi_config).grid(row=row, column=0, sticky=NSEW)
Button(top, text="Set wifi config", command=switch.set_wifi_config).grid(row=row+1, column=0, sticky=NSEW)
row=row+2

bOn = Button(top, text="Switch On", command=switch.on)
bOn.grid(row=row, column=0, columnspan=4, sticky=EW)
bOff= Button(top, text="Switch Off", command=switch.off)
bOff.grid(row=row, column=4, columnspan=4, sticky=EW)
row=row+1

fStatus=Frame(top)
sStatus = Scrollbar(fStatus)
sStatus.pack(side=RIGHT, fill=Y)
fStatus.grid(row=row, column=1, columnspan=7)
tStatus=Text(fStatus, height=4, bg="grey", yscrollcommand=sStatus.set)
sStatus.config(command=tStatus.yview)
tStatus.pack()
bClear = Button(top, text="Clear", command=lambda: tStatus.delete(1.0, END)).grid(row=row, column=0, sticky=NSEW)
tStatus.tag_configure('error', foreground='red')
row=row+1

top.mainloop()

# vim: ts=4 sw=4 expandtab
