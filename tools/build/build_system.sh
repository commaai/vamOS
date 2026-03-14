#!/usr/bin/env bash
set -e

VOID_ROOTFS_URL="https://repo-default.voidlinux.org/live/current/void-aarch64-ROOTFS-20250202.tar.xz"
VOID_ROOTFS_SHA256="01a30f17ae06d4d5b322cd579ca971bc479e02cc284ec1e5a4255bea6bac3ce6"

# Make sure we're in the correct spot
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"
cd "$DIR"

DOWNLOADS_DIR="$DIR/build/downloads"
VOID_ROOTFS_FILE="$DOWNLOADS_DIR/void-aarch64-ROOTFS-20250202.tar.xz"
BUILD_DIR="$DIR/build/tmp"
OUTPUT_DIR="$DIR/build"

ROOTFS_DIR="$BUILD_DIR/void-rootfs"
ROOTFS_IMAGE="$BUILD_DIR/system.img"
OUT_IMAGE="$OUTPUT_DIR/system.img"

# the partition is 10G, but openpilot's updater didn't always handle the full size
# Increased from 4500M to 6G for Python packages
ROOTFS_IMAGE_SIZE=6G

# Create temp dir if non-existent
mkdir -p "$BUILD_DIR" "$OUTPUT_DIR" "$DOWNLOADS_DIR"

# Download Void rootfs if not done already
if [ ! -f "$VOID_ROOTFS_FILE" ]; then
  echo "Downloading Void Linux rootfs: $VOID_ROOTFS_FILE"
  if ! curl -C - -o "$VOID_ROOTFS_FILE" "$VOID_ROOTFS_URL" --silent --remote-time --fail; then
    echo "Download failed"
    exit 1
  fi
fi

# Check SHA256 sum
if [ "$(shasum -a 256 "$VOID_ROOTFS_FILE" | awk '{print $1}')" != "$VOID_ROOTFS_SHA256" ]; then
  echo "Checksum mismatch"
  exit 1
fi

# Setup qemu multiarch
if [ "$(uname -m)" = "x86_64" ]; then
  echo "Registering emulator"
  docker run --rm --privileged tonistiigi/binfmt --install all
fi

# Check Dockerfile
export DOCKER_BUILDKIT=1
docker buildx build -f tools/build/Dockerfile --check "$DIR"

# Setup mount container for macOS and CI support
echo "Building vamos-builder docker image"
docker build -f tools/build/Dockerfile.builder -t vamos-builder "$DIR" \
  --build-arg UNAME="$(id -nu)" \
  --build-arg UID="$(id -u)" \
  --build-arg GID="$(id -g)"

echo "Starting builder container"
MOUNT_CONTAINER_ID=$(docker run -d --privileged -v "$DIR:$DIR" vamos-builder)

# Cleanup containers on possible exit
trap "echo \"Cleaning up containers:\"; \
docker container rm -f $MOUNT_CONTAINER_ID" EXIT

# Define functions for docker execution
exec_as_user() {
  docker exec -u "$(id -nu)" "$MOUNT_CONTAINER_ID" "$@"
}

exec_as_root() {
  docker exec "$MOUNT_CONTAINER_ID" "$@"
}

# Create filesystem ext4 image
echo "Creating empty filesystem"
exec_as_user fallocate -l "$ROOTFS_IMAGE_SIZE" "$ROOTFS_IMAGE"
exec_as_user mkfs.ext4 "$ROOTFS_IMAGE" &> /dev/null

# Mount filesystem
echo "Mounting empty filesystem"
exec_as_root mkdir -p "$ROOTFS_DIR"
exec_as_root mount "$ROOTFS_IMAGE" "$ROOTFS_DIR"

# Also unmount filesystem (overwrite previous trap)
trap "exec_as_root umount -l $ROOTFS_DIR &> /dev/null || true; \
echo \"Cleaning up containers:\"; \
docker container rm -f $MOUNT_CONTAINER_ID" EXIT

echo "Building and extracting vamos docker image"
docker buildx build -f tools/build/Dockerfile --platform=linux/arm64 \
  --output "type=tar,dest=-" \
  --provenance=false \
  --build-arg VOID_ROOTFS="${VOID_ROOTFS_FILE#"$DIR/"}" \
  "$DIR" | docker exec -i "$MOUNT_CONTAINER_ID" tar -xf - -C "$ROOTFS_DIR"
echo "Build and extraction complete"

# Avoid detecting as container
echo "Removing .dockerenv file"
exec_as_root rm -f "$ROOTFS_DIR/.dockerenv"

echo "Setting network stuff"
GIT_HASH=${GIT_HASH:-$(git --git-dir="$DIR/.git" rev-parse HEAD)}
DATETIME=$(date '+%Y-%m-%dT%H:%M:%S')
exec_as_root sh -c "
  set -e
  cd '$ROOTFS_DIR'

  # Add hostname and hosts
  HOST=comma
  ln -sf /proc/sys/kernel/hostname etc/hostname
  echo '127.0.0.1    localhost.localdomain localhost' > etc/hosts
  echo \"127.0.0.1    \$HOST\" >> etc/hosts

  # DNS: resolv.conf must be writable for NetworkManager
  # Docker mounts resolv.conf during build so we do this after export
  rm -f etc/resolv.conf && ln -s /run/resolv.conf etc/resolv.conf

  # Void's iputils doesn't set CAP_NET_RAW on ping, so non-root gets 'Operation not permitted'
  setcap cap_net_raw+ep bin/iputils-ping

  # Write build info
  printf '%s\n%s\n' '$GIT_HASH' '$DATETIME' > BUILD
"

# Profile rootfs (before unmount)
echo "Profiling rootfs"
MOUNT_CONTAINER_ID="$MOUNT_CONTAINER_ID" ROOTFS_DIR="$ROOTFS_DIR" \
  ROOTFS_IMAGE="$ROOTFS_IMAGE" OUTPUT_DIR="$OUTPUT_DIR" \
  "$DIR/vamos" profile

# Unmount image
echo "Unmount filesystem"
exec_as_root umount -l "$ROOTFS_DIR"

# Sparsify system image
echo "Sparsifying system image"
exec_as_user img2simg "$ROOTFS_IMAGE" "$OUT_IMAGE"

# Patch sparse image size into profile JSON
SPARSE_SIZE=$(stat -c%s "$OUT_IMAGE" 2>/dev/null || stat -f%z "$OUT_IMAGE")
if command -v jq &>/dev/null; then
  jq --arg s "$SPARSE_SIZE" '.image_size_sparse_bytes = ($s | tonumber)' \
    "$OUTPUT_DIR/rootfs-profile.json" > "$OUTPUT_DIR/rootfs-profile.json.tmp" && \
    mv "$OUTPUT_DIR/rootfs-profile.json.tmp" "$OUTPUT_DIR/rootfs-profile.json"
fi

echo "Done!"
