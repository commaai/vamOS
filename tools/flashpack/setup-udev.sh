#!/usr/bin/env bash
set -e

sudo tee /etc/udev/rules.d/99-qualcomm-edl.rules > /dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="05c6", ATTR{idProduct}=="9008", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="3801", ATTR{idProduct}=="9008", MODE="0666"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger

for d in /sys/bus/usb/drivers/qcserial/*-*
do
    [ -e "$d" ] && echo -n "$(basename $d)" | sudo tee /sys/bus/usb/drivers/qcserial/unbind > /dev/null
done

echo "Done. Unplug and replug your device."
