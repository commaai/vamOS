#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

TOOLS="$DIR/tools/bin"
KERNEL_DIR="$DIR/kernel/linux"
PATCHES_DIR="$DIR/kernel/patches"
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

HOST_OS="$(uname)"
KERNEL_LINUX_VOLUME="vamos-kernel-linux"
CCACHE_VOLUME="vamos-kernel-ccache"
CONTAINER_ID=""
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
    export CROSS_COMPILE=aarch64-none-elf-
  fi

  # ccache
  if [ "$HOST_OS" = "Darwin" ]; then
    export CCACHE_DIR="/ccache"
  else
    export CCACHE_DIR="$DIR/.ccache"
  fi
  local make_args=(
    O=out
    CC="ccache ${CROSS_COMPILE:-}gcc"
    HOSTCC="ccache gcc"
    HOSTCXX="ccache g++"
  )

  # Reproducible builds
  export KBUILD_BUILD_USER="vamos"
  export KBUILD_BUILD_HOST="vamos"
  export KCFLAGS="-w"

  # GIT_REV is computed on the host and passed in via the container heredoc.
  export LANG=C.UTF-8
  export LC_ALL=C.UTF-8
  export LOCALVERSION="-vamos-$GIT_REV"

  # Build kernel
  cd "$KERNEL_DIR"

  echo "-- Loading base config $BASE_DEFCONFIG --"
  make "${make_args[@]}" "$BASE_DEFCONFIG"

  echo "-- Merging config fragment $(basename "$CONFIG_FRAGMENT") --"
  KCONFIG_CONFIG=out/.config \
    bash scripts/kconfig/merge_config.sh \
    -m -y out/.config "$CONFIG_FRAGMENT"
  # Point EXTRA_FIRMWARE_DIR to our firmware directory
  echo "CONFIG_EXTRA_FIRMWARE_DIR=\"$DIR/kernel/firmware\"" >> out/.config
  make "${make_args[@]}" olddefconfig

  local dtb_targets=()
  local dts_name
  local IMAGE_GZ_DTB

  for dts in "${DTS_FILES[@]}"; do
    dts_name="$(basename "$dts")"
    dtb_targets+=("qcom/${dts_name%.dts}.dtb")
  done

  echo "-- Building kernel with $(nproc) cores --"
  make -j$(nproc) "${make_args[@]}" Image.gz "${dtb_targets[@]}"
  python3 scripts/clang-tools/gen_compile_commands.py -d out -o "$DIR/compile_commands.json"

  # Assemble Image.gz-dtb
  mkdir -p "$TMP_DIR"
  IMAGE_GZ_DTB="$TMP_DIR/Image.gz-dtb"
  cp out/arch/arm64/boot/Image.gz "$IMAGE_GZ_DTB"

  for dts in "${DTS_FILES[@]}"; do
    dts_name="$(basename "$dts")"
    dtb_path="out/arch/arm64/boot/dts/qcom/${dts_name%.dts}.dtb"
    cat "$dtb_path" >> "$IMAGE_GZ_DTB"
  done

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

# Check submodule initted, need to run setup
if [ ! -f "$KERNEL_DIR/Makefile" ]; then
  "$DIR/vamos" setup
fi

# Compute on host; in-container git fails for worktrees (.git is outside $DIR).
GIT_REV="$(git -C "$DIR" rev-parse --short HEAD)"

if [ -f "/.dockerenv" ]; then
  git -C / config --global --add safe.directory "$DIR"
  git -C / config --global --add safe.directory "$KERNEL_DIR"

  trap cleanup EXIT
  build_kernel
  exit 0
fi

if [ "$HOST_OS" = "Darwin" ]; then
  echo "Preparing vamos-builder image and Docker volumes"
  "$DIR/tools/build/prepare_kernel_volume.sh"
  echo "Starting vamos-builder container"
  CONTAINER_ID=$(docker run -d \
    -u "$(id -u):$(id -g)" \
    -v "$DIR":"$DIR" \
    -v "$KERNEL_LINUX_VOLUME:$KERNEL_DIR" \
    -v "$CCACHE_VOLUME:/ccache" \
    -w "$DIR" \
    vamos-builder)
else
  echo "Preparing vamos-builder image"
  "$DIR/tools/build/prepare_builder_image.sh"
  echo "Starting vamos-builder container"
  # For a git worktree, mount the shared .git so in-container git resolves.
  GIT_MOUNT_ARGS=()
  if [ -f "$DIR/.git" ]; then
    GIT_COMMON_DIR="$(git -C "$DIR" rev-parse --path-format=absolute --git-common-dir)"
    GIT_MOUNT_ARGS=(-v "$GIT_COMMON_DIR":"$GIT_COMMON_DIR")
  fi
  CONTAINER_ID=$(docker run -d -u "$(id -u):$(id -g)" -v "$DIR":"$DIR" "${GIT_MOUNT_ARGS[@]}" -w "$DIR" vamos-builder)
fi

trap cleanup EXIT

# Run build inside container
docker exec -i -u "$(id -u):$(id -g)" "$CONTAINER_ID" bash <<EOF
set -e

HOST_OS='$HOST_OS'
BASE_DEFCONFIG='$BASE_DEFCONFIG'
CONFIG_FRAGMENT='$CONFIG_FRAGMENT'
COMMON_DTSI='$COMMON_DTSI'
DIR='$DIR'
TOOLS='$TOOLS'
KERNEL_DIR='$KERNEL_DIR'
PATCHES_DIR='$PATCHES_DIR'
TMP_DIR='$TMP_DIR'
OUT_DIR='$OUT_DIR'
BOOT_IMG='$BOOT_IMG'
GIT_REV='$GIT_REV'

DTS_FILES=(
  '${DTS_FILES[0]}'
  '${DTS_FILES[1]}'
)

# building both kernel and system at same time cause git dubious ownership errors
git -C / config --global --add safe.directory '$DIR'
git -C / config --global --add safe.directory '$KERNEL_DIR'

$(declare -f apply_patches)
$(declare -f build_kernel)
$(declare -f clean_kernel_tree)
$(declare -f install_dts)

build_kernel
EOF
