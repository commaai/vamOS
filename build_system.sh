#!/usr/bin/env bash
set -e

# Timestamp helper
ts() { echo "[$(date +%H:%M:%S)] $*"; }

# Install pv for transfer monitoring
if ! command -v pv &>/dev/null; then
  sudo apt-get update -qq && sudo apt-get install -y -qq pv
fi

VOID_ROOTFS_URL="https://repo-default.voidlinux.org/live/current/void-aarch64-ROOTFS-20250202.tar.xz"
VOID_ROOTFS_FILE="void-aarch64-ROOTFS-20250202.tar.xz"
VOID_ROOTFS_SHA256="01a30f17ae06d4d5b322cd579ca971bc479e02cc284ec1e5a4255bea6bac3ce6"

# Make sure we're in the correct spot
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd "$DIR"

BUILD_DIR="$DIR/build"
OUTPUT_DIR="$DIR/output"

ROOTFS_DIR="$BUILD_DIR/void-rootfs"
ROOTFS_IMAGE="$BUILD_DIR/system.img"
OUT_IMAGE="$OUTPUT_DIR/system.img"

# the partition is 10G, but openpilot's updater didn't always handle the full size
# Increased from 4500M to 6G for Python packages
ROOTFS_IMAGE_SIZE=6G

# Create temp dir if non-existent
mkdir -p "$BUILD_DIR" "$OUTPUT_DIR"

# Download Void rootfs if not done already
if [ ! -f "$VOID_ROOTFS_FILE" ]; then
  ts "Downloading Void Linux rootfs: $VOID_ROOTFS_FILE"
  if ! curl -C - -o "$VOID_ROOTFS_FILE" "$VOID_ROOTFS_URL" --silent --remote-time --fail; then
    ts "Download failed"
    exit 1
  fi
fi

# Check SHA256 sum
if [ "$(shasum -a 256 "$VOID_ROOTFS_FILE" | awk '{print $1}')" != "$VOID_ROOTFS_SHA256" ]; then
  ts "Checksum mismatch"
  exit 1
fi

# Setup qemu multiarch
if [ "$(uname -m)" = "x86_64" ]; then
  ts "Registering emulator"
  docker run --rm --privileged tonistiigi/binfmt --install all
fi

# Check Dockerfile
export DOCKER_BUILDKIT=1
docker buildx build -f Dockerfile --check "$DIR"

# Setup mount container for macOS and CI support
ts "Building system-builder docker image"
docker build -f Dockerfile.system-builder -t vamos-system-builder "$DIR" \
  --build-arg UNAME="$(id -nu)" \
  --build-arg UID="$(id -u)" \
  --build-arg GID="$(id -g)"

ts "Starting system-builder container"
MOUNT_CONTAINER_ID=$(docker run -d --privileged -v "$DIR:$DIR" vamos-system-builder)

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
ts "Creating empty filesystem"
exec_as_user fallocate -l "$ROOTFS_IMAGE_SIZE" "$ROOTFS_IMAGE"
exec_as_user mkfs.ext4 "$ROOTFS_IMAGE" &> /dev/null

# Mount filesystem
ts "Mounting empty filesystem"
exec_as_root mkdir -p "$ROOTFS_DIR"
exec_as_root mount "$ROOTFS_IMAGE" "$ROOTFS_DIR"

# Also unmount filesystem (overwrite previous trap)
trap "exec_as_root umount -l $ROOTFS_DIR &> /dev/null || true; \
echo \"Cleaning up containers:\"; \
docker container rm -f $MOUNT_CONTAINER_ID" EXIT

# Build and export filesystem directly as tar, pipe into mounted rootfs
# This skips --load (slow "exporting layers" into Docker image store) and docker container export
ts "Building and extracting vamos docker image"
if [ -n "$NS" ]; then
  BUILD="nsc build"
else
  BUILD="docker buildx build"
fi
PLATFORM_FLAG=""
if [ "$(uname -m)" != "aarch64" ]; then
  PLATFORM_FLAG="--platform=linux/arm64"
fi
$BUILD -f Dockerfile $PLATFORM_FLAG \
  --output "type=tar,dest=-" \
  --provenance=false \
  --build-arg VOID_ROOTFS="$VOID_ROOTFS_FILE" \
  "$DIR" | pv -f | docker exec -i "$MOUNT_CONTAINER_ID" tar -xf - -C "$ROOTFS_DIR"
ts "Build and extraction complete"

# Avoid detecting as container
ts "Removing .dockerenv file"
exec_as_root rm -f "$ROOTFS_DIR/.dockerenv"

ts "Setting network stuff"
set_network_stuff() {
  cd "$ROOTFS_DIR"
  # Add hostname and hosts
  HOST=comma
  bash -c "ln -sf /proc/sys/kernel/hostname etc/hostname"
  bash -c "echo \"127.0.0.1    localhost.localdomain localhost\" > etc/hosts"
  bash -c "echo \"127.0.0.1    $HOST\" >> etc/hosts"

  # DNS: resolv.conf must be writable for NetworkManager
  # Docker mounts resolv.conf during build so we do this after export
  bash -c "rm -f etc/resolv.conf && ln -s /run/resolv.conf etc/resolv.conf"

  # Void's iputils doesn't set CAP_NET_RAW on ping, so non-root gets "Operation not permitted"
  bash -c "setcap cap_net_raw+ep bin/iputils-ping"

  # Write build info
  DATETIME=$(date '+%Y-%m-%dT%H:%M:%S')
  bash -c "printf \"$GIT_HASH\n$DATETIME\n\" > BUILD"
}
GIT_HASH=${GIT_HASH:-$(git --git-dir="$DIR/.git" rev-parse HEAD)}
exec_as_root bash -c "set -e; export ROOTFS_DIR=$ROOTFS_DIR GIT_HASH=$GIT_HASH; $(declare -f set_network_stuff); set_network_stuff"

# Unmount image
ts "Unmount filesystem"
exec_as_root umount -l "$ROOTFS_DIR"

# Sparsify system image
ts "Sparsifying system image"
exec_as_user img2simg "$ROOTFS_IMAGE" "$OUT_IMAGE"

ts "Done!"
