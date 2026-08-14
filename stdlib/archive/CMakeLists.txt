cmake_minimum_required(VERSION 3.21)

project(argon_archive_stdlib C)

# MinGW compatibility: libxml2's Conan metadata asks for "Bcrypt",
# while MinGW provides "bcrypt".
if(MINGW)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-file-name=libbcrypt.a
        OUTPUT_VARIABLE BCRYPT_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE BCRYPT_RESULT
    )

    if(NOT BCRYPT_RESULT EQUAL 0 OR NOT EXISTS "${BCRYPT_PATH}")
        message(FATAL_ERROR "Could not locate MinGW libbcrypt.a")
    endif()

    set(MINGW_COMPAT_DIR "${CMAKE_CURRENT_BINARY_DIR}/mingw-compat")

    file(MAKE_DIRECTORY "${MINGW_COMPAT_DIR}")

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                "${BCRYPT_PATH}"
                "${MINGW_COMPAT_DIR}/libBcrypt.a"
    )
endif()

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

if(MINGW)
    target_link_directories(archive PRIVATE
        "${MINGW_COMPAT_DIR}"
    )
endif()

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