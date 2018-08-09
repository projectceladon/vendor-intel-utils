#!/vendor/bin/sh

KERNEL_VERSION=$(uname -r)

if [ $(which cansend) ]; then
  CANSEND=$(which cansend)
elif [ -e /sbin/cansend ]; then
  CANSEND=/sbin/cansend
elif [ -e /vendor/bin/cansend ]; then
  CANSEND=/vendor/bin/cansend
else
  echo "Cansend not found!"
fi

echo $1
if [[ $1 = "h" ]]; then
  $CANSEND slcan0 "0000FFFF#0C010155555555"
  case "$KERNEL_VERSION" in
    4.4*) echo h > /sys/bus/platform/devices/intel-cht-otg.0/mux_state ;;
    4.9*) echo h > /sys/bus/platform/drivers/intel_usb_dr_phy/intel_usb_dr_phy.0/mux_state ;;
       *) echo "host" > /sys/class/usb_role/intel_xhci_usb_sw-role-switch/role ;;
  esac
elif [[ $1 = "p" ]]; then
  $CANSEND slcan0 "0000FFFF#0C010055555555"
  case "$KERNEL_VERSION" in
    4.4*) echo p > /sys/bus/platform/devices/intel-cht-otg.0/mux_state ;;
    4.9*) echo p > /sys/bus/platform/drivers/intel_usb_dr_phy/intel_usb_dr_phy.0/mux_state ;;
       *) echo "device" > /sys/class/usb_role/intel_xhci_usb_sw-role-switch/role ;;
  esac
else
  echo "Please input h to swith to USB OTG mode"
  echo "usb_otg_switch.sh h"
  echo "Please input p to swith USB device mode"
  echo "usb_otg_switch.sh p"
fi

