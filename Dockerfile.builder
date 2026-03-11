FROM ubuntu:24.04

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
    git \
    && rm -rf /var/lib/apt/lists/*

RUN ln -s $(which ccache) /usr/local/bin/gcc && \
    ln -s $(which ccache) /usr/local/bin/g++ && \
    ln -s $(which ccache) /usr/local/bin/cc && \
    ln -s $(which ccache) /usr/local/bin/c++

ENTRYPOINT ["tail", "-f", "/dev/null"]
