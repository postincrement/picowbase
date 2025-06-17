#!/bin/bash
set -e

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
arm-none-eabi-objcopy -Oihex pico_w_blink.elf pico_w_blink.hex
arm-none-eabi-objcopy -Obinary pico_w_blink.elf pico_w_blink.bin

echo "Build completed successfully!"
echo "Output files are available in the build directory:"
echo "  - pico_w_blink.elf"
echo "  - pico_w_blink.hex"
echo "  - pico_w_blink.bin"
echo "  - pico_w_blink.uf2"
echo ""
echo "To program the device, use picotool from your host system:"
echo "  picotool load pico_w_blink.uf2" 