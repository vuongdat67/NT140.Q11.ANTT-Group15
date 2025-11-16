# FileVault - Detailed Action Plan

**Goal:** Fix the 3 major issues and enhance CLI functionality

---

## 🎯 Priority Matrix

| Issue | Priority | Effort | Impact | Order |
|-------|----------|--------|--------|-------|
| CLI algorithm selection | 🔴 HIGH | Medium (4-6h) | High | **1** |
| Zstd compression | 🔴 HIGH | Low (2-3h) | High | **2** |
| Benchmarks implementation | 🟡 MEDIUM | Medium (6-8h) | Medium | **3** |
| Missing ciphers (DES, ChaCha20) | 🟡 MEDIUM | Medium (4-6h) | Medium | **4** |
| Classical ciphers | 🟢 LOW | High (8-10h) | Low | 5 |

---

## 📋 TASK 1: Enhanced CLI with Algorithm Selection

### Current Problem
```cpp
// main.cpp - HARDCODED
auto mode = compress ? SecurityMode::ADVANCED : SecurityMode::STANDARD;
filevault::EncryptionService service(mode);
// ❌ User cannot choose AES-128, CBC mode, PBKDF2, etc.
```

### Target Functionality
```bash
# Choose any algorithm
filevault encrypt file.txt --algorithm aes256 --mode gcm --kdf argon2id

# Mix and match
filevault encrypt file.txt --algorithm aes128 --mode cbc --kdf pbkdf2 --compress zlib

# Shortcuts (presets)
filevault encrypt file.txt --preset standard  # AES-256-GCM + Argon2id
filevault encrypt file.txt --preset advanced  # + compression
filevault encrypt file.txt --preset basic     # DES-CBC + PBKDF2
```

### Implementation Steps

#### Step 1.1: Add CLI Options (15 min)

**File:** `cli/main.cpp`

**Add after line ~55 (encrypt command setup):**
```cpp
std::string encrypt_algorithm = "aes256";  // Default
std::string encrypt_mode = "gcm";           // Default
std::string encrypt_kdf = "argon2id";       // Default
std::string encrypt_compression = "";       // Empty = based on --compress flag
std::string encrypt_preset = "";            // Empty = custom config

encrypt_cmd->add_option("--algorithm,-a", encrypt_algorithm, 
    "Encryption algorithm: aes256, aes192, aes128, chacha20, des")
    ->check(CLI::IsMember({"aes256", "aes192", "aes128", "chacha20", "des"}));

encrypt_cmd->add_option("--mode,-m", encrypt_mode,
    "Cipher mode: gcm, cbc, ctr")
    ->check(CLI::IsMember({"gcm", "cbc", "ctr"}));

encrypt_cmd->add_option("--kdf,-k", encrypt_kdf,
    "Key derivation: argon2id, pbkdf2")
    ->check(CLI::IsMember({"argon2id", "pbkdf2"}));

encrypt_cmd->add_option("--compression", encrypt_compression,
    "Compression: zlib, zstd, none")
    ->check(CLI::IsMember({"zlib", "zstd", "none", ""}));

encrypt_cmd->add_option("--preset", encrypt_preset,
    "Use preset (overrides other options): basic, standard, advanced")
    ->check(CLI::IsMember({"basic", "standard", "advanced", ""}));
```

#### Step 1.2: Add Validation Function (20 min)

**Add before encrypt_cmd->callback():**
```cpp
// Validation helper
auto validate_cipher_combination = [](const std::string& algo, const std::string& mode) -> bool {
    // ChaCha20 only supports its own AEAD mode
    if (algo == "chacha20" && mode != "gcm") {
        fmt::print(fg(fmt::color::red), "[ERROR] ChaCha20 only supports default mode (Poly1305)\n");
        return false;
    }
    
    // DES doesn't support GCM (needs AEAD-capable block cipher)
    if (algo == "des" && mode == "gcm") {
        fmt::print(fg(fmt::color::red), "[ERROR] DES does not support GCM mode (use CBC or CTR)\n");
        return false;
    }
    
    return true;
};

// Map string to enum helper
auto parse_cipher_type = [](const std::string& algo) -> filevault::CipherType {
    if (algo == "aes256") return filevault::CipherType::AES256;
    if (algo == "aes192") return filevault::CipherType::AES192;
    if (algo == "aes128") return filevault::CipherType::AES128;
    if (algo == "chacha20") return filevault::CipherType::CHACHA20;
    if (algo == "des") return filevault::CipherType::DES;
    throw std::invalid_argument("Unknown algorithm: " + algo);
};

auto parse_cipher_mode = [](const std::string& mode) -> filevault::CipherMode {
    if (mode == "gcm") return filevault::CipherMode::GCM;
    if (mode == "cbc") return filevault::CipherMode::CBC;
    if (mode == "ctr") return filevault::CipherMode::CTR;
    throw std::invalid_argument("Unknown mode: " + mode);
};

auto parse_kdf_type = [](const std::string& kdf) -> filevault::KDFType {
    if (kdf == "argon2id") return filevault::KDFType::ARGON2ID;
    if (kdf == "pbkdf2") return filevault::KDFType::PBKDF2;
    throw std::invalid_argument("Unknown KDF: " + kdf);
};

auto parse_compression = [](const std::string& comp) -> filevault::CompressionType {
    if (comp.empty() || comp == "none") return filevault::CompressionType::NONE;
    if (comp == "zlib") return filevault::CompressionType::ZLIB;
    if (comp == "zstd") return filevault::CompressionType::ZSTD;
    throw std::invalid_argument("Unknown compression: " + comp);
};
```

#### Step 1.3: Replace Service Creation (30 min)

**Replace lines ~85-90 (current service creation):**
```cpp
// OLD CODE (DELETE):
// auto mode = compress ? filevault::SecurityMode::ADVANCED : filevault::SecurityMode::STANDARD;
// filevault::EncryptionService service(mode);

// NEW CODE:
filevault::EncryptionService service;

// Use preset if specified, otherwise custom config
if (!encrypt_preset.empty()) {
    if (encrypt_preset == "basic") {
        service = filevault::EncryptionService(filevault::SecurityMode::BASIC);
        fmt::print("Preset: Basic (DES-CBC + PBKDF2)\n");
    } else if (encrypt_preset == "standard") {
        service = filevault::EncryptionService(filevault::SecurityMode::STANDARD);
        fmt::print("Preset: Standard (AES-256-GCM + Argon2id)\n");
    } else if (encrypt_preset == "advanced") {
        service = filevault::EncryptionService(filevault::SecurityMode::ADVANCED);
        fmt::print("Preset: Advanced (AES-256-GCM + Argon2id + Zstd)\n");
    }
} else {
    // Custom configuration
    if (!validate_cipher_combination(encrypt_algorithm, encrypt_mode)) {
        std::exit(1);
    }
    
    // Determine compression
    std::string comp_type = encrypt_compression;
    if (comp_type.empty() && compress) {
        comp_type = "zlib";  // Default when --compress used
    }
    
    // Create components
    auto cipher = filevault::crypto::CipherFactory::create(
        parse_cipher_type(encrypt_algorithm),
        parse_cipher_mode(encrypt_mode)
    );
    
    auto kdf = filevault::crypto::KDFFactory::create(
        parse_kdf_type(encrypt_kdf)
    );
    
    std::unique_ptr<filevault::compression::ICompressor> compressor = nullptr;
    if (!comp_type.empty() && comp_type != "none") {
        compressor = filevault::compression::CompressorFactory::create(
            parse_compression(comp_type)
        );
    }
    
    // Create service with custom config
    service = filevault::EncryptionService(
        std::move(cipher),
        std::move(kdf),
        std::move(compressor)
    );
}
```

#### Step 1.4: Update Display Info (15 min)

**Replace lines ~115-120 (display section):**
```cpp
fmt::print("Input:       {}\n", encrypt_input);
fmt::print("Output:      {}\n", encrypt_output);
fmt::print("Size:        {}\n", format_size(input_size));

// Show actual configuration
if (encrypt_preset.empty()) {
    fmt::print("Algorithm:   {}-{}\n", encrypt_algorithm, encrypt_mode);
    fmt::print("KDF:         {}\n", encrypt_kdf);
    if (!encrypt_compression.empty() && encrypt_compression != "none") {
        fmt::print("Compression: {} (enabled)\n", encrypt_compression);
    } else {
        fmt::print("Compression: None\n");
    }
} else {
    // Preset info already printed above
}
```

#### Step 1.5: Add Usage Examples to Help (10 min)

**Add at end of main.cpp (before return):**
```cpp
// Add examples to help text
app.footer(R"(
Examples:
  # Standard encryption (recommended)
  filevault encrypt secret.txt

  # AES-128 with CBC mode
  filevault encrypt file.txt --algorithm aes128 --mode cbc
  
  # Maximum security with compression
  filevault encrypt data.zip --preset advanced
  
  # Fast encryption (educational only)
  filevault encrypt demo.txt --preset basic
  
  # Custom: AES-256-CTR with PBKDF2
  filevault encrypt file.txt -a aes256 -m ctr -k pbkdf2 --compress zlib
)");
```

### Testing Plan

```powershell
# Test 1: Default (AES-256-GCM)
.\build\bin\filevault encrypt test.txt

# Test 2: AES-128-CBC
.\build\bin\filevault encrypt test.txt -a aes128 -m cbc -o test_aes128.fv

# Test 3: With compression
.\build\bin\filevault encrypt test.txt --compress zlib

# Test 4: Invalid combination (should fail gracefully)
.\build\bin\filevault encrypt test.txt -a des -m gcm
# Expected: Error message

# Test 5: Preset
.\build\bin\filevault encrypt test.txt --preset advanced
```

**Estimated Time:** 1.5 - 2 hours

---

## 📋 TASK 2: Implement Zstd Compression

### Current Problem
```cpp
Bytes ZstdCompressor::compress(const Bytes& input) {
    throw CompressionException("Zstd compression not yet implemented");
}
```

### Solution: Use Botan's Built-in Compression

Botan already has Zstd support! We don't need external library.

### Implementation Steps

#### Step 2.1: Check Botan Compression Support (5 min)

**Test if Botan has zstd:**
```cpp
#include <botan/compression.h>

// Check available algorithms
auto algos = Botan::Compression_Algorithm::providers("zstd");
if (!algos.empty()) {
    // Zstd available!
}
```

#### Step 2.2: Implement Using Botan (30 min)

**File:** `src/compression/compression_factory.cpp`

**Replace ZstdCompressor methods (lines ~120-135):**
```cpp
#include <botan/compression.h>

// ============================================================================
// ZstdCompressor Implementation (using Botan)
// ============================================================================

ZstdCompressor::ZstdCompressor(int level) : level_(level) {
    // Validate compression level (1-22 for zstd)
    if (level_ < 1 || level_ > 22) {
        level_ = 3;  // Default zstd level (fast)
    }
}

Bytes ZstdCompressor::compress(const Bytes& input) {
    if (input.empty()) {
        return Bytes();
    }
    
    try {
        // Create Botan compression object
        auto compressor = Botan::Compression_Algorithm::create("zstd");
        if (!compressor) {
            throw CompressionException("Zstd compression not available in Botan build");
        }
        
        // Start compression with level
        compressor->start(level_);
        
        // Compress data
        Botan::secure_vector<uint8_t> compressed;
        compressed = compressor->compress(input.data(), input.size());
        
        // Convert to regular vector
        return Bytes(compressed.begin(), compressed.end());
        
    } catch (const Botan::Exception& e) {
        throw CompressionException(std::string("Zstd compression failed: ") + e.what());
    } catch (const std::bad_alloc& e) {
        throw CompressionException(std::string("Zstd compression: out of memory - ") + e.what());
    }
}

Bytes ZstdCompressor::decompress(const Bytes& input) {
    if (input.empty()) {
        return Bytes();
    }
    
    try {
        // Create Botan decompression object
        auto decompressor = Botan::Decompression_Algorithm::create("zstd");
        if (!decompressor) {
            throw CompressionException("Zstd decompression not available in Botan build");
        }
        
        // Start decompression
        decompressor->start();
        
        // Decompress data
        Botan::secure_vector<uint8_t> decompressed;
        decompressed = decompressor->decompress(input.data(), input.size());
        
        // Convert to regular vector
        return Bytes(decompressed.begin(), decompressed.end());
        
    } catch (const Botan::Exception& e) {
        throw CompressionException(std::string("Zstd decompression failed: ") + e.what());
    } catch (const std::bad_alloc& e) {
        throw CompressionException(std::string("Zstd decompression: out of memory - ") + e.what());
    }
}
```

#### Step 2.3: Add Unit Test (20 min)

**File:** `tests/unit/compression/test_compression.cpp` (create if not exists)

```cpp
#include <catch2/catch_test_macros.hpp>
#include "filevault/compression/compressor.hpp"

using namespace filevault::compression;

TEST_CASE("Zstd compression round-trip", "[compression][zstd]") {
    ZstdCompressor compressor(3);  // Level 3
    
    SECTION("Small text") {
        std::string text = "Hello World! This is a test message.";
        Bytes input(text.begin(), text.end());
        
        auto compressed = compressor.compress(input);
        REQUIRE(!compressed.empty());
        REQUIRE(compressed.size() < input.size());  // Should compress
        
        auto decompressed = compressor.decompress(compressed);
        REQUIRE(decompressed == input);  // Lossless
    }
    
    SECTION("Large repetitive data") {
        Bytes input(10000, 'A');  // 10KB of 'A'
        
        auto compressed = compressor.compress(input);
        REQUIRE(compressed.size() < input.size() / 10);  // High compression ratio
        
        auto decompressed = compressor.decompress(compressed);
        REQUIRE(decompressed == input);
    }
    
    SECTION("Empty input") {
        Bytes input;
        auto compressed = compressor.compress(input);
        REQUIRE(compressed.empty());
    }
}

TEST_CASE("Zlib vs Zstd comparison", "[compression][benchmark]") {
    std::string text(1000, 'A');
    Bytes input(text.begin(), text.end());
    
    ZlibCompressor zlib(6);
    ZstdCompressor zstd(3);
    
    auto zlib_compressed = zlib.compress(input);
    auto zstd_compressed = zstd.compress(input);
    
    // Zstd should be comparable or better
    INFO("Zlib size: " << zlib_compressed.size());
    INFO("Zstd size: " << zstd_compressed.size());
    
    REQUIRE(zstd_compressed.size() <= zlib_compressed.size() * 1.1);  // Within 10%
}
```

### Testing Plan

```powershell
# Build with tests
cmake --build build --config Release

# Run compression tests
.\build\bin\tests\filevault_tests "[compression]"

# Test via CLI
echo "Test data Test data Test data" > test.txt

# Encrypt with Zstd
.\build\bin\filevault encrypt test.txt --compress zstd -o test_zstd.fv

# Decrypt and verify
.\build\bin\filevault decrypt test_zstd.fv -o test_zstd_dec.txt
fc test.txt test_zstd_dec.txt  # Should be identical
```

**Estimated Time:** 1 - 1.5 hours

---

## 📋 TASK 3: Implement Benchmarks

### Create Benchmark Suite

#### Step 3.1: Setup Benchmark CMake (15 min)

**File:** `benchmarks/CMakeLists.txt` (create)

```cmake
# Benchmarks using Google Benchmark
find_package(benchmark REQUIRED)

# Benchmark executables
add_executable(bench_crypto
    bench_crypto.cpp
)

target_link_libraries(bench_crypto
    PRIVATE
        filevault
        benchmark::benchmark
        benchmark::benchmark_main
)

add_executable(bench_compression
    bench_compression.cpp
)

target_link_libraries(bench_compression
    PRIVATE
        filevault
        benchmark::benchmark
        benchmark::benchmark_main
)

# Add to test suite (optional)
add_test(NAME benchmark_crypto COMMAND bench_crypto --benchmark_min_time=1s)
```

#### Step 3.2: Crypto Benchmarks (45 min)

**File:** `benchmarks/bench_crypto.cpp` (create)

```cpp
#include <benchmark/benchmark.h>
#include "filevault/crypto/cipher.hpp"
#include "filevault/crypto/kdf.hpp"
#include <botan/auto_rng.h>

using namespace filevault;

// ============================================================================
// AES Benchmarks
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
BENCHMARK(BM_AES256_GCM_Encrypt)->RangeMultiplier(10)->Range(1024, 10*1024*1024);

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
BENCHMARK(BM_AES256_CBC_Encrypt)->RangeMultiplier(10)->Range(1024, 10*1024*1024);

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
BENCHMARK(BM_AES128_GCM_Encrypt)->RangeMultiplier(10)->Range(1024, 10*1024*1024);

// ============================================================================
// KDF Benchmarks
// ============================================================================

static void BM_Argon2id_KeyDerivation(benchmark::State& state) {
    auto kdf = crypto::KDFFactory::create(KDFType::ARGON2ID);
    
    std::string password = "TestPassword123!";
    Bytes salt(32, 0x42);
    
    for (auto _ : state) {
        auto key = kdf->derive_key(password, salt, 32);
        benchmark::DoNotOptimize(key.data());
    }
    
    state.SetLabel("Argon2id (64MB)");
}
BENCHMARK(BM_Argon2id_KeyDerivation);

static void BM_PBKDF2_KeyDerivation(benchmark::State& state) {
    auto kdf = crypto::KDFFactory::create(KDFType::PBKDF2);
    
    std::string password = "TestPassword123!";
    Bytes salt(32, 0x42);
    
    for (auto _ : state) {
        auto key = kdf->derive_key(password, salt, 32);
        benchmark::DoNotOptimize(key.data());
    }
    
    state.SetLabel("PBKDF2 (600K iterations)");
}
BENCHMARK(BM_PBKDF2_KeyDerivation);

// Run benchmarks
BENCHMARK_MAIN();
```

#### Step 3.3: Compression Benchmarks (30 min)

**File:** `benchmarks/bench_compression.cpp` (create)

```cpp
#include <benchmark/benchmark.h>
#include "filevault/compression/compressor.hpp"

using namespace filevault::compression;

static void BM_Zlib_Compress(benchmark::State& state) {
    ZlibCompressor compressor(6);
    Bytes input(state.range(0), 'A');
    
    for (auto _ : state) {
        auto compressed = compressor.compress(input);
        benchmark::DoNotOptimize(compressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
}
BENCHMARK(BM_Zlib_Compress)->RangeMultiplier(10)->Range(1024, 10*1024*1024);

static void BM_Zstd_Compress(benchmark::State& state) {
    ZstdCompressor compressor(3);
    Bytes input(state.range(0), 'A');
    
    for (auto _ : state) {
        auto compressed = compressor.compress(input);
        benchmark::DoNotOptimize(compressed.data());
    }
    
    state.SetBytesProcessed(state.iterations() * input.size());
}
BENCHMARK(BM_Zstd_Compress)->RangeMultiplier(10)->Range(1024, 10*1024*1024);

BENCHMARK_MAIN();
```

#### Step 3.4: Update Root CMakeLists.txt (5 min)

**File:** `CMakeLists.txt`

**Add after line with tests:**
```cmake
# Benchmarks (optional)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
```

### Testing Plan

```powershell
# Build with benchmarks
cmake --preset conan-default -DBUILD_BENCHMARKS=ON
cmake --build --preset conan-release

# Run crypto benchmarks
.\build\benchmarks\Release\bench_crypto.exe

# Run compression benchmarks  
.\build\benchmarks\Release\bench_compression.exe

# Export to JSON
.\build\benchmarks\Release\bench_crypto.exe --benchmark_format=json --benchmark_out=results.json
```

**Estimated Time:** 1.5 - 2 hours

---

## 📊 Summary Timeline

| Task | Priority | Time | Status |
|------|----------|------|--------|
| 1. Enhanced CLI | 🔴 HIGH | 1.5-2h | ⏸️ Ready |
| 2. Zstd Implementation | 🔴 HIGH | 1-1.5h | ⏸️ Ready |
| 3. Benchmarks | 🟡 MEDIUM | 1.5-2h | ⏸️ Ready |
| 4. Testing & Validation | - | 1h | ⏸️ Ready |

**Total Estimated Time:** 5-6.5 hours for all 3 fixes

---

## 🚀 Getting Started

### Quick Start Command

```powershell
# Start with Task 1 (highest impact)
# I'll help you modify cli/main.cpp step by step

# After Task 1 complete, rebuild and test:
cmake --build build --config Release
.\build\bin\Release\filevault.exe encrypt test.txt --algorithm aes128 --mode cbc
```

**Which task would you like to start with?**
1. Enhanced CLI (most user-visible)
2. Zstd compression (quickest win)
3. Benchmarks (foundation for performance)

Let me know and I'll provide detailed code changes! 🎯
