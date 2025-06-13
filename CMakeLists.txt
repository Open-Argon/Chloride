cmake_minimum_required(VERSION 3.16)
project(argon C)

# Conan will define FLEX_EXECUTABLE via an environment variable or toolchain
# You should set it in CMakeLists.txt only if it's not set
if(NOT DEFINED FLEX_EXECUTABLE)
    find_program(FLEX_EXECUTABLE flex REQUIRED)
endif()

# Set source and output files
set(LEXER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/lex.l)
set(LEXER_C ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/lex.yy.c)
set(LEXER_H ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer/lex.yy.h)

# Gather all C files EXCEPT the generated lex.yy.c
file(GLOB_RECURSE CFILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)
list(REMOVE_ITEM CFILES ${LEXER_C})  # remove if globbed

# Step 1: Custom command to generate lexer
add_custom_command(
    OUTPUT ${LEXER_C} ${LEXER_H}
    COMMAND ${FLEX_EXECUTABLE} --header-file=${LEXER_H} -o ${LEXER_C} ${LEXER_SRC}
    DEPENDS ${LEXER_SRC}
    COMMENT "Generating lexer with flex"
)

# Step 2: Custom target for lexer
add_custom_target(GenerateLexer DEPENDS ${LEXER_C} ${LEXER_H})

# Step 3: Add executable
add_executable(argon ${CFILES} ${LEXER_C})

# Step 4: Build order
add_dependencies(argon GenerateLexer)

# Step 5: Output path
set_target_properties(argon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/bin
)

# Step 6: Conan libraries
find_package(BDWgc REQUIRED)
find_package(cJSON REQUIRED)
find_package(gmp REQUIRED)

target_compile_options(argon PRIVATE -O3 -Wall -Wextra -Wno-unused-function -s)
target_link_options(argon PRIVATE -static)

target_link_libraries(argon PRIVATE
    BDWgc::BDWgc
    cjson::cjson
    gmp::gmp
    m
)

target_include_directories(argon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer
)