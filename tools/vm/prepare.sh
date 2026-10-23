#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

BUILD_DIR="$DIR/build"

# Arch Linux
if [ -f /usr/bin/pacman ]; then
  sudo pacman -S --needed \
    docker \
    jq \
    docker-buildx \
    bc \
    qemu-full \
    android-tools
 
  if ! systemctl is-active --quiet docker.service; then
    sudo systemctl start docker.service
  fi

fi

# Ubuntu/Debian
if [ -f /usr/bin/apt ]; then
  sudo apt-get install \
    jq \
    git-lfs \
    docker.io \
    docker-buildx \
    qemu-system \
    android-sdk-libsparse-utils \
    bc \
    -y

  if ! systemctl is-active --quiet docker.service; then
    sudo systemctl start docker.service
  fi
fi

if ! groups "$USER" | grep -q "\bdocker\b"; then
  echo "Adding $USER to docker group..."
  sudo groupadd docker || true
  sudo usermod -aG docker "$USER"
  echo "Please log out and log back in for docker group changes to take effect."
fi

if [ ${BUILD_DIR}/system.img -nt ${BUILD_DIR}/system_raw.img ]; then
  echo "Converting system.img to raw format..."
  simg2img ${BUILD_DIR}/system.img ${BUILD_DIR}/system_raw.img
else
  echo "system_raw.img is up to date."
fi
