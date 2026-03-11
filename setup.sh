#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

git -C "$DIR" submodule update --init --depth 1

# Set up udev rules for Qualcomm EDL mode (needed for qdl flashing)
if [ "$(uname)" = "Linux" ]; then
  UDEV_RULES="/etc/udev/rules.d/99-qualcomm-edl.rules"
  if [ ! -f "$UDEV_RULES" ]; then
    echo "Setting up udev rules for Qualcomm EDL..."
    sudo tee "$UDEV_RULES" > /dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="05c6", ATTR{idProduct}=="9008", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="3801", ATTR{idProduct}=="9008", MODE="0666"
EOF
    sudo udevadm trigger --attr-match=subsystem=usb
  fi
fi
