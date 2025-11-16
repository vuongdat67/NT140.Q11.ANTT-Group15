# FileVault Benchmarks
# Performance testing for crypto algorithms, compression, and KDF

find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found - benchmarks will not be built")
    message(STATUS "To enable: add 'benchmark/1.8.3' to conanfile.txt")
    return()
endif()

message(STATUS "Building benchmarks with Google Benchmark")

# Crypto benchmarks (AES, modes, etc.)
add_executable(bench_crypto
    bench_crypto.cpp
)

target_link_libraries(bench_crypto
    PRIVATE
        filevault
        benchmark::benchmark
        benchmark::benchmark_main
)

set_target_properties(bench_crypto PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
)

# Compression benchmarks (Zlib, Zstd)
add_executable(bench_compression
    bench_compression.cpp
)

target_link_libraries(bench_compression
    PRIVATE
        filevault
        benchmark::benchmark
        benchmark::benchmark_main
)

set_target_properties(bench_compression PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
)

# KDF benchmarks (Argon2, PBKDF2)
add_executable(bench_kdf
    bench_kdf.cpp
)

target_link_libraries(bench_kdf
    PRIVATE
        filevault
        benchmark::benchmark
        benchmark::benchmark_main
)

set_target_properties(bench_kdf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
)

# Add benchmarks to test suite (optional)
if(BUILD_TESTING)
    add_test(NAME benchmark_crypto 
             COMMAND bench_crypto --benchmark_min_time=0.5s
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks)
    
    add_test(NAME benchmark_compression
             COMMAND bench_compression --benchmark_min_time=0.5s
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks)
    
    add_test(NAME benchmark_kdf
             COMMAND bench_kdf --benchmark_min_time=0.5s
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks)
endif()

message(STATUS "Benchmark targets: bench_crypto, bench_compression, bench_kdf")
