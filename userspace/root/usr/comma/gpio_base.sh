#!/bin/bash

# Get TLMM GPIO chip base (dynamic on mainline, was 0 on downstream)
TLMM_BASE=$(cat /sys/bus/platform/devices/3400000.pinctrl/gpio/*/base 2>/dev/null | head -1)
TLMM_BASE=${TLMM_BASE:-0}

# Get PM8998 GPIO chip base (SPMI USID 0, GPIO @ 0xc000)
for chip in /sys/class/gpio/gpiochip*/; do
  label=$(cat "$chip/label" 2>/dev/null)
  if [[ "$label" == *"spmi"*"pmic@0"*"gpio"* ]] || [[ "$label" == *"pm8998"*"gpio"* ]]; then
    PM8998_BASE=$(cat "$chip/base")
    break
  fi
done
PM8998_BASE=${PM8998_BASE:-0}

function gpio {
  local pin=$((TLMM_BASE + $1))
  echo "out" > /sys/class/gpio/gpio$pin/direction
  echo $2 > /sys/class/gpio/gpio$pin/value
}
