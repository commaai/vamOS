#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

TOOLS="$DIR/tools/bin"
KERNEL_DIR="$DIR/kernel/linux"
PATCHES_DIR="$DIR/kernel/patches"
KBUILD_OUT="$DIR/build/kernel-out"
TMP_DIR="$DIR/build/tmp-kernel"
OUT_DIR="$DIR/build"
BOOT_IMG=./boot.img

BASE_DEFCONFIG="defconfig"
CONFIG_FRAGMENT="$DIR/kernel/configs/vamos.config"

COMMON_DTSI="$DIR/kernel/dts/sdm845-comma-common.dtsi"
DTS_FILES=(
  "$DIR/kernel/dts/sdm845-comma-mici.dts"
  "$DIR/kernel/dts/sdm845-comma-tizi.dts"
)

# Check submodule initted, need to run setup
if [ ! -f "$KERNEL_DIR/Makefile" ]; then
  "$DIR/vamos" setup
fi

clean_kernel_tree() {
  git -C "$KERNEL_DIR" reset --hard HEAD >/dev/null 2>&1 || true
  git -C "$KERNEL_DIR" clean -fd >/dev/null 2>&1 || true
}

apply_patches() {
  cd "$KERNEL_DIR"

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

  cd "$DIR"
}

# Reset kernel source and apply patches before starting container
apply_patches

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

build_kernel() {
  # Install the device tree files
  install_dts

  # Cross-compilation setup
  ARCH_HOST=$(uname -m)
  export ARCH=arm64
  if [ "$ARCH_HOST" != "aarch64" ] && [ "$ARCH_HOST" != "arm64" ]; then
    export CROSS_COMPILE=aarch64-none-elf-
  fi

  # ccache (use CC= directly instead of PATH symlinks for reliability)
  export CCACHE_DIR="$DIR/.ccache"
  if [ -n "$CROSS_COMPILE" ]; then
    CC_CMD="ccache ${CROSS_COMPILE}gcc"
  else
    CC_CMD="ccache gcc"
  fi

  # Reproducible builds
  export KBUILD_BUILD_USER="vamos"
  export KBUILD_BUILD_HOST="vamos"
  export KCFLAGS="-w"

  # Build kernel
  cd "$KERNEL_DIR"

  mkdir -p "$KBUILD_OUT"

  echo "-- Loading base config $BASE_DEFCONFIG --"
  make CC="$CC_CMD" O="$KBUILD_OUT" "$BASE_DEFCONFIG"

  echo "-- Merging config fragment $(basename "$CONFIG_FRAGMENT") --"
  KCONFIG_CONFIG="$KBUILD_OUT/.config" \
    bash scripts/kconfig/merge_config.sh \
    -m "$KBUILD_OUT/.config" "$CONFIG_FRAGMENT"
  # Point EXTRA_FIRMWARE_DIR to our firmware directory so the kernel build
  # can find the blobs without symlinking into the kernel tree
  echo "CONFIG_EXTRA_FIRMWARE_DIR=\"$DIR/kernel/firmware\"" >> "$KBUILD_OUT/.config"
  make CC="$CC_CMD" O="$KBUILD_OUT" olddefconfig

  local dtb_targets=()
  local dts_name
  local IMAGE_GZ_DTB

  for dts in "${DTS_FILES[@]}"; do
    dts_name="$(basename "$dts")"
    dtb_targets+=("qcom/${dts_name%.dts}.dtb")
  done

  echo "-- Building kernel with $(nproc) cores --"
  make CC="$CC_CMD" -j$(nproc) O="$KBUILD_OUT" Image.gz "${dtb_targets[@]}"

  # Assemble Image.gz-dtb
  mkdir -p "$TMP_DIR"
  IMAGE_GZ_DTB="$TMP_DIR/Image.gz-dtb"
  cp "$KBUILD_OUT/arch/arm64/boot/Image.gz" "$IMAGE_GZ_DTB"

  for dts in "${DTS_FILES[@]}"; do
    dts_name="$(basename "$dts")"
    dtb_path="$KBUILD_OUT/arch/arm64/boot/dts/qcom/${dts_name%.dts}.dtb"
    cat "$dtb_path" >> "$IMAGE_GZ_DTB"
  done

  cd "$TMP_DIR"

  # Create boot.img
  mkdir -p "$OUT_DIR"
  $TOOLS/mkbootimg \
    --kernel Image.gz-dtb \
    --ramdisk /dev/null \
    --cmdline "console=ttyMSM0,115200n8 earlycon=msm_geni_serial,0xA84000 androidboot.hardware=qcom androidboot.console=ttyMSM0 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 service_locator.enable=1 androidboot.selinux=permissive firmware_class.path=/lib/firmware/updates net.ifnames=0 fbcon=rotate:3 adreno.address_space_size=0x800000000" \
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

cleanup() {
  echo "Cleaning up container and kernel tree..."

  clean_kernel_tree

  docker container rm -f "${CONTAINER_ID:-}" >/dev/null 2>&1 || true
  rm -rf "$TMP_DIR"
}

install_dts() {
  local dst_dir="$KERNEL_DIR/arch/arm64/boot/dts/qcom"

  echo "-- Installing DTS/DTSI files --"

  cp "$COMMON_DTSI" "$dst_dir/"
  for dts in "${DTS_FILES[@]}"; do
    cp "$dts" "$dst_dir/"
  done
}

# Run build inside container
docker exec -i -u "$(id -u):$(id -g)" "$CONTAINER_ID" bash <<EOF
set -e

BASE_DEFCONFIG='$BASE_DEFCONFIG'
CONFIG_FRAGMENT='$CONFIG_FRAGMENT'
COMMON_DTSI='$COMMON_DTSI'
DIR='$DIR'
TOOLS='$TOOLS'
KERNEL_DIR='$KERNEL_DIR'
PATCHES_DIR='$PATCHES_DIR'
KBUILD_OUT='$KBUILD_OUT'
TMP_DIR='$TMP_DIR'
OUT_DIR='$OUT_DIR'
BOOT_IMG='$BOOT_IMG'

DTS_FILES=(
  '${DTS_FILES[0]}'
  '${DTS_FILES[1]}'
)

$(declare -f build_kernel)
$(declare -f install_dts)

build_kernel
EOF
