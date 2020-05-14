#!/usr/bin/python
import sys
import subprocess
import usb
import usb.core
import usb.util as util
import pyudev
import copy
context = pyudev.Context()
monitor = pyudev.Monitor.from_netlink(context)
monitor.filter_by(subsystem='usb')
old_busid = []
old_portid = []
scanDone = False
def findAll():
    if (scanDone == True):
        return
    # find USB devices
    dev = usb.core.find(find_all=True)
    busid = []
    portid = []
    # loop through devices, printing vendor and product ids in decimal and hex
    for cfg in dev:
        flag = 0
        #VendorID=0x46d & ProductID=0x82d bDeviceClass=0xef
        #print('VendorID=' + hex(cfg.idVendor) + ' & ProductID=' + hex(cfg.idProduct) + ' portid=' + hex(cfg.port_number) + ' busid=' + hex(cfg.bus) +'\n')
        if (cfg.idVendor == 0x46d):
            if (cfg.idProduct == 0x82d or cfg.idProduct == 0x85c):
                flag = 1
                if (flag == 1):
                    #print(' busid=' + hex(cfg.bus) + ' portid=' + hex(cfg.port_number) + '\n')
                    busid.append(cfg.bus)
                    portid.append(cfg.port_number)
                    flag = 0
        #ms = [(vid[i], pid[i]) for i in range(0, len(vid))]
    f= open("usb_vm.txt","w+")
    for j in range (0, len(busid)):
        f.write(str(busid[j]) + " " +  str(portid[j]) + "\n")
        #print (str(hex(vid[j])) + " " +  str(hex(pid[j])))
        old_busid = copy.deepcopy(busid)
        old_portid = copy.deepcopy(portid)
def findUpdates(action, dev):
    global old_busid, old_portid
    if action != 'add' and action != 'delete':
        return
    # find USB devices
    dev = usb.core.find(find_all=True)
    vid = []
    pid = []
    # loop through devices, printing vendor and product ids in decimal and hex
    for cfg in dev:
        flag = 0
        #VendorID=0x46d & ProductID=0x82d bDeviceClass=0xef
        #print('VendorID=' + hex(cfg.idVendor) + ' & ProductID=' + hex(cfg.idProduct) + ' portid=' + hex(cfg.port_number) + ' busid=' + hex(cfg.bus) +'\n')
        if (cfg.idVendor == 0x46d):
            if (cfg.idProduct == 0x82d or cfg.idProduct == 0x85c):
                flag = 1
                if (flag == 1):
                   # print(' busid=' + hex(cfg.bus) + ' portid=' + hex(cfg.port_number) + '\n')
                    vid.append(cfg.bus)
                    pid.append(cfg.port_number)
                    flag = 0

    if old_busid != busid and old_portid != portid:
        f= open("usb_vm.txt","w+")
        for j in range (0, len(busid)):
            f.write(str(hex(busid[j])) + " " +  str(hex(portid[j])) + "\n")
            #print (str(hex(vid[j])) + " " +  str(hex(pid[j])))
            old_busid = copy.deepcopy(busid)
            old_portid = copy.deepcopy(portid)
if __name__ == "__main__":
    observer = pyudev.MonitorObserver(monitor, findUpdates)
    observer.start()
    #while True:
    findAll()
    scanDone = True
    pass
