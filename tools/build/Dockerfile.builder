# check=error=true

FROM alpine:3.23.3

ARG UNAME
ARG UID
ARG GID

RUN apk add --no-cache \
    android-tools \
    bash \
    bc \
    bison \
    build-base \
    ccache \
    e2fsprogs \
    findutils \
    flex \
    git \
    jq \
    libcap \
    linux-headers \
    lz4-dev \
    openssl \
    openssl-dev \
    perl \
    python3 \
    util-linux-dev \
    xz-dev

# Build erofs-utils from source (not packaged in Alpine)
RUN git clone https://git.kernel.org/pub/scm/linux/kernel/git/xiang/erofs-utils.git /tmp/erofs-utils \
    && cd /tmp/erofs-utils \
    && git checkout v1.8.5 \
    && apk add --no-cache autoconf automake libtool \
    && autoreconf -fi \
    && ./configure --enable-lz4 --enable-lzma --disable-fuse --enable-multithreading \
    && make -j$(nproc) \
    && make install \
    && rm -rf /tmp/erofs-utils

# Cross-compiler for x86_64 hosts building aarch64 kernel
# gcc-aarch64-none-elf is bare-metal but works for kernel (freestanding code)
RUN if [ "$(uname -m)" != "aarch64" ]; then \
    apk add --no-cache gcc-aarch64-none-elf binutils-aarch64-none-elf; \
    fi

RUN if [ ${UID:-0} -ne 0 ] && [ ${GID:-0} -ne 0 ]; then \
    deluser $(getent passwd ${UID} | cut -d : -f 1) > /dev/null 2>&1; \
    delgroup $(getent group ${GID} | cut -d : -f 1) > /dev/null 2>&1; \
    addgroup -g ${GID} ${UNAME} && \
    adduser -u ${UID} -G ${UNAME} -D ${UNAME} \
;fi

ENTRYPOINT ["tail", "-f", "/dev/null"]
