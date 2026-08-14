cmake_minimum_required(VERSION 3.21)

project(argon_network_stdlib C)

find_package(OpenSSL REQUIRED)

file(GLOB NETWORK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/native/*.c"
)

add_library(network SHARED
    ${NETWORK_SOURCES}
)

if(NOT DEFINED ENV{ARGON_INCLUDE})
    message(FATAL_ERROR "ARGON_INCLUDE environment variable is not set")
endif()

set(ARGON_INCLUDE "$ENV{ARGON_INCLUDE}")

target_include_directories(network PRIVATE
    "${ARGON_INCLUDE}"
)

target_link_libraries(network PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_compile_options(network PRIVATE
    -fvisibility=hidden
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    -Werror=unused-result
)

if(UNIX AND NOT APPLE)
    target_link_options(network PRIVATE
        "-Wl,--gc-sections"
        "-Wl,-s"
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/native/exports.map"
    )
endif()

set(ARGON_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/native/bin")

set_target_properties(network PROPERTIES
    OUTPUT_NAME "network"
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON

    RUNTIME_OUTPUT_DIRECTORY "${ARGON_BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ARGON_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${ARGON_BIN_DIR}"
)