# vamOS Void Linux Build
# check=error=true

# ################## #
# ###### Base ###### #
# ################## #
FROM scratch AS void-base

# Add Void Linux aarch64 rootfs
ARG VOID_ROOTFS
ADD ${VOID_ROOTFS} /

# Build folder
RUN mkdir -p /tmp/vamos

ARG USERNAME=comma

# Base system setup
COPY ./userspace/base_setup.sh /tmp/vamos/
RUN /tmp/vamos/base_setup.sh

# ################### #
# ###### vamOS ###### #
# ################### #
FROM void-base

ARG USERNAME=comma

# Build 64-bit irsc_util (replaces 32-bit blob that requires armhf libs)
COPY ./userspace/irsc_util/ /tmp/vamos/irsc_util/
RUN cd /tmp/vamos/irsc_util && \
    gcc -Wall -O2 -o /usr/bin/irsc_util irsc_util.c && \
    rm -rf /tmp/vamos/irsc_util

# libqmi, ModemManager are from Void repos (in base_setup.sh)
# lpac, qt, weston, gcc-arm-none-eabi - removed, now vendored via commaai/dependencies

# Mesa DRI drivers (freedreno for Qualcomm Adreno GPU)
# Required for EGL/GBM to work with the display
RUN yes | xbps-install -Sy mesa-dri

# Extra packages (nice-to-haves for development)
RUN xbps-install -y \
  bash-completion \
  btop \
  hyperfine \
  iperf \
  iperf3 \
  ripgrep \
  ncdu \
  nfs-utils \
  socat \
  stress-ng \
  tree \
  wavemon \
  android-tools \
  avahi-utils \
  usbutils \
  vim

# Python packages via uv
ARG XDG_DATA_HOME="/usr/local"
ENV PATH="/root/.local/bin:$XDG_DATA_HOME/venv/bin:$PATH"
COPY ./userspace/uv /tmp/vamos/uv
RUN cd /tmp/vamos/uv && \
    MAKEFLAGS="-j$(nproc)" UV_CONCURRENT_BUILDS=4 UV_NO_CACHE=1 UV_PROJECT_ENVIRONMENT=$XDG_DATA_HOME/venv \
    uv sync --frozen --inexact

# Root filesystem overlay — all userspace files at their final paths
# Symlinks, service enablement, scripts, and permissions are in the overlay.
# See userspace/root/ for the full filesystem layout.
COPY ./userspace/root/ /

# Fix ownership on home directory (overlay COPY runs as root)
RUN chown -R comma:comma /home/comma

# Remove udevd service - we start udevd in runit stage 1, no need for service
RUN rm -f /etc/runit/runsvdir/default/udevd

# Make logs readable by comma user (for journalctl shim)
RUN sed -i 's/FileCreateMode 0640/FileCreateMode 0644/' /etc/rsyslog.conf

# NetworkManager connection permissions (git can't store 600)
RUN chmod 600 /usr/lib/NetworkManager/system-connections/*.nmconnection

# MOTD - profile.d runs update-motd.d scripts into /run/motd, pam_motd displays it
RUN sed -i 's|motd=/etc/motd|motd=/run/motd|' /etc/pam.d/system-login

# Mount point ownership (directories created by overlay, ownership set here)
RUN chown comma:comma /dsp /bt_firmware /firmware

# Prefer ipv4 over ipv6
RUN echo "precedence ::ffff:0:0/96 100" >> /etc/gai.conf

# Compatibility symlink for prebuilt wheels
# raylib wheel needs libffi.so.6 but Void has newer version
RUN ln -sf $(ls /usr/lib/libffi.so.* 2>/dev/null | grep -v '.so.6' | head -1) /usr/lib/libffi.so.6

# ldconfig
RUN ldconfig

# Read-only rootfs setup
COPY ./userspace/readonly_setup.sh /tmp/vamos/
RUN /tmp/vamos/readonly_setup.sh

# TODO: Remove Qualcomm GPU blobs by rebuilding pyray/raylib against Mesa
# Currently raylib is compiled directly against libEGL_adreno.so/libGLESv2_adreno.so
# To use open-source Mesa freedreno instead:
#   1. Rebuild pyray wheel linking against GLVND (-lEGL -lGLESv2) not Adreno directly
#   2. Remove display blobs (userspace/root/usr/lib/)
#   3. Keep mesa-dri package (provides freedreno via msm_dri.so)
# This would eliminate all proprietary GPU dependencies

# Fix EGL/GBM library symlinks for Adreno GPU
# openpilot accesses eglCreateImageKHR from libEGL.so via cffi, but glvnd doesn't
# export extension functions (must use eglGetProcAddress). Bypass glvnd dispatcher.
# Old: just symlinked to glvnd (libEGL.so.1.1.0 from mesa)
#   ln -sf libEGL.so.1.1.0 /usr/lib/libEGL.so
# New: replace glvnd with libEGL_adreno.so which exports eglCreateImageKHR
# Also replace libwayland-egl with Qualcomm's version (has WlEglWindow* symbols)
RUN cp /usr/lib/libEGL_adreno.so /usr/lib/libEGL.so.1.1.0 && \
    ln -sf libEGL.so.1.1.0 /usr/lib/libEGL.so && \
    ln -sf libEGL.so.1.1.0 /usr/lib/libEGL.so.1 && \
    rm -f /usr/lib/libEGL.so.1.0 /usr/lib/libEGL.so.1.0.0 && \
    rm -f /usr/lib/libwayland-egl.so.1 /usr/lib/libwayland-egl.so.1.* && \
    cp /usr/lib/libwayland-egl-qcom.so /usr/lib/libwayland-egl.so.1 && \
    ln -sf libwayland-egl.so.1 /usr/lib/libwayland-egl.so

# Drop non-English locales, man/doc/info pages, and xbps cache
RUN find /usr/share/locale -maxdepth 1 -mindepth 1 ! -name en_US ! -name en -exec rm -rf {} + && \
    rm -rf /usr/share/man /usr/share/doc /usr/share/info && \
    rm -rf /var/cache/xbps

# ################# #
# #### Cleanup #### #
# ################# #

# DEBUG: allow root login
RUN echo ttyS0 >> /etc/securetty
RUN echo ttyMSM0 >> /etc/securetty

# DEBUG: fix comma password (chpasswd fails under QEMU)
# Use pre-computed hash for reliability: password is "comma"
RUN usermod -p '$6$vamosdev$NmMcideymvdAilEr.m7OeTX.fnt3H4UmcgheKB39rkrmebVfvZIUE17zjgtOxi7K07uGicamzFFbkFLE3EjQj/' comma && \
    usermod -p '$6$vamosdev$NmMcideymvdAilEr.m7OeTX.fnt3H4UmcgheKB39rkrmebVfvZIUE17zjgtOxi7K07uGicamzFFbkFLE3EjQj/' root

RUN ldconfig

# Note: /data mount is handled in runit stage 1 (/etc/runit/1)
