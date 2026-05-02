FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /workspace

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    gperf \
    ccache \
    dfu-util \
    device-tree-compiler \
    wget \
    xz-utils \
    file \
    make \
    gcc \
    g++ \
    gcc-arm-none-eabi \
    libsdl2-dev \
    libmagic1 \
    git \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    python3-venv \
    libusb-1.0-0 \
    openocd \
    gdb-multiarch \
    clangd \
    curl \
    usbutils \
    vim \
    nano \
    && rm -rf /var/lib/apt/lists/*

# Zephyr 3.5.x (NCS 2.6) expects Zephyr SDK >= 0.16 for host tools + CMake package (not the same as Ubuntu gcc-arm-none-eabi).
ARG ZEPHYR_SDK_VERSION=0.16.8
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-${ZEPHYR_SDK_VERSION}
RUN wget -q "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz" \
    && tar xf "zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz" -C /opt \
    && rm "zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz" \
    && bash "${ZEPHYR_SDK_INSTALL_DIR}/setup.sh" -c -h -t arm-zephyr-eabi

RUN pip3 install --no-cache-dir --upgrade pip && \
    pip3 install --no-cache-dir west pyelftools

RUN curl -sS https://starship.rs/install.sh | sh -s -- -y -b /usr/local/bin

RUN echo '' >> /etc/skel/.bashrc \
    && echo '# Starship prompt (BlueSim devcontainer image)' >> /etc/skel/.bashrc \
    && echo '[ -n "$BASH_VERSION" ] && command -v starship >/dev/null 2>&1 && eval "$(starship init bash)"' >> /etc/skel/.bashrc

RUN useradd -m -s /bin/bash developer && \
    usermod -aG dialout developer && \
    mkdir -p /opt/ncs && \
    chown -R developer:developer /opt/ncs && \
    chown -R developer:developer /workspace

USER developer
CMD ["/bin/bash"]
