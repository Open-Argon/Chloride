cmake_minimum_required(VERSION 3.21)

project(argon_archive_stdlib C)

find_package(LibArchive REQUIRED)

add_library(archive SHARED
    native/main.c
)

if(NOT DEFINED ENV{ARGON_INCLUDE})
    message(FATAL_ERROR "ARGON_INCLUDE environment variable is not set")
endif()

set(ARGON_INCLUDE "$ENV{ARGON_INCLUDE}")

target_include_directories(archive PRIVATE
    "${ARGON_INCLUDE}"
)

target_link_libraries(archive PRIVATE
    LibArchive::LibArchive
)

target_compile_options(archive PRIVATE
    -fvisibility=hidden
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    -Werror=unused-result
)

if(UNIX AND NOT APPLE)
    target_link_options(archive PRIVATE
        "-Wl,--gc-sections"
        "-Wl,-s"
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/native/exports.map"
    )
endif()

set(ARGON_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/native/bin")

set_target_properties(archive PROPERTIES
    OUTPUT_NAME "archive"
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON

    RUNTIME_OUTPUT_DIRECTORY "${ARGON_BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ARGON_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${ARGON_BIN_DIR}"
)