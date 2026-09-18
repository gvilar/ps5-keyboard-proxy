cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD waveshare_rp2040_zero)

if(NOT DEFINED ENV{PICO_SDK_PATH})
message(FATAL_ERROR
"PICO_SDK_PATH is not set. "
"Example: export PICO_SDK_PATH=/path/to/pico-sdk"
)
endif()

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(rp2040_uart_test C CXX ASM)

set(TINYUSB_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/tusb_config.h")

pico_sdk_init()

add_executable(rp2040_uart_test
main.c
usb_descriptors.c
)

target_include_directories(rp2040_uart_test PUBLIC
${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(rp2040_uart_test
pico_stdlib
hardware_uart
tinyusb_device
tinyusb_board
)

pico_enable_stdio_usb(rp2040_uart_test 0)
pico_enable_stdio_uart(rp2040_uart_test 0)

pico_add_extra_outputs(rp2040_uart_test)

