cmake_minimum_required(VERSION 3.20)

project(FileVault
    VERSION 1.1.0
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
    # Suppress specific warnings that are false positives or intentional
    add_compile_options(
        /wd4101  # unreferenced local variable (catch blocks)
        /wd4244  # conversion from 'type1' to 'type2', possible loss of data
        /wd4267  # conversion from 'size_t' to 'type', possible loss of data  
    )
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC-specific warnings
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-stringop-overread)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang/AppleClang - no stringop-overread warning
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-unused-private-field)
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

# stb is header-only, include from conan-generated config
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/build/Release/generators")
find_package(stb REQUIRED)

if(BUILD_TESTS)
    find_package(Catch2 REQUIRED)
endif()

# Core library sources
set(CORE_SOURCES
    src/core/crypto_engine.cpp
    src/core/types.cpp
    src/core/modes.cpp
    src/core/streaming.cpp
    src/utils/console.cpp
    src/utils/file_io.cpp
    src/utils/crypto_utils.cpp
    src/utils/progress.cpp
    src/utils/table_formatter.cpp
    src/utils/password.cpp
    src/utils/config.cpp
    src/format/file_header.cpp
    src/format/file_format.cpp
)

set(CLI_SOURCES
    src/cli/app.cpp
    src/cli/commands/encrypt_cmd.cpp
    src/cli/commands/decrypt_cmd.cpp
    src/cli/commands/hash_cmd.cpp
    src/cli/commands/list_cmd.cpp
    src/cli/commands/benchmark_cmd.cpp
    src/cli/commands/config_cmd.cpp
    src/cli/commands/info_cmd.cpp
    src/cli/commands/compress_cmd.cpp
    src/cli/commands/decompress_cmd.cpp
    src/cli/commands/stego_cmd.cpp
    src/cli/commands/archive_cmd.cpp
    src/cli/commands/keygen_cmd.cpp
    src/cli/commands/dump_cmd.cpp
    src/cli/commands/sign_cmd.cpp
    src/cli/commands/verify_cmd.cpp
    src/cli/commands/keyinfo_cmd.cpp
)

set(ALGORITHM_SOURCES
    src/algorithms/symmetric/aes_gcm.cpp
    src/algorithms/symmetric/aes_cbc.cpp
    src/algorithms/symmetric/aes_ctr.cpp
    src/algorithms/symmetric/aes_cfb.cpp
    src/algorithms/symmetric/aes_ofb.cpp
    src/algorithms/symmetric/aes_ecb.cpp
    src/algorithms/symmetric/aes_xts.cpp
    src/algorithms/symmetric/triple_des.cpp
    src/algorithms/symmetric/chacha20_poly1305.cpp
    src/algorithms/symmetric/serpent_gcm.cpp
    src/algorithms/symmetric/twofish_gcm.cpp
    src/algorithms/symmetric/camellia_gcm.cpp
    src/algorithms/symmetric/aria_gcm.cpp
    src/algorithms/symmetric/sm4_gcm.cpp
    src/algorithms/asymmetric/rsa.cpp
    src/algorithms/asymmetric/ecc.cpp
    src/algorithms/pqc/post_quantum.cpp
    src/algorithms/classical/caesar.cpp
    src/algorithms/classical/vigenere.cpp
    src/algorithms/classical/playfair.cpp
    src/algorithms/classical/hill.cpp
    src/algorithms/classical/substitution.cpp
)

set(COMPRESSION_SOURCES
    src/compression/compressor.cpp
)

set(STEGANOGRAPHY_SOURCES
    src/steganography/lsb.cpp
)

set(ARCHIVE_SOURCES
    src/archive/archive_format.cpp
)

# Create static library
add_library(filevault_lib STATIC
    ${CORE_SOURCES}
    ${ALGORITHM_SOURCES}
    ${COMPRESSION_SOURCES}
    ${STEGANOGRAPHY_SOURCES}
    ${ARCHIVE_SOURCES}
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
        stb::stb
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
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(nist_test_vectors PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(nist_test_vectors PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    
    # Rainbow Table Protection Test
    add_executable(rainbow_table_test tests/rainbow_table_test.cpp)
    target_link_libraries(rainbow_table_test PRIVATE filevault_lib)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(rainbow_table_test PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(rainbow_table_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
    
    # Unit Tests
    add_executable(test_classical tests/unit/crypto/test_classical.cpp)
    target_link_libraries(test_classical PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_classical PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_classical PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_aes tests/unit/crypto/test_aes.cpp)
    target_link_libraries(test_aes PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_aes PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_aes PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_kdf tests/unit/crypto/test_kdf.cpp)
    target_link_libraries(test_kdf PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_kdf PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_kdf PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_compression tests/unit/compression/test_compression.cpp)
    target_link_libraries(test_compression PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_compression PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_compression PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_chacha20 tests/unit/crypto/test_chacha20.cpp)
    target_link_libraries(test_chacha20 PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_chacha20 PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_chacha20 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_twofish tests/unit/crypto/test_twofish.cpp)
    target_link_libraries(test_twofish PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_twofish PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_twofish PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_international_ciphers tests/unit/crypto/test_international_ciphers.cpp)
    target_link_libraries(test_international_ciphers PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_international_ciphers PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_international_ciphers PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_non_aead_ciphers tests/unit/crypto/test_non_aead_ciphers.cpp)
    target_link_libraries(test_non_aead_ciphers PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_non_aead_ciphers PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_non_aead_ciphers PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_aes_modes tests/unit/crypto/test_aes_modes.cpp)
    target_link_libraries(test_aes_modes PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_aes_modes PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_aes_modes PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_rsa tests/unit/crypto/test_rsa.cpp)
    target_link_libraries(test_rsa PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_rsa PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_rsa PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    # Security Tests
    add_executable(test_nonce_uniqueness tests/security/test_nonce_uniqueness.cpp)
    target_link_libraries(test_nonce_uniqueness PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_nonce_uniqueness PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_nonce_uniqueness PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_salt_uniqueness tests/security/test_salt_uniqueness.cpp)
    target_link_libraries(test_salt_uniqueness PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_salt_uniqueness PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_salt_uniqueness PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    add_executable(test_timing_attacks tests/security/test_timing_attacks.cpp)
    target_link_libraries(test_timing_attacks PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_timing_attacks PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_timing_attacks PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    # Integration Tests
    add_executable(test_encrypt_decrypt_flow tests/integration/test_encrypt_decrypt_flow.cpp)
    target_link_libraries(test_encrypt_decrypt_flow PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_encrypt_decrypt_flow PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_encrypt_decrypt_flow PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    # Hash Tests
    add_executable(test_hash tests/unit/crypto/test_hash.cpp)
    target_link_libraries(test_hash PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_hash PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_hash PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    # Steganography Tests
    add_executable(test_steganography tests/unit/steganography/test_steganography.cpp)
    target_link_libraries(test_steganography PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_steganography PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_steganography PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    # Archive Tests
    add_executable(test_archive tests/unit/archive/test_archive.cpp)
    target_link_libraries(test_archive PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_archive PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_archive PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    # ECC Tests
    add_executable(test_ecc tests/unit/crypto/test_ecc.cpp)
    target_link_libraries(test_ecc PRIVATE filevault_lib Catch2::Catch2WithMain)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_ecc PRIVATE -Wno-error=stringop-overread)
    endif()
    set_target_properties(test_ecc PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    # PQC (Post-Quantum Cryptography) Tests
    add_executable(test_pqc tests/unit/crypto/test_pqc.cpp)
    target_link_libraries(test_pqc PRIVATE filevault_lib Catch2::Catch2WithMain)
    set_target_properties(test_pqc PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/release
    )
    
    enable_testing()
    add_test(NAME NIST_Vectors COMMAND nist_test_vectors)
    add_test(NAME Rainbow_Table COMMAND rainbow_table_test)
    add_test(NAME Classical_Ciphers COMMAND test_classical)
    add_test(NAME AES_GCM COMMAND test_aes)
    add_test(NAME ChaCha20_Poly1305 COMMAND test_chacha20)
    add_test(NAME KDF COMMAND test_kdf)
    add_test(NAME Compression COMMAND test_compression)
    add_test(NAME Integration_Flow COMMAND test_encrypt_decrypt_flow)
    add_test(NAME Security_Nonce_Uniqueness COMMAND test_nonce_uniqueness)
    add_test(NAME Security_Salt_Uniqueness COMMAND test_salt_uniqueness)
    add_test(NAME Security_Timing_Attacks COMMAND test_timing_attacks)
    add_test(NAME Hash_Functions COMMAND test_hash)
    add_test(NAME Steganography COMMAND test_steganography)
    add_test(NAME Archive_Format COMMAND test_archive)
    add_test(NAME Twofish_GCM COMMAND test_twofish)
    add_test(NAME International_Ciphers COMMAND test_international_ciphers)
    add_test(NAME Non_AEAD_Ciphers COMMAND test_non_aead_ciphers)
    add_test(NAME AES_Modes COMMAND test_aes_modes)
    add_test(NAME RSA_Encryption COMMAND test_rsa)
    add_test(NAME ECC_Encryption COMMAND test_ecc)
    add_test(NAME PQC_Encryption COMMAND test_pqc)
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
