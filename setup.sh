#!/bin/bash

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

git -C "$DIR" submodule update --init --depth 1

# Install kernel build dependencies
echo "Installing kernel build dependencies..."
sudo apt-get install -y build-essential libssl-dev bc openssl flex bison libelf-dev python3 ccache

# Install cross-compiler if not on aarch64
ARCH_HOST=$(uname -m)
if [ "$ARCH_HOST" != "aarch64" ] && [ "$ARCH_HOST" != "arm64" ]; then
  echo "Installing aarch64 cross-compiler..."
  sudo apt-get install -y gcc-aarch64-linux-gnu
fi

# Set up udev rules for Qualcomm EDL mode (needed for qdl flashing)
UDEV_RULES="/etc/udev/rules.d/99-qualcomm-edl.rules"
if [ ! -f "$UDEV_RULES" ]; then
  echo "Setting up udev rules for Qualcomm EDL..."
  sudo tee "$UDEV_RULES" > /dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="05c6", ATTR{idProduct}=="9008", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="3801", ATTR{idProduct}=="9008", MODE="0666"
EOF
  sudo udevadm trigger --attr-match=subsystem=usb
fi
