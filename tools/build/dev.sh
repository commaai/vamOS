#!/bin/bash
set -e

#
# Kernel dev build - edit source, build, push, repeat.
# Does NOT reset the kernel tree. Works directly on kernel/linux/.
#
# Prerequisites: run build_kernel.sh once first (builds docker image + .config)
#
# Usage:
#   ./tools/build/dev.sh                  # build camss module
#   ./tools/build/dev.sh kernel           # full kernel + modules
#   ./tools/build/dev.sh push [host]      # scp module to device
#   ./tools/build/dev.sh config           # reconfigure kernel (after config changes)
#

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
KERNEL_DIR="$DIR/kernel/linux"
KBUILD_OUT="$DIR/build/kernel-out"
CONFIG_FRAGMENT="$DIR/kernel/configs/vamos.config"
DEVICE="${DEVICE:-comma@comma-9539449f}"
MODULE_KO="$KBUILD_OUT/drivers/media/platform/qcom/camss/qcom-camss.ko"

# Cross-compilation setup: match build_kernel.sh logic
# On aarch64/arm64 hosts the container's native gcc is the right compiler;
# on x86_64 hosts the container has aarch64-none-elf-gcc installed separately.
ARCH_HOST=$(uname -m)
if [ "$ARCH_HOST" != "aarch64" ] && [ "$ARCH_HOST" != "arm64" ]; then
  CROSS_COMPILE="aarch64-none-elf-"
  CC_CMD="ccache ${CROSS_COMPILE}gcc"
else
  CROSS_COMPILE=""
  CC_CMD="ccache gcc"
fi

MAKE="make ARCH=arm64 \
  ${CROSS_COMPILE:+CROSS_COMPILE=$CROSS_COMPILE} \
  CC='$CC_CMD' \
  CCACHE_DIR=$DIR/.ccache \
  KBUILD_BUILD_USER=vamos KBUILD_BUILD_HOST=vamos KCFLAGS=-w \
  O=$KBUILD_OUT"

# Reuse running container or start a new one
CONTAINER_NAME="vamos-dev"
if ! docker inspect "$CONTAINER_NAME" >/dev/null 2>&1; then
  echo "Starting dev container (persistent)..."
  docker run -d --name "$CONTAINER_NAME" \
    -u "$(id -u):$(id -g)" \
    -v "$DIR":"$DIR" -w "$DIR" \
    vamos-builder sleep infinity
fi

run() {
  docker exec -i -u "$(id -u):$(id -g)" "$CONTAINER_NAME" \
    bash -c "cd $KERNEL_DIR && $*"
}

case "${1:-module}" in
  module|camss)
    echo "Building camera modules..."
    run "$MAKE -j\$(nproc) M=drivers/media/platform/qcom/camss modules"
    run "$MAKE -j\$(nproc) M=drivers/media/i2c modules"
    ls -lh "$MODULE_KO"
    find "$KBUILD_OUT/drivers/media/i2c" -name "ox03c10.ko" -exec ls -lh {} \; 2>/dev/null
    ;;

  kernel)
    echo "Building kernel + modules..."
    # Install DTS files
    cp "$DIR/kernel/dts/sdm845-comma-common.dtsi" "$KERNEL_DIR/arch/arm64/boot/dts/qcom/"
    cp "$DIR"/kernel/dts/sdm845-comma-*.dts "$KERNEL_DIR/arch/arm64/boot/dts/qcom/"
    run "$MAKE -j\$(nproc) Image.gz modules qcom/sdm845-comma-tizi.dtb qcom/sdm845-comma-mici.dtb"
    ;;

  config)
    echo "Reconfiguring kernel..."
    run "$MAKE defconfig"
    run "KCONFIG_CONFIG=$KBUILD_OUT/.config bash scripts/kconfig/merge_config.sh -m $KBUILD_OUT/.config $CONFIG_FRAGMENT"
    run "echo 'CONFIG_EXTRA_FIRMWARE_DIR=\"$DIR/kernel/firmware\"' >> $KBUILD_OUT/.config"
    run "$MAKE olddefconfig"
    ;;

  push)
    HOST="${2:-$DEVICE}"
    scp "$MODULE_KO" "$HOST:/tmp/"
    echo "Reload: sudo rmmod qcom-camss; sudo insmod /tmp/qcom-camss.ko"
    ;;

  shell)
    docker exec -it -u "$(id -u):$(id -g)" "$CONTAINER_NAME" bash
    ;;

  clean)
    docker rm -f "$CONTAINER_NAME" 2>/dev/null || true
    echo "Dev container removed."
    ;;

  *)
    echo "Usage: $0 {module|kernel|config|push [host]|shell|clean}"
    ;;
esac
