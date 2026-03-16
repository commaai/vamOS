# check=error=true

FROM ghcr.io/void-linux/void-glibc-full:latest

ARG UNAME
ARG UID
ARG GID

RUN mkdir -p /etc/xbps.d && \
    cp /usr/share/xbps.d/*-repository-*.conf /etc/xbps.d/ && \
    sed -i 's|https://repo-default.voidlinux.org|https://mirrors.cicku.me/voidlinux|g' /etc/xbps.d/*-repository-*.conf

RUN xbps-install -Sy && \
    xbps-install -y \
    android-tools \
    base-devel \
    bash \
    ccache \
    e2fsprogs \
    git \
    kmod \
    libcap-progs \
    openssl-devel \
    python3 && \
    xbps-remove -O

# Cross-compiler for x86_64 hosts building aarch64 kernel
RUN if [ "$(uname -m)" != "aarch64" ]; then \
        xbps-install -y cross-aarch64-linux-gnu && \
        xbps-remove -O; \
    fi

RUN if [ ${UID:-0} -ne 0 ] && [ ${GID:-0} -ne 0 ]; then \
        userdel $(getent passwd ${UID} | cut -d : -f 1)  > /dev/null 2>&1; \
        groupdel $(getent group ${GID} | cut -d : -f 1)  > /dev/null 2>&1; \
        groupadd -g ${GID} ${UNAME} && \
        useradd -u ${UID} -g ${GID} ${UNAME}; \
    fi

ENTRYPOINT ["tail", "-f", "/dev/null"]
