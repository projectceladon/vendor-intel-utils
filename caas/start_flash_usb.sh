#!/bin/bash

qemu-system-x86_64 \
  -m 2048 -smp 2 -M q35 \
  -name caas-vm \
  -enable-kvm \
  -k en-us \
  -vga qxl \
  -machine kernel_irqchip=on \
  -chardev socket,id=charserial0,path=./kernel-console,server,nowait \
  -device isa-serial,chardev=charserial0,id=serial0 \
  -device qemu-xhci,id=xhci,addr=0x5 \
  -device usb-host,bus=xhci.0,vendorid=<your-usb-vendorid>,productid=<your-usb-productid> \
  -device virtio-scsi-pci,id=scsi0,addr=0x8 \
  -drive file=./android.qcow2,if=none,format=qcow2,id=scsidisk1 \
  -device scsi-hd,drive=scsidisk1,bus=scsi0.0 \
  -bios ./OVMF.fd \

