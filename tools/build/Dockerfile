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

# Install firmware from upstream linux-firmware (ath10k WiFi + modem + Bluetooth)
COPY ./kernel/firmware/ath10k/WCN3990/hw1.0 /lib/firmware/ath10k/WCN3990/hw1.0
COPY ./kernel/firmware/qcom/sdm845 /lib/firmware/qcom/sdm845
COPY ./kernel/firmware/qca /lib/firmware/qca
RUN ln -sf /lib/firmware/ath10k/WCN3990/hw1.0/wlanmdsp.mbn /lib/firmware/qcom/sdm845/wlanmdsp.mbn

# Root filesystem overlay — all userspace files at their final paths
COPY ./userspace/root/ /

# Fix ownership on home directory (overlay COPY runs as root)
RUN chown -R comma:comma /home/comma
# Void's default .bashrc doesn't source .bash_aliases
RUN printf '\nif [ -f ~/.bash_aliases ]; then\n    . ~/.bash_aliases\nfi\n' >> /home/$USERNAME/.bashrc
RUN mkdir -p /root/.config

# Enable runit services
RUN for svc in \
  ipa_fws brightnessd fs_setup init-qcom lte \
  avahi-ssh-publish power_monitor power_drop_monitor \
  screen_calibration serial-hostname \
  sound varwatch busybox-ntpd \
  irsc_util leprop adsp cdsp \
  qrtr-ns rmtfs tqftpserv pd-mapper \
  comma modemmanager bluetoothd magic \
  ssh-param-watcher ncm-param-watcher dnsmasq \
  rsyslogd iptables polkitd \
  agetty-ttyMSM0 agetty-ttyAMA0; do \
    ln -sf /etc/sv/$svc /etc/runit/runsvdir/default/; \
  done

# Remove udevd service - we start udevd in runit stage 1, no need for service
RUN rm -f /etc/runit/runsvdir/default/udevd

# Make logs readable by comma user (for journalctl shim)
RUN sed -i 's/FileCreateMode 0640/FileCreateMode 0644/' /etc/rsyslog.conf

# Set executable permissions
RUN chmod +x /usr/comma/comma.sh /usr/comma/get-serial.sh /usr/comma/get-bt-address.sh /usr/comma/set-bt-address.sh /usr/comma/set_ssh.sh /usr/comma/set_ncm.sh /usr/comma/fs_setup.sh /usr/bin/journalctl /etc/sv/bluetoothd/run
RUN chmod +x /etc/runit/1 /etc/runit/2 /etc/runit/3

# Shutdown wrappers - send wall broadcast like systemd does on Ubuntu
# Only poweroff and reboot: halt/shutdown must NOT be wrapped because Void's
# /usr/bin/shutdown script calls halt/reboot internally, causing cascading walls
RUN chmod +x /usr/local/sbin/poweroff && \
    ln -sf poweroff /usr/local/sbin/reboot

# NetworkManager connection permissions
RUN chmod 600 /usr/lib/NetworkManager/system-connections/*.nmconnection

# Void's iptables service expects iptables.rules
RUN ln -s /etc/iptables/rules.v4 /etc/iptables/iptables.rules

# MOTD - profile.d runs update-motd.d scripts into /run/motd, pam_motd displays it
RUN sed -i 's|motd=/etc/motd|motd=/run/motd|' /etc/pam.d/system-login

# Hourly logrotate cron job
RUN chmod 755 /etc/cron.hourly/logrotate

# Mount point directories used by runtime partition mounts
RUN mkdir -p /dsp /firmware && \
    chown comma:comma /dsp /firmware

# /system/persist structure (copied to /data on first boot by fs_setup.sh)
RUN mkdir -p /system/persist/hlos_rfs/shared \
             /system/persist/rfs/mdm/adsp \
             /system/persist/rfs/mdm/mpss \
             /system/persist/rfs/msm/adsp \
             /system/persist/rfs/msm/mpss \
             /system/persist/rfs/shared

# Prefer ipv4 over ipv6
RUN echo "precedence ::ffff:0:0/96 100" >> /etc/gai.conf

# Compatibility symlinks for prebuilt wheels
# raylib wheel needs libffi.so.6 but Void has newer version
RUN ln -sf $(ls /usr/lib/libffi.so.* 2>/dev/null | grep -v '.so.6' | head -1) /usr/lib/libffi.so.6

# ldconfig
RUN ldconfig

# Rootfs development/eval tools
COPY ./userspace/eval/rootfs-init.sh /usr/local/bin/
COPY ./userspace/eval/rootfs-manifest.sh /usr/local/bin/
COPY ./userspace/eval/rootfs-diff.sh /usr/local/bin/
COPY ./userspace/eval/rootfs-export.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/rootfs-*.sh

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

# DEBUG: SSH setup - sshd_config expects keys in /data/etc/ssh/
# The /data partition is mounted at runtime from /dev/sda12
# We need to create the ssh directory structure on first boot
RUN mkdir -p /var/chroot/ssh

# DEBUG: Custom sshd run script - /data is mounted in runit stage 1
RUN echo '#!/bin/sh' > /etc/sv/sshd/run && \
    echo 'exec 2>&1' >> /etc/sv/sshd/run && \
    echo '# Create ssh host keys if needed (on first boot)' >> /etc/sv/sshd/run && \
    echo 'mkdir -p /data/etc/ssh' >> /etc/sv/sshd/run && \
    echo 'if [ ! -f /data/etc/ssh/ssh_host_ed25519_key ]; then' >> /etc/sv/sshd/run && \
    echo '  ssh-keygen -t ed25519 -f /data/etc/ssh/ssh_host_ed25519_key -N ""' >> /etc/sv/sshd/run && \
    echo '  ssh-keygen -t rsa -b 4096 -f /data/etc/ssh/ssh_host_rsa_key -N ""' >> /etc/sv/sshd/run && \
    echo '  ssh-keygen -t ecdsa -f /data/etc/ssh/ssh_host_ecdsa_key -N ""' >> /etc/sv/sshd/run && \
    echo 'fi' >> /etc/sv/sshd/run && \
    echo 'exec /usr/bin/sshd -D' >> /etc/sv/sshd/run

# DEBUG: Fix iptables to allow SSH (insert before wwan0 DROP rule)
RUN sed -i '/-A INPUT -i wwan0 -j DROP/i -A INPUT -p tcp --dport 22 -j ACCEPT' /etc/iptables/rules.v4 || true

RUN ldconfig

# Note: /data mount is handled in runit stage 1 (/etc/runit/1)
