#!/bin/bash
# Enables/disables USB NCM networking based on UsbNcmEnabled param.
# Called by ncm-param-watcher on param changes.

GADGET=/config/usb_gadget/g1
USB_IF="usb0"
USB_DNSMASQ_CONF="/run/dnsmasq-usb0.conf"
UDC_NAME="a600000.usb"
NCM_PARAM="/data/params/d/UsbNcmEnabled"
USB_SERIAL=""
USB_SUBNET=""
USB_DEVICE_ADDR=""
USB_HOST_ADDR=""

ensure_configfs() {
  if ! mountpoint -q /config; then
    mount -t configfs none /config
  fi
}

ensure_gadget_base() {
  ensure_configfs

  mkdir -p "$GADGET"
  cd "$GADGET" || exit 1

  mkdir -p strings/0x409
  mkdir -p configs/c.1/strings/0x409
  mkdir -p functions/ncm.0

  echo 0x1d6b > idVendor
  echo 0x0103 > idProduct
  echo 250 > configs/c.1/MaxPower

  local serial model
  serial="$(get_usb_serial)"
  model="$(tr -d '\0' < /sys/firmware/devicetree/base/model 2>/dev/null || true)"

  echo "$serial" > strings/0x409/serialnumber
  echo "comma.ai" > strings/0x409/manufacturer
  echo "$model ($serial)" > strings/0x409/product
  echo "NCM" > configs/c.1/strings/0x409/configuration
}

get_usb_serial() {
  if [ -n "$USB_SERIAL" ]; then
    echo "$USB_SERIAL"
    return 0
  fi

  USB_SERIAL="$(sed -n 's/.*androidboot.serialno=\([^ ]*\).*/\1/p' /proc/cmdline)"
  if [ -z "$USB_SERIAL" ]; then
    USB_SERIAL="$(hostname)"
  fi

  echo "$USB_SERIAL"
}

derive_usb_network() {
  if [ -n "$USB_SUBNET" ] && [ -n "$USB_DEVICE_ADDR" ] && [ -n "$USB_HOST_ADDR" ]; then
    return 0
  fi

  local serial hash subnet_id octet2 octet3
  serial="$(get_usb_serial)"
  hash="$(printf '%s' "$serial" | cksum | awk '{print $1}')"
  subnet_id=$((hash % 4096))
  octet2=$((16 + (subnet_id / 256)))
  octet3=$((subnet_id % 256))

  USB_SUBNET="172.${octet2}.${octet3}"
  USB_DEVICE_ADDR="${USB_SUBNET}.1/24"
  USB_HOST_ADDR="${USB_SUBNET}.2"
}

unbind_gadget() {
  cd "$GADGET" || return 1
  echo "" > UDC 2>/dev/null || true
}

bind_gadget() {
  cd "$GADGET" || return 1
  echo "$UDC_NAME" > UDC
}

wait_for_usb_if() {
  for i in $(seq 1 30); do
    ip link show "$USB_IF" >/dev/null 2>&1 && return 0
    sleep 0.1
  done
  return 1
}

reset_usb_if() {
  dhcpcd -x "$USB_IF" >/dev/null 2>&1 || true
  ip -4 addr flush dev "$USB_IF" 2>/dev/null || true
}

write_dnsmasq_conf() {
  derive_usb_network
  mkdir -p /run

  cat > "$USB_DNSMASQ_CONF" <<EOF
port=0
interface=${USB_IF}
bind-interfaces
dhcp-authoritative
dhcp-range=${USB_HOST_ADDR},${USB_HOST_ADDR},255.255.255.0,12h
# Don't advertise a default gateway or DNS server over USB.
dhcp-option=3
dhcp-option=6
EOF
}

configure_usb_if() {
  derive_usb_network
  ip link set "$USB_IF" up
  reset_usb_if
  ip addr add "$USB_DEVICE_ADDR" dev "$USB_IF"
  write_dnsmasq_conf
  sv down dnsmasq 2>/dev/null || true
  sv up dnsmasq
  echo "Configured ${USB_IF}: device=${USB_DEVICE_ADDR%/*} host=${USB_HOST_ADDR} subnet=${USB_SUBNET}.0/24"
}

enable_ncm() {
  ensure_gadget_base
  cd "$GADGET" || exit 1

  unbind_gadget

  ln -s functions/ncm.0 configs/c.1/f1 2>/dev/null || true
  echo "NCM" > configs/c.1/strings/0x409/configuration

  bind_gadget

  if wait_for_usb_if; then
    configure_usb_if
  else
    echo "WARNING: $USB_IF not present yet."
  fi

}

disable_ncm() {
  ensure_gadget_base
  cd "$GADGET" || exit 1

  sv down dnsmasq 2>/dev/null || true
  rm -f "$USB_DNSMASQ_CONF"

  if ip link show "$USB_IF" >/dev/null 2>&1; then
    reset_usb_if
    ip link set "$USB_IF" down 2>/dev/null || true
  fi

  unbind_gadget
  rm -f configs/c.1/f1 2>/dev/null || true
}

if [ -f "$NCM_PARAM" ] && [ "$(< "$NCM_PARAM")" = "1" ]; then
  echo "Enabling USB NCM"
  enable_ncm
else
  echo "Disabling USB NCM"
  disable_ncm
fi
