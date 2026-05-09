FROM debian:bullseye-slim

RUN apt-get update && apt-get install -y \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    make \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth=1 \
    https://github.com/STMicroelectronics/STM32CubeF4.git \
    /opt/STM32CubeF4 && \
    cd /opt/STM32CubeF4 && \
    git submodule update --init --recursive

WORKDIR /workspace
