cmake_minimum_required(VERSION 3.9)

project(pi400kb)

add_library(libusbgx INTERFACE)
target_sources(libusbgx INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_common.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_common_libconfig.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_schemes_libconfig.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/usbg_error.c

    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/ether.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/ffs.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/hid.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/loopback.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/midi.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/ms.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/phonet.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/serial.c
    ${CMAKE_CURRENT_LIST_DIR}/libusbgx/src/function/uac2.c
)
target_include_directories(libusbgx INTERFACE ${CMAKE_CURRENT_LIST_DIR}/libusbgx/include)
target_include_directories(libusbgx INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(libusbgx INTERFACE HAS_GADGET_SCHEMES)

set(USBG_VERSION_HEX "0x00000000")
configure_file(${CMAKE_CURRENT_LIST_DIR}/libusbgx/include/usbg/usbg_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/usbg_version.h @ONLY)

find_library(libconfig libconfig)

target_link_libraries(libusbgx INTERFACE config)

add_executable(
    pi400kb
    pi400.c
    gadget-hid.c
)

target_compile_definitions(pi400kb PRIVATE HAS_GADGET_SCHEMES)

target_link_libraries(pi400kb libusbgx)
