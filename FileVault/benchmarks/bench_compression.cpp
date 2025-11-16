/**
 * FileVault - Compression Benchmarks
 * 
 * Tests compression/decompression performance for:
 * - Zlib (levels 1, 6, 9)
 * - Zstd (levels 1, 3, 9) - if available
 * - Different data types (text, binary, random)
 * 
 * Usage:
 *   bench_compression
 *   bench_compression --benchmark_filter=Zlib
 *   bench_compression --benchmark_out=compression_results.json
 */

#include <benchmark/benchmark.h>
#include "filevault/compression/compressor.hpp"
#include <botan/auto_rng.h>

using namespace filevault::compression;
using namespace filevault;

// ============================================================================
// Test Data Generators
// ============================================================================

// Repetitive text (highly compressible)
Bytes generate_text_data(size_t size) {
    const std::string pattern = "The quick brown fox jumps over the lazy dog. ";
    Bytes data;
    data.reserve(size);
    
    while (data.size() < size) {
        data.insert(data.end(), pattern.begin(), pattern.end());
    }
    
    data.resize(size);
    return data;
}

// Random data (incompressible)
Bytes generate_random_data(size_t size) {
    Botan::AutoSeeded_RNG rng;
    Bytes data(size);
    rng.randomize(data.data(), data.size());
    return data;
}

// ============================================================================
// Zlib Compression Benchmarks
// ============================================================================

static void BM_Zlib_Compress_Text_Level6(benchmark::State& state) {
    ZlibCompressor compressor(6);  // Default level
    auto input = generate_text_data(state.range(0));
    
    for (auto _ : state) {
        auto compressed = compressor.compress(input);
        benchmark::DoNotOptimize(compressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
    state.SetLabel("Zlib-6 (text)");
}
BENCHMARK(BM_Zlib_Compress_Text_Level6)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

static void BM_Zlib_Decompress_Text_Level6(benchmark::State& state) {
    ZlibCompressor compressor(6);
    auto input = generate_text_data(state.range(0));
    auto compressed = compressor.compress(input);
    
    for (auto _ : state) {
        auto decompressed = compressor.decompress(compressed);
        benchmark::DoNotOptimize(decompressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
    state.SetLabel("Zlib-6 (text)");
}
BENCHMARK(BM_Zlib_Decompress_Text_Level6)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// Fast compression (level 1)
static void BM_Zlib_Compress_Text_Level1(benchmark::State& state) {
    ZlibCompressor compressor(1);
    auto input = generate_text_data(state.range(0));
    
    for (auto _ : state) {
        auto compressed = compressor.compress(input);
        benchmark::DoNotOptimize(compressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
    state.SetLabel("Zlib-1 (fast)");
}
BENCHMARK(BM_Zlib_Compress_Text_Level1)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// Maximum compression (level 9)
static void BM_Zlib_Compress_Text_Level9(benchmark::State& state) {
    ZlibCompressor compressor(9);
    auto input = generate_text_data(state.range(0));
    
    for (auto _ : state) {
        auto compressed = compressor.compress(input);
        benchmark::DoNotOptimize(compressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
    state.SetLabel("Zlib-9 (max)");
}
BENCHMARK(BM_Zlib_Compress_Text_Level9)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// Random data (incompressible)
static void BM_Zlib_Compress_Random(benchmark::State& state) {
    ZlibCompressor compressor(6);
    auto input = generate_random_data(state.range(0));
    
    for (auto _ : state) {
        auto compressed = compressor.compress(input);
        benchmark::DoNotOptimize(compressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
    state.SetLabel("Zlib (random)");
}
BENCHMARK(BM_Zlib_Compress_Random)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Zstd Compression Benchmarks (using Zlib fallback currently)
// ============================================================================

static void BM_Zstd_Compress_Text_Level3(benchmark::State& state) {
    ZstdCompressor compressor(3);  // Default level
    auto input = generate_text_data(state.range(0));
    
    for (auto _ : state) {
        auto compressed = compressor.compress(input);
        benchmark::DoNotOptimize(compressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
    state.SetLabel("Zstd-3 (fallback: Zlib)");
}
BENCHMARK(BM_Zstd_Compress_Text_Level3)
    ->Arg(1024)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

static void BM_Zstd_Decompress_Text_Level3(benchmark::State& state) {
    ZstdCompressor compressor(3);
    auto input = generate_text_data(state.range(0));
    auto compressed = compressor.compress(input);
    
    for (auto _ : state) {
        auto decompressed = compressor.decompress(compressed);
        benchmark::DoNotOptimize(decompressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
    state.SetLabel("Zstd-3 (fallback: Zlib)");
}
BENCHMARK(BM_Zstd_Decompress_Text_Level3)
    ->Arg(1024)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Compression Ratio Test (not a benchmark, just info)
// ============================================================================

static void BM_CompressionRatio_Text(benchmark::State& state) {
    ZlibCompressor zlib(6);
    auto input = generate_text_data(state.range(0));
    auto compressed = zlib.compress(input);
    
    double ratio = (double)compressed.size() / input.size();
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(ratio);
    }
    
    state.SetLabel("Ratio: " + std::to_string((int)(ratio * 100)) + "%");
}
BENCHMARK(BM_CompressionRatio_Text)
    ->Arg(1024*1024)
    ->Iterations(1);

BENCHMARK_MAIN();
