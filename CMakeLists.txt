cmake_minimum_required(VERSION 3.20)

project(FileVault
    VERSION 1.0.0
    DESCRIPTION "Professional File Encryption CLI Tool"
    LANGUAGES CXX
)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Output directories - organized structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Separate directories for different build types
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/release)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/include)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Optimization for Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Find dependencies (via Conan)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(CLI11 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(botan REQUIRED)
find_package(ZLIB REQUIRED)
find_package(bzip3 REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(indicators REQUIRED)
find_package(tabulate REQUIRED)

# Core library sources
set(CORE_SOURCES
    src/core/crypto_engine.cpp
    src/core/types.cpp
    src/utils/console.cpp
    src/utils/file_io.cpp
    src/utils/crypto_utils.cpp
    src/utils/progress.cpp
    src/utils/table_formatter.cpp
    src/utils/password.cpp
    src/format/file_header.cpp
)

set(CLI_SOURCES
    src/cli/app.cpp
    src/cli/commands/encrypt_cmd.cpp
    src/cli/commands/decrypt_cmd.cpp
    src/cli/commands/hash_cmd.cpp
    src/cli/commands/list_cmd.cpp
    src/cli/commands/benchmark_cmd.cpp
)

set(ALGORITHM_SOURCES
    src/algorithms/symmetric/aes_gcm.cpp
    src/algorithms/classical/caesar.cpp
    src/algorithms/classical/vigenere.cpp
    src/algorithms/classical/playfair.cpp
)

set(COMPRESSION_SOURCES
    src/compression/compressor.cpp
)

# Create static library
add_library(filevault_lib STATIC
    ${CORE_SOURCES}
    ${ALGORITHM_SOURCES}
    ${COMPRESSION_SOURCES}
)

target_include_directories(filevault_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(filevault_lib
    PUBLIC
        fmt::fmt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        botan::botan
        ZLIB::ZLIB
        bzip3::bzip3
        LibLZMA::LibLZMA
        indicators::indicators
        tabulate::tabulate
)

# Main executable
add_executable(filevault
    src/main.cpp
    ${CLI_SOURCES}
)

target_link_libraries(filevault
    PRIVATE
        filevault_lib
        CLI11::CLI11
        indicators::indicators
        tabulate::tabulate
)

# Tests - output to tests/ directory
if(BUILD_TESTS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_TEST ${CMAKE_BINARY_DIR}/tests)
    
    # NIST Test Vectors
    add_executable(nist_test_vectors tests/nist_test_vectors.cpp)
    target_link_libraries(nist_test_vectors PRIVATE filevault_lib)
    target_compile_options(nist_test_vectors PRIVATE -Wno-error=stringop-overread)
    set_target_properties(nist_test_vectors PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    
    # Rainbow Table Protection Test
    add_executable(rainbow_table_test tests/rainbow_table_test.cpp)
    target_link_libraries(rainbow_table_test PRIVATE filevault_lib)
    target_compile_options(rainbow_table_test PRIVATE -Wno-error=stringop-overread)
    set_target_properties(rainbow_table_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    
    enable_testing()
    add_test(NAME NIST_Vectors COMMAND nist_test_vectors)
    add_test(NAME Rainbow_Table COMMAND rainbow_table_test)
endif()

# Benchmarks - output to benchmarks/ directory
if(BUILD_BENCHMARKS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_BENCH ${CMAKE_BINARY_DIR}/benchmarks)
    # Benchmark targets will be added here
endif()

# Installation
install(TARGETS filevault
    RUNTIME DESTINATION bin
)

install(TARGETS filevault_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/filevault
    DESTINATION include
)
