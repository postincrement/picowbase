#!/bin/bash
set -e

IMAGE_NAME="picowbase"

# Print build environment info
echo "Build environment:"
echo "SDK Path: $PICO_SDK_PATH"
echo "Board: $PICO_BOARD"
echo "Compiler: $(arm-none-eabi-gcc --version | head -n1)"

# Create build directory
mkdir -p build
cd build

# Configure CMake
cmake -DPICO_SDK_PATH=/opt/pico-sdk ..

# Build the project
make -j4

# Generate additional output files
arm-none-eabi-objcopy -Oihex ${IMAGE_NAME}.elf ${IMAGE_NAME}.hex
arm-none-eabi-objcopy -Obinary ${IMAGE_NAME}.elf ${IMAGE_NAME}.bin

echo "Build completed successfully!"
echo "Output files are available in the build directory:"
echo "  - ${IMAGE_NAME}.elf"
echo "  - ${IMAGE_NAME}.hex"
echo "  - ${IMAGE_NAME}.bin"
echo "  - ${IMAGE_NAME}.uf2"
echo ""
echo "To program the device, use picotool from your host system:"
echo "  picotool load ${IMAGE_NAME}.elf" 