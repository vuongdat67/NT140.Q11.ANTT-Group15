/**
 * FileVault - Key Derivation Function (KDF) Benchmarks
 * 
 * Tests KDF performance for:
 * - Argon2id (memory-hard)
 * - PBKDF2-HMAC-SHA256
 * - Different iteration/memory parameters
 * 
 * Usage:
 *   bench_kdf
 *   bench_kdf --benchmark_filter=Argon2
 */

#include <benchmark/benchmark.h>
#include "filevault/crypto/kdf.hpp"

using namespace filevault;
using namespace filevault::crypto;

// ============================================================================
// Argon2id Benchmarks
// ============================================================================

static void BM_Argon2id_Default(benchmark::State& state) {
    auto kdf = KDFFactory::create(KDFType::ARGON2ID);
    
    std::string password = "MySecurePassword123!";
    Bytes salt(32, 0x42);  // Fixed salt for consistency
    size_t key_length = 32;  // 256-bit key
    
    for (auto _ : state) {
        auto key = kdf->derive_key(password, salt, key_length);
        benchmark::DoNotOptimize(key.data());
    }
    
    state.SetLabel("Argon2id (64MB, 3 iter)");
}
BENCHMARK(BM_Argon2id_Default)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// PBKDF2 Benchmarks
// ============================================================================

static void BM_PBKDF2_Default(benchmark::State& state) {
    auto kdf = KDFFactory::create(KDFType::PBKDF2);
    
    std::string password = "MySecurePassword123!";
    Bytes salt(32, 0x42);
    size_t key_length = 32;
    
    for (auto _ : state) {
        auto key = kdf->derive_key(password, salt, key_length);
        benchmark::DoNotOptimize(key.data());
    }
    
    state.SetLabel("PBKDF2 (600K iter)");
}
BENCHMARK(BM_PBKDF2_Default)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Comparison Test
// ============================================================================

static void BM_KDF_Comparison(benchmark::State& state) {
    auto argon2 = KDFFactory::create(KDFType::ARGON2ID);
    auto pbkdf2 = KDFFactory::create(KDFType::PBKDF2);
    
    std::string password = "TestPassword";
    Bytes salt(32, 0x42);
    
    for (auto _ : state) {
        if (state.range(0) == 0) {
            auto key = pbkdf2->derive_key(password, salt, 32);
            benchmark::DoNotOptimize(key.data());
        } else {
            auto key = argon2->derive_key(password, salt, 32);
            benchmark::DoNotOptimize(key.data());
        }
    }
    
    state.SetLabel(state.range(0) == 0 ? "PBKDF2" : "Argon2id");
}
BENCHMARK(BM_KDF_Comparison)
    ->Arg(0)   // PBKDF2
    ->Arg(1)   // Argon2id
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
