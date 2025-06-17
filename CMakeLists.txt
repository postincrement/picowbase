cmake_minimum_required(VERSION 3.13)

# Initialize the SDK
set(PICO_SDK_PATH "/opt/pico-sdk")
set(PICO_BOARD pico_w)
set(PICO_NO_PICOTOOL 1)
include(pico_sdk_import.cmake)

project(picowbase C CXX ASM)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Initialize the SDK
pico_sdk_init()

# Add the executable
add_executable(picowbase main.cpp)

# Add include directories
target_include_directories(picowbase PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(picowbase 1)
pico_enable_stdio_uart(picowbase 0)

# Add the standard library to the build
target_link_libraries(picowbase pico_stdlib)

# Add WiFi libraries
target_link_libraries(picowbase
    pico_cyw43_arch_poll
    hardware_gpio
    hardware_timer
    pico_lwip_arch
    pico_lwip_nosys
    pico_multicore
)

# Enable quick boot and validation for programming without disconnecting USB
target_compile_definitions(picowbase PRIVATE
    PICO_BOOTLOADER_DRIVER_QUICK_BOOT=1
    PICO_BOOTLOADER_DRIVER_QUICK_BOOT_VALIDATE=1
    PICO_BOOTLOADER_DRIVER_QUICK_BOOT_VALIDATE_CRC=1
    PICO_BOOTLOADER_DRIVER_QUICK_BOOT_VALIDATE_CRC_FAST=1
)

# Generate UF2 file directly
add_custom_command(TARGET picowbase POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/picowbase.elf
        ${CMAKE_BINARY_DIR}/picowbase.uf2
) 
