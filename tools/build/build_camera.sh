#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

KERNEL_DIR="$DIR/kernel/linux"
PATCHES_DIR="$DIR/kernel/patches"
CONFIG_FRAGMENT="$DIR/kernel/configs/vamos.config"
CAMERA_DIR="$DIR/kernel/camera"

# Prep kernel tree (patches + camera source) if not already done
prep_tree() {
  cd "$KERNEL_DIR"

  # Only reset if no out/.config exists (first run)
  if [ ! -f out/.config ]; then
    echo "-- Resetting kernel submodule --"
    git reset --hard HEAD >/dev/null 2>&1 || true
    git clean -fd >/dev/null 2>&1 || true

    if [ -d "$PATCHES_DIR" ] && ls "$PATCHES_DIR"/*.patch 1>/dev/null 2>&1; then
      echo "-- Applying patches --"
      for patch in "$PATCHES_DIR"/*.patch; do
        echo "Applying $(basename "$patch")"
        git apply --whitespace=error "$patch"
      done
    fi

    # Install camera source
    echo "-- Installing camera driver source --"
    cp -r "$CAMERA_DIR/drivers/media/platform/msm" "$KERNEL_DIR/drivers/media/platform/"
    mkdir -p "$KERNEL_DIR/include/uapi/media"
    cp "$CAMERA_DIR/include/uapi/media/"*.h "$KERNEL_DIR/include/uapi/media/"

    # Configure
    export ARCH=arm64
    ARCH_HOST=$(uname -m)
    if [ "$ARCH_HOST" != "aarch64" ] && [ "$ARCH_HOST" != "arm64" ]; then
      export CROSS_COMPILE=aarch64-none-elf-
    fi

    echo "-- Configuring kernel --"
    make O=out defconfig
    KCONFIG_CONFIG=out/.config bash scripts/kconfig/merge_config.sh -m -y out/.config "$CONFIG_FRAGMENT"
    make olddefconfig O=out
  else
    # Just re-copy camera source (may have been edited)
    echo "-- Updating camera driver source --"
    rm -rf "$KERNEL_DIR/drivers/media/platform/msm"
    cp -r "$CAMERA_DIR/drivers/media/platform/msm" "$KERNEL_DIR/drivers/media/platform/"
    cp "$CAMERA_DIR/include/uapi/media/"*.h "$KERNEL_DIR/include/uapi/media/"
  fi
}

build_camera() {
  cd "$KERNEL_DIR"

  export ARCH=arm64
  export KCFLAGS="-w"
  ARCH_HOST=$(uname -m)
  if [ "$ARCH_HOST" != "aarch64" ] && [ "$ARCH_HOST" != "arm64" ]; then
    export CROSS_COMPILE=aarch64-none-elf-
  fi

  export CCACHE_DIR="$DIR/.ccache"
  export PATH="/usr/lib/ccache/bin:$PATH"

  echo "-- Building camera drivers --"
  make -j$(nproc) O=out drivers/media/platform/msm/
}

# Build docker image
echo "Building vamos-builder docker image"
export DOCKER_BUILDKIT=1
docker build -q -f tools/build/Dockerfile.builder -t vamos-builder "$DIR" \
  --build-arg UNAME="$(id -nu)" \
  --build-arg UID="$(id -u)" \
  --build-arg GID="$(id -g)" >/dev/null

echo "Starting container"
CONTAINER_ID=$(docker run -d -u "$(id -u):$(id -g)" -v "$DIR":"$DIR" -w "$DIR" vamos-builder)
trap 'docker container rm -f "$CONTAINER_ID" >/dev/null 2>&1' EXIT

docker exec -i -u "$(id -u):$(id -g)" "$CONTAINER_ID" bash <<EOF
set -e
DIR='$DIR'
KERNEL_DIR='$KERNEL_DIR'
PATCHES_DIR='$PATCHES_DIR'
CONFIG_FRAGMENT='$CONFIG_FRAGMENT'
CAMERA_DIR='$CAMERA_DIR'

git config --global --add safe.directory '$DIR'
git config --global --add safe.directory '$KERNEL_DIR'

$(declare -f prep_tree)
$(declare -f build_camera)

prep_tree
build_camera
EOF
