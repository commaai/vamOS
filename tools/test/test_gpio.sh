#!/bin/bash
# GPIO PR validation script — run on device via SSH
set -o pipefail

PASS=0
FAIL=0
pass() { echo "  PASS: $1"; ((PASS++)); }
fail() { echo "  FAIL: $1"; ((FAIL++)); }
check() { if eval "$1" >/dev/null 2>&1; then pass "$2"; else fail "$2"; fi; }

MODEL=$(cat /sys/firmware/devicetree/base/model 2>/dev/null | tr -d '\0')
echo "=== GPIO validation on: $MODEL ==="
echo

# 1. GPIO chip enumeration
echo "[1] GPIO chip enumeration"
TLMM_BASE=$(cat /sys/bus/platform/devices/3400000.pinctrl/gpio/*/base 2>/dev/null | head -1)
check "[ -n '$TLMM_BASE' ]" "TLMM chip present (base=$TLMM_BASE)"

PM8998_BASE=""
PMI8998_BASE=""
for chip in /sys/class/gpio/gpiochip*/; do
  label=$(cat "$chip/label" 2>/dev/null)
  base=$(cat "$chip/base" 2>/dev/null)
  ngpio=$(cat "$chip/ngpio" 2>/dev/null)
  echo "  $label: base=$base ngpio=$ngpio"
  if [[ "$label" == *"pmic@0"*"gpio"* ]]; then PM8998_BASE=$base; fi
  if [[ "$label" == *"pmic@2"*"gpio"* ]]; then PMI8998_BASE=$base; fi
done
check "[ -n '$PM8998_BASE' ]" "PM8998 GPIO chip present (base=$PM8998_BASE)"
check "[ -n '$PMI8998_BASE' ]" "PMI8998 GPIO chip present (base=$PMI8998_BASE)"
echo

# 2. TLMM pin exports (gpio.sh)
echo "[2] TLMM pin exports"
check "[ -f /run/gpio.ready ]" "gpio.ready exists"
TLMM_PINS=(30 49 134 41 50 116 124 34 33 32 52)
for p in "${TLMM_PINS[@]}"; do
  pin=$((TLMM_BASE + p))
  # tici skips pin 41 (SSD_3v3 EN)
  if [ "$p" -eq 41 ] && echo "$MODEL" | grep -q "tici"; then
    check "! [ -d /sys/class/gpio/gpio$pin ]" "gpio$pin (TLMM $p) skipped on tici"
  else
    check "[ -d /sys/class/gpio/gpio$pin ]" "gpio$pin (TLMM $p) exported"
  fi
done
echo

# 3. Power alert pin (PM8998_GPIO4)
echo "[3] Power alert pin"
if [ -n "$PM8998_BASE" ]; then
  ALERT_PIN=$((PM8998_BASE + 3))
  check "[ -d /sys/class/gpio/gpio$ALERT_PIN ]" "gpio$ALERT_PIN (PM8998_GPIO4) exported"
  check "[ \"\$(cat /sys/class/gpio/gpio$ALERT_PIN/direction)\" = 'in' ]" "direction = in"
  check "[ \"\$(cat /sys/class/gpio/gpio$ALERT_PIN/edge)\" = 'falling' ]" "edge = falling"
  echo "  value: $(cat /sys/class/gpio/gpio$ALERT_PIN/value)"
fi
echo

# 4. INA231 power monitor
echo "[4] INA231 power monitor"
check "ls /sys/bus/i2c/devices/*-0040" "INA231 I2C device probed"
INA_HWMON=""
for h in /sys/class/hwmon/hwmon*/; do
  if [ "$(cat "$h/name" 2>/dev/null)" = "ina231" ]; then
    INA_HWMON=$h
    break
  fi
done
if [ -n "$INA_HWMON" ]; then
  pass "INA231 hwmon present"
  V=$(cat "$INA_HWMON/in1_input" 2>/dev/null)
  I=$(cat "$INA_HWMON/curr1_input" 2>/dev/null)
  echo "  voltage: ${V} mV, current: ${I} mA"
  check "[ '$V' -gt 4000 ] && [ '$V' -lt 6000 ]" "voltage sane (4000-6000 mV)"
  check "[ '$I' -gt 0 ]" "current > 0 mA"
  INA_OK=1
else
  echo "  WARN: INA231 hwmon not present (chip not responding — known issue on some boards)"
  INA_OK=0
fi
echo

# 5. power_drop_monitor service
echo "[5] power_drop_monitor"
PDM_STATUS=$(sudo sv status power_drop_monitor 2>/dev/null || true)
echo "  $PDM_STATUS"
if [ "$INA_OK" = "1" ]; then
  check "echo '$PDM_STATUS' | grep -q '^run:'" "power_drop_monitor running"
else
  echo "  WARN: skipped (INA231 not available, power_drop_monitor expected to fail)"
fi
echo

# 6. LTE service (lte.sh uses TLMM offset)
echo "[6] LTE service"
LTE_STATUS=$(sudo sv status lte 2>/dev/null || true)
echo "  $LTE_STATUS"
# lte service may be "run" or "finish" depending on modem presence, but should not be crashing
check "! echo '$LTE_STATUS' | grep -q 'want down'" "lte service not crash-looping"
echo

# 7. USB hub reset
echo "[7] USB hub reset"
HUB_PIN=$((TLMM_BASE + 30))
check "[ \"\$(cat /sys/class/gpio/gpio$HUB_PIN/value)\" = '1' ]" "HUB_RST_N deasserted (value=1)"
USB_DEVS=$(ls /sys/bus/usb/devices/ 2>/dev/null | wc -l)
echo "  USB devices: $USB_DEVS"
check "[ '$USB_DEVS' -gt 2 ]" "USB devices present behind hub"
echo

# 8. Kernel errors
echo "[8] Kernel health"
DEFERRED=$(cat /sys/kernel/debug/devices_deferred 2>/dev/null | wc -l)
echo "  deferred devices: $DEFERRED"
ERRORS=$(dmesg | grep -ciE "error|fail|unable" 2>/dev/null || echo 0)
echo "  dmesg error/fail lines: $ERRORS"
echo

# Summary
echo "================================"
echo "PASS: $PASS  FAIL: $FAIL"
[ "$FAIL" -eq 0 ] && echo "ALL TESTS PASSED" || echo "SOME TESTS FAILED"
exit $FAIL
