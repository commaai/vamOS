FROM ghcr.io/void-linux/void-glibc-full

RUN xbps-install -yS

RUN xbps-install -y \
    base-devel \
    bash \
    ccache \
    git \
    openssl-devel \
    python3 && \
    xbps-remove -O

ENTRYPOINT ["tail", "-f", "/dev/null"]
