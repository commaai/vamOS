#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

TOOLS="$DIR/tools"
KERNEL_DIR="$DIR/kernel/linux"
PATCHES_DIR="$DIR/kernel/patches"
TMP_DIR="/tmp/vamos-build-tmp"
OUT_DIR="$DIR/output"
BOOT_IMG=./boot.img

DEFCONFIG="${DEFCONFIG:-vamos_defconfig}"
DTB="${DTB:-qcom/sdm845-mtp.dtb}"

# Check submodule initted, need to run setup
if [ ! -f "$KERNEL_DIR/Makefile" ]; then
  "$DIR/tools/setup.sh"
fi

# Build docker container
echo "Building vamos-builder docker image"
export DOCKER_BUILDKIT=1
docker build -f tools/build/Dockerfile.builder -t vamos-builder "$DIR"

echo "Starting vamos-builder container"
CONTAINER_ID=$(docker run -d -u "$(id -u):$(id -g)" -v "$DIR":"$DIR" -w "$DIR" vamos-builder)

trap "echo 'Cleaning up container:'; docker container rm -f $CONTAINER_ID; rm -rf $TMP_DIR" EXIT

apply_patches() {
  cd "$KERNEL_DIR"

  # Reset submodule to committed state for deterministic builds
  echo "-- Resetting kernel submodule to clean state --"
  git checkout .
  git clean -fd

  if [ -d "$PATCHES_DIR" ] && ls "$PATCHES_DIR"/*.patch 1>/dev/null 2>&1; then
    echo "-- Applying patches --"
    for patch in "$PATCHES_DIR"/*.patch; do
      echo "Applying $(basename "$patch")"
      git apply --check --whitespace=error "$patch"
      git apply --whitespace=error "$patch"
    done
  fi
}

build_kernel() {
  # Apply patches to kernel tree
  apply_patches

  # Cross-compilation setup
  ARCH_HOST=$(uname -m)
  export ARCH=arm64
  if [ "$ARCH_HOST" != "aarch64" ] && [ "$ARCH_HOST" != "arm64" ]; then
    export CROSS_COMPILE=aarch64-linux-gnu-
  fi

  # ccache
  export CCACHE_DIR="$DIR/.ccache"
  export PATH="/usr/lib/ccache/bin:$PATH"

  # Reproducible builds
  export KBUILD_BUILD_USER="vamos"
  export KBUILD_BUILD_HOST="vamos"
  export KCFLAGS="-w"

  # Build kernel
  cd "$KERNEL_DIR"

  echo "-- Loading $DEFCONFIG --"
  make defconfig O=out
  KCONFIG_CONFIG=out/.config scripts/kconfig/merge_config.sh -m out/.config "arch/arm64/configs/$DEFCONFIG"
  # Point EXTRA_FIRMWARE_DIR to our firmware directory so the kernel build
  # can find the blobs without symlinking into the kernel tree
  echo "CONFIG_EXTRA_FIRMWARE_DIR=\"$DIR/kernel/firmware\"" >> out/.config
  make olddefconfig O=out

  echo "-- Building kernel with $(nproc) cores --"
  make -j$(nproc) O=out Image.gz dtbs

  # Assemble Image.gz-dtb
  mkdir -p "$TMP_DIR"
  DTB_PATH="out/arch/arm64/boot/dts/$DTB"
  if [ ! -f "$DTB_PATH" ]; then
    echo "ERROR: DTB not found at $DTB_PATH"
    find out/arch/arm64/boot/dts -name '*.dtb' 2>/dev/null | head -20
    exit 1
  fi

  cat out/arch/arm64/boot/Image.gz "$DTB_PATH" > "$TMP_DIR/Image.gz-dtb"
  cd "$TMP_DIR"

  # Create boot.img
  mkdir -p "$OUT_DIR"
  $TOOLS/mkbootimg \
    --kernel Image.gz-dtb \
    --ramdisk /dev/null \
    --cmdline "console=ttyMSM0,115200n8 earlycon=msm_geni_serial,0xA84000 androidboot.hardware=qcom androidboot.console=ttyMSM0 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 service_locator.enable=1 androidboot.selinux=permissive firmware_class.path=/lib/firmware/updates net.ifnames=0" \
    --pagesize 4096 \
    --base 0x80000000 \
    --kernel_offset 0x8000 \
    --ramdisk_offset 0x8000 \
    --tags_offset 0x100 \
    --output $BOOT_IMG.nonsecure

  # Sign boot.img
  openssl dgst -sha256 -binary $BOOT_IMG.nonsecure > $BOOT_IMG.sha256
  openssl pkeyutl -sign -in $BOOT_IMG.sha256 -inkey $DIR/vble-qti.key -out $BOOT_IMG.sig -pkeyopt digest:sha256 -pkeyopt rsa_padding_mode:pkcs1
  dd if=/dev/zero of=$BOOT_IMG.sig.padded bs=2048 count=1 2>/dev/null
  dd if=$BOOT_IMG.sig of=$BOOT_IMG.sig.padded conv=notrunc 2>/dev/null
  cat $BOOT_IMG.nonsecure $BOOT_IMG.sig.padded > $BOOT_IMG

  rm -f $BOOT_IMG.nonsecure $BOOT_IMG.sha256 $BOOT_IMG.sig $BOOT_IMG.sig.padded

  mv $BOOT_IMG "$OUT_DIR/"
  echo "-- Done! boot.img: $OUT_DIR/boot.img --"
  ls -lh "$OUT_DIR/boot.img"
}

# Run build inside container
docker exec -u "$(id -u):$(id -g)" $CONTAINER_ID bash -c "set -e; export DEFCONFIG=$DEFCONFIG DIR=$DIR TOOLS=$TOOLS KERNEL_DIR=$KERNEL_DIR PATCHES_DIR=$PATCHES_DIR TMP_DIR=$TMP_DIR OUT_DIR=$OUT_DIR BOOT_IMG=$BOOT_IMG DTB=$DTB; $(declare -f apply_patches build_kernel); build_kernel"
