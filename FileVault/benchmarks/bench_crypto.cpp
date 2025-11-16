/**
 * FileVault - Cryptography Benchmarks
 * 
 * Tests encryption/decryption performance for:
 * - AES-256-GCM, AES-256-CBC, AES-256-CTR
 * - AES-128-GCM, AES-128-CBC
 * - Different data sizes (1KB to 10MB)
 * 
 * Usage:
 *   bench_crypto                                   # Run all benchmarks
 *   bench_crypto --benchmark_filter=AES256_GCM     # Run specific
 *   bench_crypto --benchmark_out=results.json      # Export results
 */

#include <benchmark/benchmark.h>
#include "filevault/crypto/cipher.hpp"
#include <botan/auto_rng.h>

using namespace filevault;

// ============================================================================
// AES-256-GCM Benchmarks
// ============================================================================

static void BM_AES256_GCM_Encrypt(benchmark::State& state) {
    auto cipher = crypto::CipherFactory::create(CipherType::AES256, CipherMode::GCM);
    
    Botan::AutoSeeded_RNG rng;
    size_t data_size = state.range(0);
    Bytes plaintext(data_size, 'A');
    SecureBytes key(32);
    rng.randomize(key.data(), key.size());
    Bytes iv(12);
    rng.randomize(iv.data(), iv.size());
    
    for (auto _ : state) {
        auto ciphertext = cipher->encrypt(plaintext, key, iv);
        benchmark::DoNotOptimize(ciphertext.data());
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * data_size);
    state.SetLabel("AES-256-GCM");
}
BENCHMARK(BM_AES256_GCM_Encrypt)
    ->Arg(1024)           // 1 KB
    ->Arg(10240)          // 10 KB
    ->Arg(102400)         // 100 KB
    ->Arg(1024*1024)      // 1 MB
    ->Arg(10*1024*1024)   // 10 MB
    ->Unit(benchmark::kMillisecond);

static void BM_AES256_GCM_Decrypt(benchmark::State& state) {
    auto cipher = crypto::CipherFactory::create(CipherType::AES256, CipherMode::GCM);
    
    Botan::AutoSeeded_RNG rng;
    size_t data_size = state.range(0);
    Bytes plaintext(data_size, 'A');
    SecureBytes key(32);
    rng.randomize(key.data(), key.size());
    Bytes iv(12);
    rng.randomize(iv.data(), iv.size());
    
    // Pre-encrypt data
    auto ciphertext = cipher->encrypt(plaintext, key, iv);
    
    for (auto _ : state) {
        auto decrypted = cipher->decrypt(ciphertext, key, iv);
        benchmark::DoNotOptimize(decrypted.data());
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * data_size);
    state.SetLabel("AES-256-GCM");
}
BENCHMARK(BM_AES256_GCM_Decrypt)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// AES-256-CBC Benchmarks
// ============================================================================

static void BM_AES256_CBC_Encrypt(benchmark::State& state) {
    auto cipher = crypto::CipherFactory::create(CipherType::AES256, CipherMode::CBC);
    
    Botan::AutoSeeded_RNG rng;
    size_t data_size = state.range(0);
    Bytes plaintext(data_size, 'A');
    SecureBytes key(32);
    rng.randomize(key.data(), key.size());
    Bytes iv(16);
    rng.randomize(iv.data(), iv.size());
    
    for (auto _ : state) {
        auto ciphertext = cipher->encrypt(plaintext, key, iv);
        benchmark::DoNotOptimize(ciphertext.data());
    }
    
    state.SetBytesProcessed(state.iterations() * data_size);
    state.SetLabel("AES-256-CBC");
}
BENCHMARK(BM_AES256_CBC_Encrypt)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

static void BM_AES256_CBC_Decrypt(benchmark::State& state) {
    auto cipher = crypto::CipherFactory::create(CipherType::AES256, CipherMode::CBC);
    
    Botan::AutoSeeded_RNG rng;
    size_t data_size = state.range(0);
    Bytes plaintext(data_size, 'A');
    SecureBytes key(32);
    rng.randomize(key.data(), key.size());
    Bytes iv(16);
    rng.randomize(iv.data(), iv.size());
    
    auto ciphertext = cipher->encrypt(plaintext, key, iv);
    
    for (auto _ : state) {
        auto decrypted = cipher->decrypt(ciphertext, key, iv);
        benchmark::DoNotOptimize(decrypted.data());
    }
    
    state.SetBytesProcessed(state.iterations() * data_size);
    state.SetLabel("AES-256-CBC");
}
BENCHMARK(BM_AES256_CBC_Decrypt)
    ->Arg(1024)
    ->Arg(10240)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// AES-256-CTR Benchmarks
// ============================================================================

static void BM_AES256_CTR_Encrypt(benchmark::State& state) {
    auto cipher = crypto::CipherFactory::create(CipherType::AES256, CipherMode::CTR);
    
    Botan::AutoSeeded_RNG rng;
    size_t data_size = state.range(0);
    Bytes plaintext(data_size, 'A');
    SecureBytes key(32);
    rng.randomize(key.data(), key.size());
    Bytes iv(16);
    rng.randomize(iv.data(), iv.size());
    
    for (auto _ : state) {
        auto ciphertext = cipher->encrypt(plaintext, key, iv);
        benchmark::DoNotOptimize(ciphertext.data());
    }
    
    state.SetBytesProcessed(state.iterations() * data_size);
    state.SetLabel("AES-256-CTR");
}
BENCHMARK(BM_AES256_CTR_Encrypt)
    ->Arg(1024)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// AES-128-GCM Benchmarks (comparison)
// ============================================================================

static void BM_AES128_GCM_Encrypt(benchmark::State& state) {
    auto cipher = crypto::CipherFactory::create(CipherType::AES128, CipherMode::GCM);
    
    Botan::AutoSeeded_RNG rng;
    size_t data_size = state.range(0);
    Bytes plaintext(data_size, 'A');
    SecureBytes key(16);
    rng.randomize(key.data(), key.size());
    Bytes iv(12);
    rng.randomize(iv.data(), iv.size());
    
    for (auto _ : state) {
        auto ciphertext = cipher->encrypt(plaintext, key, iv);
        benchmark::DoNotOptimize(ciphertext.data());
    }
    
    state.SetBytesProcessed(state.iterations() * data_size);
    state.SetLabel("AES-128-GCM");
}
BENCHMARK(BM_AES128_GCM_Encrypt)
    ->Arg(1024)
    ->Arg(102400)
    ->Arg(1024*1024)
    ->Arg(10*1024*1024)
    ->Unit(benchmark::kMillisecond);

// Google Benchmark will automatically generate the main() function
BENCHMARK_MAIN();
