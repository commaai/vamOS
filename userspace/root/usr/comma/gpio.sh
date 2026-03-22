#!/bin/bash
source /usr/comma/gpio_base.sh

pins=(
# 27  # SW_3V3_EN
# 25  # SW_5V_EN
30  # HUB_RST_N
49  # SOM_ST_IO
134 # ST_BOOT0
41  # PANDA_1V8_EN_N
50  # LTE_RST_N
116 # LTE_PWRKEY
124 # ST_RST_N
34  # GPS_PWR_EN
33  # GPS_SAFEBOOT_N
32  # GPS_RST_N
52  # LTE_BOOT
)

# PM8998 GPIO4 = INA231 POWER ALERT (not a TLMM pin, separate GPIO chip)
if [ "$PM8998_BASE" -gt 0 ]; then
  POWER_ALERT_PIN=$((PM8998_BASE + 3))
  echo "POWER_ALERT (gpio$POWER_ALERT_PIN)"
  echo $POWER_ALERT_PIN > /sys/class/gpio/export
  until [ -d /sys/class/gpio/gpio$POWER_ALERT_PIN ]; do sleep .05; done
  chown root:gpio /sys/class/gpio/gpio$POWER_ALERT_PIN/direction /sys/class/gpio/gpio$POWER_ALERT_PIN/value 2>/dev/null
  chmod 660 /sys/class/gpio/gpio$POWER_ALERT_PIN/direction /sys/class/gpio/gpio$POWER_ALERT_PIN/value 2>/dev/null
fi

for p in ${pins[@]}; do
  pin=$((TLMM_BASE + p))
  echo "$p (gpio$pin)"

  # this is SSD_3v3 EN on tici
  if [ "$p" -eq 41 ] && grep -q "comma tici" /sys/firmware/devicetree/base/model; then
    echo "Skipping $p"
    continue
  fi

  echo $pin > /sys/class/gpio/export
  until [ -d /sys/class/gpio/gpio$pin ]
  do
    sleep .05
  done
  # eudev doesn't apply GROUP/MODE from udev rules to sysfs GPIO files
  # like systemd-udevd does, so set permissions manually after export
  chown root:gpio /sys/class/gpio/gpio$pin/direction /sys/class/gpio/gpio$pin/value 2>/dev/null
  chmod 660 /sys/class/gpio/gpio$pin/direction /sys/class/gpio/gpio$pin/value 2>/dev/null
done


HUB_RST_N=30
gpio $HUB_RST_N 1

touch /run/gpio.ready
