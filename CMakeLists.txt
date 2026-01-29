cmake_minimum_required(VERSION 3.13)

set(NAME youme-transformer)

set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2)

# Set the default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Specify the compiler to use
if(DEFINED ENV{PICO_TOOLCHAIN_PATH})
    set(CMAKE_C_COMPILER "$ENV{PICO_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc")
    set(CMAKE_CXX_COMPILER "$ENV{PICO_TOOLCHAIN_PATH}/bin/arm-none-eabi-g++")
endif()

# Set up the Pico SDK
set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Compiler settings
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/midi_uart_lib)

set(PICO_TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb)

set(SOURCE_FILES
    src/passthrough.cpp
    src/usb_descriptors.c
)

add_executable(${NAME}
    ${SOURCE_FILES}
)

# Be pedantic about warnings.
set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "-Wextra;-Wpedantic")

target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(${NAME}
    pico_stdlib
    ring_buffer_lib
    midi_uart_lib
    tinyusb_device
)

target_compile_definitions(${NAME} PRIVATE)

pico_add_extra_outputs(${NAME})

# By default, UART0 is reserved for stdio.
# It needs to be disabled for YouMeTransformer
# v1.0 hardware, which uses UART0 for MIDI.
pico_enable_stdio_uart(${NAME} 0)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
