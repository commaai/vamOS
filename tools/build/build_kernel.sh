#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

TOOLS="$DIR/tools/bin"
KERNEL_DIR="$DIR/kernel/linux"
PATCHES_DIR="$DIR/kernel/patches"
TMP_DIR="$DIR/build/tmp"
OUT_DIR="$DIR/build"
BOOT_IMG=./boot.img

BASE_DEFCONFIG="defconfig"
CONFIG_FRAGMENT="$DIR/kernel/configs/vamos.config"

DTS_FILE="$DIR/kernel/dts/sdm845-mici.dts"
DTB="qcom/$(basename "${DTS_FILE%.dts}.dtb")"

# Check submodule initted, need to run setup
if [ ! -f "$KERNEL_DIR/Makefile" ]; then
  "$DIR/vamos" setup
fi

# Build docker container
echo "Building vamos-builder docker image"
export DOCKER_BUILDKIT=1
docker build -f tools/build/Dockerfile.builder -t vamos-builder "$DIR" \
  --build-arg UNAME="$(id -nu)" \
  --build-arg UID="$(id -u)" \
  --build-arg GID="$(id -g)"

echo "Starting vamos-builder container"
CONTAINER_ID=$(docker run -d -u "$(id -u):$(id -g)" -v "$DIR":"$DIR" -w "$DIR" vamos-builder)

trap cleanup EXIT

apply_patches() {
  cd "$KERNEL_DIR"

  # Reset submodule to committed state for deterministic builds
  echo "-- Resetting kernel submodule to clean state --"
  clean_kernel_tree

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

  # Install the device tree files
  install_dts

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

  # Clean host-built scripts so they get rebuilt for the container
  rm -rf out/scripts

  echo "-- Loading base config $BASE_DEFCONFIG --"
  make O=out "$BASE_DEFCONFIG"

  echo "-- Merging config fragment $(basename "$CONFIG_FRAGMENT") --"
  KCONFIG_CONFIG=out/.config \
    bash scripts/kconfig/merge_config.sh \
    -m out/.config "$CONFIG_FRAGMENT"
  # Point EXTRA_FIRMWARE_DIR to our firmware directory so the kernel build
  # can find the blobs without symlinking into the kernel tree
  echo "CONFIG_EXTRA_FIRMWARE_DIR=\"$DIR/kernel/firmware\"" >> out/.config
  make olddefconfig O=out

  echo "-- Building kernel with $(nproc) cores --"
  make -j$(nproc) O=out Image.gz "$DTB"

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
  openssl pkeyutl -sign -in $BOOT_IMG.sha256 -inkey $DIR/tools/build/vble-qti.key -out $BOOT_IMG.sig -pkeyopt digest:sha256 -pkeyopt rsa_padding_mode:pkcs1
  dd if=/dev/zero of=$BOOT_IMG.sig.padded bs=2048 count=1 2>/dev/null
  dd if=$BOOT_IMG.sig of=$BOOT_IMG.sig.padded conv=notrunc 2>/dev/null
  cat $BOOT_IMG.nonsecure $BOOT_IMG.sig.padded > $BOOT_IMG

  rm -f $BOOT_IMG.nonsecure $BOOT_IMG.sha256 $BOOT_IMG.sig $BOOT_IMG.sig.padded

  mv $BOOT_IMG "$OUT_DIR/"
  echo "-- Done! boot.img: $OUT_DIR/boot.img --"
  ls -lh "$OUT_DIR/boot.img"
}

clean_kernel_tree() {
  git -C "$KERNEL_DIR" reset --hard HEAD >/dev/null 2>&1 || true
  git -C "$KERNEL_DIR" clean -fd >/dev/null 2>&1 || true
}

cleanup() {
  echo "Cleaning up container and kernel tree..."

  clean_kernel_tree

  docker container rm -f "${CONTAINER_ID:-}" >/dev/null 2>&1 || true
  rm -rf "$TMP_DIR"
}

install_dts() {
  local dts_name dtb_name

  dts_name="$(basename "$DTS_FILE")"
  dtb_name="${dts_name%.dts}.dtb"

  echo "-- Installing DTS $dts_name --"
  cp "$DTS_FILE" "$KERNEL_DIR/arch/arm64/boot/dts/qcom/"
}

# Run build inside container
docker exec -u "$(id -u):$(id -g)" $CONTAINER_ID bash -c "set -e; export BASE_DEFCONFIG='$BASE_DEFCONFIG' CONFIG_FRAGMENT='$CONFIG_FRAGMENT' DTS_FILE='$DTS_FILE' DIR=$DIR TOOLS=$TOOLS KERNEL_DIR=$KERNEL_DIR PATCHES_DIR=$PATCHES_DIR TMP_DIR=$TMP_DIR OUT_DIR=$OUT_DIR BOOT_IMG=$BOOT_IMG DTB=$DTB; $(declare -f apply_patches build_kernel clean_kernel_tree install_dts); build_kernel"
