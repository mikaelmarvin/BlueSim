FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /workspace

RUN apt-get update && apt-get install -y \
    ca-certificates \
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
    pip3 install --no-cache-dir west pyelftools intelhex

# nRF Command Line Tools (nrfjprog): Zephyr nrf52840dk default flash-runner is nrfjprog before nrfutil.
# Tarball bundles SEGGER J-Link shared libs; unpack JLink_*.tgz so nrfjprog can load libjlinkarm.so.
ARG NRFCLT_URL=https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-24-2/nrf-command-line-tools-10.24.2_linux-amd64.tar.gz
ENV NRF5_TOOLS_ROOT=/opt/nrf5-tools
RUN mkdir -p "${NRF5_TOOLS_ROOT}" && cd "${NRF5_TOOLS_ROOT}" \
    && wget -q "${NRFCLT_URL}" -O nrfclt.tar.gz \
    && tar xzf nrfclt.tar.gz \
    && rm nrfclt.tar.gz \
    && tar xzf JLink_Linux_*.tgz \
    && rm JLink_Linux_*.tgz \
    && JLINK_DIR="$(ls -d "${NRF5_TOOLS_ROOT}"/JLink_Linux_*_x86_64 | head -1)" \
    && ln -sfn "${JLINK_DIR}" "${NRF5_TOOLS_ROOT}/jlink-extract" \
    && rm -rf "${NRF5_TOOLS_ROOT}/nrf-command-line-tools/python"
ENV PATH=${NRF5_TOOLS_ROOT}/nrf-command-line-tools/bin:${NRF5_TOOLS_ROOT}/jlink-extract:${PATH}
ENV LD_LIBRARY_PATH=${NRF5_TOOLS_ROOT}/nrf-command-line-tools/lib:${NRF5_TOOLS_ROOT}/jlink-extract

RUN curl -sS https://starship.rs/install.sh | sh -s -- -y -b /usr/local/bin

# Starship init for ~/.bashrc is added by .devcontainer/setup.sh (postCreate / post-start).

RUN useradd -m -s /bin/bash developer && \
    usermod -aG dialout developer && \
    chown -R developer:developer /workspace

USER developer

CMD ["/bin/bash"]
