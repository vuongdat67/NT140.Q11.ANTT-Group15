cmake_minimum_required(VERSION 3.20)
project(FileVault VERSION 1.0.0 LANGUAGES CXX)

# C++20 required (for Botan 3.x)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Conan packages
find_package(botan REQUIRED)
find_package(CLI11 REQUIRED)
find_package(spdlog REQUIRED)
find_package(indicators REQUIRED)
find_package(Catch2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(fmt REQUIRED)

# Compiler warnings and MSVC runtime
if(MSVC)
    add_compile_options(/W4)
    # Use MultiThreaded runtime for static linking
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Sanitizers for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# Library
add_subdirectory(src)

# CLI executable
add_subdirectory(cli)

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS filevault_cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/filevault
    DESTINATION include
)
