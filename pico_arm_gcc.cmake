# This is a modified version of the Pico SDK's pico_arm_gcc.cmake
# Original file is at /opt/pico-sdk/cmake/pico_arm_gcc.cmake

# this one is important
set(CMAKE_SYSTEM_NAME Generic)
#this one not so much
set(CMAKE_SYSTEM_PROCESSOR arm)

# specify the cross compilers
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-as)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set compiler flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m0plus -mthumb -specs=nosys.specs -specs=nano.specs")
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m0plus -mthumb -specs=nosys.specs -specs=nano.specs")
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m0plus -mthumb")

# Override the default assembler command to use only -I flags
set(CMAKE_ASM_COMPILE_OBJECT
    "<CMAKE_ASM_COMPILER> -mcpu=cortex-m0plus -mthumb -I${PICO_SDK_PATH}/src/rp2040/boot_stage2/asminclude -o <OBJECT> <SOURCE>")

# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m0plus -mthumb -specs=nosys.specs -specs=nano.specs -T${PICO_SDK_PATH}/src/rp2040/pico_standard_link/memmap_default.ld") 