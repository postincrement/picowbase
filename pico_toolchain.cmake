set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compilers
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-as)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Basic flags for all tools
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m0plus -mthumb")
set(CMAKE_C_FLAGS "-mcpu=cortex-m0plus -mthumb -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# Include paths for all tools
set(INCLUDE_PATHS
    ${PICO_SDK_PATH}/src/rp2040/boot_stage2/asminclude
    ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_base/include
    ${PICO_SDK_PATH}/src/common/pico_base_headers/include
    ${PICO_SDK_PATH}/src/boards/include
    ${PICO_SDK_PATH}/src/rp2040/pico_platform/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_platform_compiler/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_platform_panic/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_platform_sections/include
    ${PICO_SDK_PATH}/src/rp2040/boot_stage2/include
)

# Add include paths to flags
foreach(PATH ${INCLUDE_PATHS})
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -I${PATH}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${PATH}")
endforeach()

# Override the default assembler command to use only -I flags
set(CMAKE_ASM_COMPILE_OBJECT
    "<CMAKE_ASM_COMPILER> <DEFINES> <FLAGS> -o <OBJECT> <SOURCE>") 