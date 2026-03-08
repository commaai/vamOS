FROM ubuntu:24.04

ARG UNAME
ARG UID
ARG GID

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    libssl-dev \
    bc \
    flex \
    bison \
    libelf-dev \
    python3 \
    openssl \
    ccache \
    && rm -rf /var/lib/apt/lists/*

RUN if [ ${UID:-0} -ne 0 ] && [ ${GID:-0} -ne 0 ]; then \
    userdel -r `getent passwd ${UID} | cut -d : -f 1` > /dev/null 2>&1; \
    groupdel -f `getent group ${GID} | cut -d : -f 1` > /dev/null 2>&1; \
    groupadd -g ${GID} -o ${UNAME} && \
    useradd -u $UID -g $GID ${UNAME} \
;fi

RUN ln -s $(which ccache) /usr/local/bin/gcc && \
    ln -s $(which ccache) /usr/local/bin/g++ && \
    ln -s $(which ccache) /usr/local/bin/cc && \
    ln -s $(which ccache) /usr/local/bin/c++

ENTRYPOINT ["tail", "-f", "/dev/null"]
