FROM debian:bullseye

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    wget \
    sudo \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libusb-1.0-0-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone Pico SDK
RUN git clone https://github.com/raspberrypi/pico-sdk.git /opt/pico-sdk \
    && cd /opt/pico-sdk \
    && git submodule update --init

# Set up Pico SDK environment
ENV PICO_SDK_PATH=/opt/pico-sdk

# Create build directory
WORKDIR /build

# Copy build files
COPY CMakeLists.txt .
COPY main.cpp .
COPY pico_sdk_import.cmake .
COPY pico_arm_gcc.cmake .
COPY lwipopts.h .
COPY build.sh .

# Make build script executable
RUN chmod +x build.sh

# Set the entrypoint to keep the container running
ENTRYPOINT ["/bin/bash", "-c", "while true; do sleep 1; done"] 