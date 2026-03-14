# check=error=true

FROM alpine:3.23.3

ARG UNAME
ARG UID
ARG GID

RUN apk add --no-cache \
    android-tools \
    bash \
    bc \
    build-base \
    ccache \
    e2fsprogs \
    git \
    libcap \
    linux-headers \
    openssl \
    openssl-dev \
    python3

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
