#!/bin/bash
# Enables/disables USB NCM networking based on UsbNcmEnabled param.
# Called by ncm-param-watcher on param changes.

GADGET=/sys/kernel/config/usb_gadget/g1

ensure_configfs() {
  while [ ! -d "$GADGET/configs/c.1" ]; do
    sleep .5
  done
}

enable_ncm() {
  ensure_configfs
  cd $GADGET

  # Unbind gadget
  echo > UDC 2>/dev/null || true

  # Set USB device name
  mkdir -p strings/0x409
  tr -d '\0' < /sys/firmware/devicetree/base/model > strings/0x409/product
  sed -n 's/.*androidboot.serialno=\([^ ]*\).*/\1/p' /proc/cmdline > strings/0x409/serialnumber

  # Add NCM function to gadget config
  ln -s functions/ncm.0 configs/c.1/f1 2>/dev/null || true
  echo "NCM" > configs/c.1/strings/0x409/configuration

  # Rebind gadget
  echo a600000.dwc3 > UDC

  # Wait for usb0 interface
  for i in $(seq 1 30); do
    ip link show usb0 > /dev/null 2>&1 && break
    sleep 0.1
  done
  ip addr add 192.168.42.2/24 dev usb0 2>/dev/null || true
  ip link set usb0 up

  sv up dnsmasq
}

disable_ncm() {
  ensure_configfs
  cd $GADGET

  sv down dnsmasq

  # Unbind gadget
  echo > UDC 2>/dev/null || true

  # Remove NCM function
  rm -f configs/c.1/f1 2>/dev/null

  # Rebind gadget without NCM
  echo a600000.dwc3 > UDC 2>/dev/null || true
}

NCM_PARAM="/data/params/d/UsbNcmEnabled"

if [ -f "$NCM_PARAM" ] && [ "$(< $NCM_PARAM)" == "1" ]; then
  echo "Enabling USB NCM"
  enable_ncm
else
  echo "Disabling USB NCM"
  disable_ncm
fi
