FROM ghcr.io/void-linux/void-glibc-full

RUN xbps-install -yS

RUN xbps-install -y \
    base-devel \
    bash \
    ccache \
    git \
    openssl-devel \
    python3 && \
    if [ "$(uname -m)" != "aarch64" ]; then xbps-install -y cross-aarch64-linux-gnu; fi && \
    xbps-remove -O

ENTRYPOINT ["tail", "-f", "/dev/null"]
