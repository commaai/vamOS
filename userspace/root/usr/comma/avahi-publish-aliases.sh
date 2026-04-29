#!/bin/bash
set -u
exec 2>&1

. /usr/comma/mdns_helpers.sh

WATCH_INTERFACES=(wlan0 usb0)
PUBLISH_PIDS=()

cleanup_publishers() {
  local pid

  for pid in "${PUBLISH_PIDS[@]}"; do
    kill "$pid" 2>/dev/null || true
  done

  for pid in "${PUBLISH_PIDS[@]}"; do
    wait "$pid" 2>/dev/null || true
  done

  PUBLISH_PIDS=()
}

wait_for_avahi() {
  until [ -S /run/avahi-daemon/socket ]; do
    sleep 1
  done
}

get_interface_addresses() {
  local iface

  for iface in "${WATCH_INTERFACES[@]}"; do
    ip -4 -o addr show dev "$iface" scope global 2>/dev/null | awk '{split($4, cidr, "/"); print cidr[1]}'
  done | sort -u
}

refresh_publishers() {
  local alias alias_fqdn addresses address

  alias="$(get_model_alias 2>/dev/null || true)"
  cleanup_publishers

  if [ -z "$alias" ]; then
    return 0
  fi

  alias_fqdn="${alias}.local"

  wait_for_avahi
  addresses="$(get_interface_addresses)"
  if [ -z "$addresses" ]; then
    echo "No active IPv4 addresses for $alias"
    return 0
  fi

  while IFS= read -r address; do
    [ -n "$address" ] || continue
    avahi-publish -a -R "$alias_fqdn" "$address" &
    PUBLISH_PIDS+=("$!")
  done <<< "$addresses"

  echo "Publishing $alias for: $(printf '%s ' $addresses)"
}

handle_monitor_event() {
  case "$1" in
    *"wlan0"*|*"usb0"*)
      refresh_publishers
      ;;
  esac
}

trap 'cleanup_publishers' EXIT INT TERM

if ! get_model_alias >/dev/null 2>&1; then
  echo "Skipping model alias publish: unsupported device model"
  exec sleep infinity
fi

refresh_publishers

while true; do
  while IFS= read -r line; do
    handle_monitor_event "$line"
  done < <(ip monitor address 2>/dev/null)

  sleep 1
done
