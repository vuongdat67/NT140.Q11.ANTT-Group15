# GitHub Copilot Instructions for FileVault

## 🎯 Project Overview

FileVault is a professional cross-platform file encryption CLI tool with comprehensive cryptographic algorithm support, built with modern C++20 and Botan 3.x.

**Core Philosophy:**
- Security first, performance second, usability third
- Clean code > clever code
- Test everything, trust nothing
- Document for your future self

---

## 📋 Project Standards

### Language & Compiler Requirements

```cpp
// REQUIRED: C++20 with Botan 3.x
#include <botan/version.h>
static_assert(BOTAN_VERSION_MAJOR >= 3, "Botan 3.x required");

// Use modern C++20 features
#include <concepts>
#include <ranges>
#include <span>
#include <format>  // C++20 formatting
```

### Coding Standards

#### 1. **File Organization**
```
Rule: ONE algorithm per file, ONE class per file
Good: src/algorithms/symmetric/aes.cpp
Bad:  src/crypto_stuff.cpp (multiple algorithms)

Rule: Header-only for templates, .cpp for implementations
Good: include/filevault/core/crypto_engine.hpp + src/core/crypto_engine.cpp
Bad:  everything in .hpp files
```

#### 2. **Naming Conventions**
```cpp
// Classes: PascalCase
class CryptoEngine { };

// Functions/methods: snake_case
void encrypt_file(const std::string& path);

// Constants: UPPER_SNAKE_CASE
constexpr size_t MAX_FILE_SIZE = 1024 * 1024 * 1024;

// Private members: trailing underscore
class MyClass {
private:
    int counter_;
    std::string name_;
};

// Namespaces: lowercase
namespace filevault::core::algorithms { }
```

#### 3. **Modern C++20 Patterns**

```cpp
// ✅ GOOD: Use concepts for type constraints
template<std::ranges::range R>
void process_data(R&& data) { }

// ✅ GOOD: Use std::span for contiguous data
void encrypt(std::span<const uint8_t> plaintext);

// ✅ GOOD: Use std::format (C++20)
auto msg = std::format("Encrypted {} bytes", size);

// ✅ GOOD: Use ranges
auto filtered = data | std::views::filter(is_valid)
                    | std::views::transform(encrypt);

// ❌ BAD: Old-style loops when ranges work
for (size_t i = 0; i < vec.size(); ++i) { }

// ✅ GOOD: Range-based for or ranges
for (const auto& item : vec) { }
```

#### 4. **Error Handling**

```cpp
// ✅ GOOD: Use std::expected (C++23) or custom Result type
[[nodiscard]] Result<std::vector<uint8_t>> encrypt_file(const std::string& path);

// ✅ GOOD: Specific exceptions with context
class EncryptionError : public std::runtime_error {
public:
    EncryptionError(const std::string& msg, std::string file, int line)
        : std::runtime_error(msg), file_(std::move(file)), line_(line) {}
private:
    std::string file_;
    int line_;
};

// ✅ GOOD: RAII for resource management
class SecureMemory {
public:
    SecureMemory(size_t size) : data_(new uint8_t[size]), size_(size) {
        lock_memory(data_, size_);
    }
    ~SecureMemory() {
        secure_zero(data_, size_);
        unlock_memory(data_, size_);
        delete[] data_;
    }
    // Delete copy, allow move
    SecureMemory(const SecureMemory&) = delete;
    SecureMemory& operator=(const SecureMemory&) = delete;
    SecureMemory(SecureMemory&&) noexcept = default;
    SecureMemory& operator=(SecureMemory&&) noexcept = default;
};

// ❌ BAD: Raw pointers without RAII
uint8_t* data = new uint8_t[size];
// ... (forgot to delete)
```

#### 5. **Security Guidelines**

```cpp
// ✅ CRITICAL: Generate unique salt/nonce for EVERY encryption
auto salt = generate_random_bytes(32);  // MUST be unique
auto nonce = generate_random_bytes(12); // MUST NEVER reuse

// ✅ CRITICAL: Clear sensitive data
void process_password(const std::string& password) {
    SecureVector<uint8_t> key = derive_key(password);
    // ... use key ...
    secure_zero(key.data(), key.size());  // MUST clear
}

// ✅ CRITICAL: Constant-time comparison
bool verify_tag(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    return constant_time_compare(a.data(), b.data(), a.size());
}

// ❌ BAD: Timing attack vulnerable
if (computed_tag == provided_tag) { } // Variable time!

// ✅ GOOD: Store salt/nonce with ciphertext
struct EncryptedData {
    std::vector<uint8_t> salt;    // Store with data
    std::vector<uint8_t> nonce;   // Store with data
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;     // GCM authentication tag
};
```

#### 6. **Botan 3.x Usage**

```cpp
// ✅ GOOD: Modern Botan 3.x API
#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
#include <botan/argon2.h>

// Initialize RNG
Botan::AutoSeeded_RNG rng;

// Create cipher
auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", Botan::Cipher_Dir::Encryption);

// Key derivation with Argon2
auto key = Botan::argon2_generate(
    32,                          // output length
    password,                    // password
    salt.data(), salt.size(),    // salt
    65536,                       // memory in KB
    4,                           // parallelism
    100000                       // iterations
);

// ❌ BAD: Using deprecated Botan 2.x API
Botan::PBKDF* pbkdf = get_pbkdf("PBKDF2(SHA-256)"); // Old API
```

---

## 🔧 Development Workflow

### When Creating New Algorithms

```cpp
// Template for new algorithm implementation:

// 1. Create header: include/filevault/algorithms/[category]/[name].hpp
#ifndef FILEVAULT_ALGORITHMS_SYMMETRIC_CHACHA20_HPP
#define FILEVAULT_ALGORITHMS_SYMMETRIC_CHACHA20_HPP

#include "filevault/core/crypto_engine.hpp"
#include <botan/chacha20poly1305.h>

namespace filevault::algorithms::symmetric {

/**
 * @brief ChaCha20-Poly1305 AEAD cipher implementation
 * @see RFC 8439
 * @see https://botan.randombit.net/handbook/api_ref/cipher_modes.html
 */
class ChaCha20Poly1305 : public core::ICryptoAlgorithm {
public:
    std::string name() const override { return "ChaCha20-Poly1305"; }
    
    core::AlgorithmType type() const override {
        return core::AlgorithmType::CHACHA20_POLY1305;
    }
    
    core::CryptoResult encrypt(
        std::span<const uint8_t> plaintext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    core::CryptoResult decrypt(
        std::span<const uint8_t> ciphertext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    size_t key_size() const override { return 32; }  // 256 bits
    
    bool is_suitable_for(core::UserLevel level) const override {
        return level >= core::UserLevel::PROFESSIONAL;
    }

private:
    // Helper methods
    std::vector<uint8_t> do_cipher(
        std::span<const uint8_t> input,
        std::span<const uint8_t> key,
        std::span<const uint8_t> nonce,
        Botan::Cipher_Dir direction
    );
};

} // namespace filevault::algorithms::symmetric

#endif

// 2. Create implementation: src/algorithms/symmetric/chacha20.cpp
#include "filevault/algorithms/symmetric/chacha20.hpp"
#include <botan/auto_rng.h>

namespace filevault::algorithms::symmetric {

core::CryptoResult ChaCha20Poly1305::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    try {
        // Validate inputs
        if (key.size() != key_size()) {
            return {
                .success = false,
                .error_message = std::format("Invalid key size: {} (expected {})", 
                                           key.size(), key_size())
            };
        }
        
        // Generate unique nonce for THIS encryption
        Botan::AutoSeeded_RNG rng;
        std::vector<uint8_t> nonce(12);  // 96-bit nonce for ChaCha20
        rng.randomize(nonce.data(), nonce.size());
        
        // Encrypt
        auto start = std::chrono::high_resolution_clock::now();
        auto ciphertext = do_cipher(plaintext, key, nonce, Botan::Cipher_Dir::Encryption);
        auto end = std::chrono::high_resolution_clock::now();
        
        // Package result with nonce
        std::vector<uint8_t> result;
        result.reserve(nonce.size() + ciphertext.size());
        result.insert(result.end(), nonce.begin(), nonce.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());
        
        return {
            .success = true,
            .data = std::move(result),
            .algorithm_used = type(),
            .original_size = plaintext.size(),
            .final_size = result.size(),
            .processing_time_ms = std::chrono::duration<double, std::milli>(end - start).count()
        };
        
    } catch (const Botan::Exception& e) {
        return {
            .success = false,
            .error_message = std::format("Botan error: {}", e.what())
        };
    } catch (const std::exception& e) {
        return {
            .success = false,
            .error_message = std::format("Encryption failed: {}", e.what())
        };
    }
}

} // namespace filevault::algorithms::symmetric

// 3. Create test: tests/algorithms/test_chacha20.cpp
#include <catch2/catch_test_macros.hpp>
#include "filevault/algorithms/symmetric/chacha20.hpp"

TEST_CASE("ChaCha20-Poly1305 basic encryption/decryption", "[chacha20]") {
    using namespace filevault;
    
    algorithms::symmetric::ChaCha20Poly1305 cipher;
    
    SECTION("Encrypt and decrypt") {
        std::vector<uint8_t> plaintext = {'H', 'e', 'l', 'l', 'o'};
        std::vector<uint8_t> key(32, 0xAB);  // Test key
        
        core::EncryptionConfig config;
        
        auto encrypted = cipher.encrypt(plaintext, key, config);
        REQUIRE(encrypted.success);
        REQUIRE(encrypted.data.size() > plaintext.size());  // Has nonce + tag
        
        auto decrypted = cipher.decrypt(encrypted.data, key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == plaintext);
    }
    
    SECTION("Invalid key size") {
        std::vector<uint8_t> plaintext = {'T', 'e', 's', 't'};
        std::vector<uint8_t> wrong_key(16);  // Too short
        
        core::EncryptionConfig config;
        auto result = cipher.encrypt(plaintext, wrong_key, config);
        REQUIRE_FALSE(result.success);
    }
}

// 4. Add NIST test vectors: tests/algorithms/nist_vectors/chacha20_vectors.cpp
TEST_CASE("ChaCha20 NIST test vectors", "[chacha20][nist]") {
    // From RFC 8439
    const std::vector<uint8_t> key = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        // ... (32 bytes total)
    };
    
    const std::vector<uint8_t> nonce = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a,
        0x00, 0x00, 0x00, 0x00
    };
    
    const std::vector<uint8_t> plaintext = {
        // ... test data
    };
    
    const std::vector<uint8_t> expected_ciphertext = {
        // ... expected output from RFC
    };
    
    // Test encryption matches NIST vectors
    auto result = encrypt_with_nonce(plaintext, key, nonce);
    REQUIRE(result == expected_ciphertext);
}
```

---

## 🧪 Testing Requirements

### Every Algorithm MUST Have:

1. **Unit tests** (basic functionality)
2. **NIST test vectors** (correctness validation)
3. **Edge case tests** (empty input, large input, etc.)
4. **Security tests** (no nonce reuse, unique outputs, etc.)

```cpp
// Security test template
TEST_CASE("Security properties", "[security][algorithm_name]") {
    SECTION("Same input, different outputs (unique nonces)") {
        std::vector<uint8_t> plaintext = {'T', 'e', 's', 't'};
        std::vector<uint8_t> key(32, 0xAB);
        
        std::vector<std::vector<uint8_t>> ciphertexts;
        for (int i = 0; i < 100; ++i) {
            auto result = cipher.encrypt(plaintext, key, config);
            REQUIRE(result.success);
            ciphertexts.push_back(result.data);
        }
        
        // All ciphertexts MUST be different
        for (size_t i = 0; i < ciphertexts.size(); ++i) {
            for (size_t j = i + 1; j < ciphertexts.size(); ++j) {
                REQUIRE(ciphertexts[i] != ciphertexts[j]);
            }
        }
    }
    
    SECTION("No information leakage") {
        // Test that metadata doesn't leak info
    }
}
```

---

## 📚 Documentation Requirements

### Code Comments

```cpp
// ✅ GOOD: Doxygen style with details
/**
 * @brief Derive encryption key from password using Argon2id
 * 
 * Uses Argon2id (RFC 9106) with recommended parameters for password-based
 * key derivation. Provides protection against:
 * - Rainbow table attacks (via unique salt)
 * - GPU cracking (via memory-hard function)
 * - Side-channel attacks (via data-independent memory access)
 * 
 * @param password User-provided password (will be securely cleared)
 * @param salt Unique random salt (min 32 bytes, MUST be unique per file)
 * @param iterations Number of iterations (min 100000 for OWASP 2023)
 * @param memory_kb Memory usage in KB (min 65536 for high security)
 * @param parallelism Degree of parallelism (typically 4)
 * @param key_length Output key length in bytes (typically 32 for AES-256)
 * 
 * @return Derived key
 * @throws std::invalid_argument if parameters are invalid
 * 
 * @see https://www.rfc-editor.org/rfc/rfc9106.html
 * @see https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html
 * 
 * @note Salt MUST be stored with ciphertext for decryption
 * @warning Never reuse salt across different encryptions
 */
SecureVector<uint8_t> derive_key_argon2(
    const SecureString& password,
    std::span<const uint8_t> salt,
    uint32_t iterations = 100000,
    uint32_t memory_kb = 65536,
    uint32_t parallelism = 4,
    size_t key_length = 32
);

// ❌ BAD: Vague or missing comments
// Derives a key
std::vector<uint8_t> derive_key(const std::string& pwd, const std::vector<uint8_t>& s);
```

### Reference Documentation

Every algorithm file should include references:

```cpp
/**
 * @file aes.cpp
 * @brief AES (Advanced Encryption Standard) implementation using Botan 3.x
 * 
 * References:
 * - FIPS 197: Advanced Encryption Standard
 *   https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
 * 
 * - NIST SP 800-38D: GCM Mode
 *   https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf
 * 
 * - Botan AES Documentation:
 *   https://botan.randombit.net/handbook/api_ref/cipher_modes.html
 * 
 * Test Vectors:
 * - NIST CAVP: https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program
 */
```

---

## 🐛 Debugging Guidelines

### When Things Go Wrong:

```cpp
// ✅ GOOD: Detailed error messages with context
if (file_size > MAX_FILE_SIZE) {
    throw std::runtime_error(
        std::format("File too large: {} bytes (max: {} bytes)\n"
                   "File: {}\n"
                   "Consider using streaming encryption for large files",
                   file_size, MAX_FILE_SIZE, file_path)
    );
}

// ✅ GOOD: Debug logging (use spdlog)
spdlog::debug("Encrypting file: path={}, size={}, algorithm={}", 
              file_path, file_size, algorithm_name);

// ✅ GOOD: Assertions for invariants
assert(salt.size() >= 32 && "Salt must be at least 32 bytes");
assert(nonce.size() == 12 && "GCM nonce must be exactly 12 bytes");

// ✅ GOOD: Hex dump for debugging
void print_hex_dump(std::span<const uint8_t> data, const std::string& label) {
    std::cout << std::format("{} ({} bytes):\n", label, data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout << std::format("{:02x} ", data[i]);
        if ((i + 1) % 16 == 0) std::cout << '\n';
    }
    std::cout << '\n';
}
```

### Common Pitfalls to Avoid:

```cpp
// ❌ PITFALL 1: Nonce reuse
static std::vector<uint8_t> global_nonce(12);  // NEVER DO THIS!

// ✅ CORRECT: Generate new nonce every time
Botan::AutoSeeded_RNG rng;
std::vector<uint8_t> nonce(12);
rng.randomize(nonce.data(), nonce.size());

// ❌ PITFALL 2: Not clearing sensitive data
std::string password = get_password();
auto key = derive_key(password);
// password still in memory! ❌

// ✅ CORRECT: Use SecureString and clear
SecureString password = get_password();
auto key = derive_key(password);
secure_zero(password.data(), password.size());

// ❌ PITFALL 3: Timing attacks in comparison
bool verify(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    return a == b;  // Variable-time comparison!
}

// ✅ CORRECT: Constant-time comparison
bool verify(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    return Botan::constant_time_compare(a.data(), b.data(), a.size());
}

// ❌ PITFALL 4: Integer overflow
size_t total_size = num_blocks * block_size;  // May overflow!

// ✅ CORRECT: Check for overflow
if (num_blocks > SIZE_MAX / block_size) {
    throw std::overflow_error("Size calculation would overflow");
}
size_t total_size = num_blocks * block_size;
```

---

## 🔍 Code Review Checklist

Before committing, verify:

- [ ] No hardcoded keys, passwords, or secrets
- [ ] All sensitive data is cleared after use
- [ ] Salt/nonce generation is unique per encryption
- [ ] Error messages don't leak sensitive information
- [ ] All public APIs have documentation
- [ ] Tests cover happy path and edge cases
- [ ] NIST test vectors pass (if applicable)
- [ ] No compiler warnings (-Wall -Wextra -Wpedantic)
- [ ] Valgrind clean (no memory leaks)
- [ ] clang-format applied
- [ ] Includes are minimal and necessary

---

## 🎯 Performance Considerations

```cpp
// ✅ GOOD: Reserve capacity
std::vector<uint8_t> result;
result.reserve(plaintext.size() + overhead);

// ✅ GOOD: Move instead of copy
return std::move(large_vector);

// ✅ GOOD: Use string_view for read-only strings
void process(std::string_view input);

// ✅ GOOD: Avoid unnecessary allocations
std::span<const uint8_t> get_data() const { return data_; }

// ❌ BAD: Return by value for large data
std::vector<uint8_t> get_data() const { return data_; }
```

---

## 📦 Build System Integration

### CMakeLists.txt Guidelines:

```cmake
# Always specify minimum versions
cmake_minimum_required(VERSION 3.20)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Use target-based approach
add_library(my_lib STATIC ${SOURCES})
target_include_directories(my_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(my_lib PUBLIC botan-3)
```

---

## 🚀 Quick Reference

### Botan 3.x Common Operations:

```cpp
// Random generation
Botan::AutoSeeded_RNG rng;
std::vector<uint8_t> random_data(32);
rng.randomize(random_data.data(), random_data.size());

// Hashing
auto hash = Botan::HashFunction::create("SHA-256");
hash->update(data);
auto result = hash->final();

// Key derivation
auto key = Botan::argon2_generate(32, password, salt, 65536, 4, 100000);

// Cipher
auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", Botan::Cipher_Dir::Encryption);
cipher->set_key(key);
cipher->start(nonce);
cipher->finish(buffer);
```

---

## ✅ Summary for Copilot

When writing code:
1. **Security first**: Unique nonces, clear sensitive data, constant-time ops
2. **Modern C++20**: Use concepts, ranges, span, format
3. **Botan 3.x**: Always use latest API patterns
4. **One file, one purpose**: Clean separation
5. **Test everything**: Unit + NIST + Security tests
6. **Document thoroughly**: Doxygen + references
7. **Handle errors**: Specific exceptions with context
8. **No warnings**: Clean compile with -Wall -Wextra -Wpedantic -Werror

**Remember:** This is cryptographic code. Mistakes can be catastrophic. When in doubt, be explicit, verbose, and safe.

---

# FileVault Troubleshooting Guide

## 🔧 Common Issues & Solutions

### Build Issues

#### 1. "Botan 3.x not found"

**Problem:**
```
CMake Error: Could not find Botan 3.x
```

**Solution:**
```bash
# Install Botan 3.x from source
git clone https://github.com/randombit/botan.git
cd botan
./configure.py --prefix=/usr/local
make -j$(nproc)
sudo make install

# Update library path
echo '/usr/local/lib' | sudo tee /etc/ld.so.conf.d/botan.conf
sudo ldconfig

# Verify installation
pkg-config --modversion botan-3
```

**Alternative (vcpkg):**
```bash
vcpkg install botan:x64-linux
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake ..
```

---

#### 2. "C++20 not supported"

**Problem:**
```
error: This file requires compiler and library support for the ISO C++ 2020 standard
```

**Solution:**
```bash
# Check compiler version
g++ --version  # Need GCC 10+
clang++ --version  # Need Clang 12+

# Update CMakeLists.txt
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force C++20
cmake -DCMAKE_CXX_STANDARD=20 ..
```

---

#### 3. "undefined reference to botan_*"

**Problem:**
```
undefined reference to `Botan::AutoSeeded_RNG::AutoSeeded_RNG()'
```

**Solution:**
```cmake
# Make sure to link Botan 3.x
find_package(botan REQUIRED)
target_link_libraries(your_target PRIVATE botan::botan-3)

# NOT botan-2!
```

**Manual linking:**
```bash
g++ -std=c++20 main.cpp -lbotan-3 -o filevault
```

---

### Runtime Issues

#### 4. "Algorithm not available"

**Problem:**
```cpp
auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", ...);
// cipher is nullptr
```

**Debug:**
```cpp
// List available algorithms
#include <botan/version.h>

auto modes = Botan::Cipher_Mode::providers("AES-256/GCM");
if (modes.empty()) {
    std::cerr << "AES-256/GCM not available\n";
    
    // Check what IS available
    for (const auto& algo : {"AES-128/GCM", "AES-256/CBC", "ChaCha20Poly1305"}) {
        auto test = Botan::Cipher_Mode::create(algo, Botan::Cipher_Dir::Encryption);
        std::cout << algo << ": " << (test ? "✓" : "✗") << "\n";
    }
}
```

**Solution:**
Check Botan build configuration:
```bash
# Rebuild Botan with all modules
./configure.py --minimized-build --enable-modules=aes,gcm,sha2,argon2
```

---

#### 5. "Invalid_IV_Length exception"

**Problem:**
```cpp
Botan::Invalid_IV_Length: AES-256/GCM requires exactly 12 byte nonce
```

**Solution:**
```cpp
// ❌ WRONG: Wrong nonce size
std::vector<uint8_t> nonce(16);  // Too big for GCM!

// ✅ CORRECT: Exact size
std::vector<uint8_t> nonce(12);  // 96 bits for GCM

// Different modes need different IV sizes:
// GCM: 12 bytes (96 bits)
// CBC: 16 bytes (128 bits) for AES
// ChaCha20: 8 or 12 bytes
```

---

#### 6. "Integrity_Failure on decrypt"

**Problem:**
```cpp
Botan::Integrity_Failure: GCM tag verification failed
```

**Causes:**
1. Wrong password/key
2. Corrupted ciphertext
3. Nonce not saved correctly
4. Tag not saved correctly

**Debug:**
```cpp
try {
    cipher->finish(buffer);
} catch (const Botan::Integrity_Failure& e) {
    std::cerr << "Authentication failed!\n";
    std::cerr << "Possible causes:\n";
    std::cerr << "- Wrong password\n";
    std::cerr << "- Corrupted file\n";
    std::cerr << "- Tampered data\n";
    
    // Check file format
    if (buffer.size() < 16) {
        std::cerr << "Buffer too small for GCM tag\n";
    }
}
```

---

#### 7. Segmentation Fault

**Problem:**
```
Segmentation fault (core dumped)
```

**Debug with gdb:**
```bash
# Compile with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with gdb
gdb ./filevault
(gdb) run encrypt test.txt
(gdb) bt  # backtrace when crash

# Or catch crash
(gdb) catch throw
(gdb) run
```

**Debug with Valgrind:**
```bash
valgrind --leak-check=full \
         --track-origins=yes \
         --show-leak-kinds=all \
         ./filevault encrypt test.txt
```

**Common causes:**
```cpp
// ❌ Dereferencing nullptr
auto cipher = Botan::Cipher_Mode::create("BadAlgo", ...);
cipher->set_key(key);  // CRASH if cipher is null!

// ✅ Check before use
if (!cipher) {
    throw std::runtime_error("Failed to create cipher");
}

// ❌ Buffer overflow
std::vector<uint8_t> buffer(10);
cipher->finish(buffer);  // May need more space!

// ✅ Reserve enough space
size_t min_size = cipher->minimum_final_size();
buffer.reserve(buffer.size() + min_size);
```

---

### Security Issues

#### 8. "Same ciphertext for same plaintext"

**Problem:**
```cpp
auto ct1 = encrypt("test", "password");
auto ct2 = encrypt("test", "password");
// ct1 == ct2  ← SECURITY BUG!
```

**Cause:** Reusing nonce/IV

**Solution:**
```cpp
// ✅ Generate new nonce EVERY encryption
CryptoResult encrypt(const std::vector<uint8_t>& plaintext,
                    const std::string& password) {
    Botan::AutoSeeded_RNG rng;
    
    // NEW nonce every call
    std::vector<uint8_t> nonce(12);
    rng.randomize(nonce.data(), nonce.size());
    
    // ... encrypt with this nonce
}
```

**Verify:**
```cpp
TEST_CASE("Unique ciphertexts") {
    for (int i = 0; i < 100; ++i) {
        auto ct = encrypt("same plaintext", "same password");
        all_ciphertexts.insert(ct);
    }
    
    // Must have 100 unique ciphertexts
    REQUIRE(all_ciphertexts.size() == 100);
}
```

---

#### 9. Memory not cleared

**Problem:**
Password/key still in memory after use

**Check:**
```bash
# Run with gdb and check memory
gdb ./filevault
(gdb) break after_encryption
(gdb) run
(gdb) x/32xb password_address  # Should be all zeros
```

**Solution:**
```cpp
// ✅ Use SecureVector
Botan::secure_vector<uint8_t> key(32);
// Automatically zeroed on destruction

// ✅ Manual clearing
void clear_sensitive_data(std::vector<uint8_t>& data) {
    #ifdef _WIN32
        SecureZeroMemory(data.data(), data.size());
    #else
        volatile uint8_t* p = data.data();
        for (size_t i = 0; i < data.size(); ++i) {
            p[i] = 0;
        }
    #endif
    data.clear();
}
```

---

### Performance Issues

#### 10. "Encryption is too slow"

**Benchmark:**
```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
// ... encryption code ...
auto end = std::chrono::high_resolution_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Time: " << duration.count() << " ms\n";

// Calculate throughput
double mb_per_sec = (file_size_bytes / 1024.0 / 1024.0) / 
                    (duration.count() / 1000.0);
std::cout << "Throughput: " << mb_per_sec << " MB/s\n";
```

**Optimization tips:**
```cpp
// 1. Use hardware acceleration
if (Botan::CPUID::has_aes_ni()) {
    // AES-NI will be used automatically
    std::cout << "AES-NI available\n";
}

// 2. Choose fast algorithm
"ChaCha20Poly1305"  // ~800 MB/s (software)
"AES-256/GCM"       // ~500 MB/s (software), 2-4 GB/s (AES-NI)

// 3. Reduce KDF iterations (ONLY for testing!)
// Production: 100,000+
// Testing: 10,000

// 4. Process in chunks
const size_t CHUNK_SIZE = 1024 * 1024;  // 1MB
for (size_t offset = 0; offset < total; offset += CHUNK_SIZE) {
    process_chunk(offset, CHUNK_SIZE);
}

// 5. Parallel processing
#pragma omp parallel for
for (size_t i = 0; i < num_files; ++i) {
    encrypt_file(files[i]);
}
```

---

#### 11. "Out of memory"

**Problem:**
```
std::bad_alloc
terminate called after throwing an instance of 'std::bad_alloc'
```

**Solution:**
```cpp
// ❌ BAD: Load entire file to memory
std::vector<uint8_t> file_data = read_entire_file(path);  // 10GB file!

// ✅ GOOD: Stream processing
void encrypt_large_file(const std::string& input, 
                       const std::string& output) {
    std::ifstream in(input, std::ios::binary);
    std::ofstream out(output, std::ios::binary);
    
    const size_t CHUNK = 1024 * 1024;  // 1MB chunks
    std::vector<uint8_t> buffer(CHUNK);
    
    auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", ...);
    cipher->start(nonce);
    
    while (in.read(reinterpret_cast<char*>(buffer.data()), CHUNK)) {
        size_t bytes_read = in.gcount();
        Botan::secure_vector<uint8_t> chunk(buffer.begin(), 
                                            buffer.begin() + bytes_read);
        cipher->update(chunk);
        out.write(reinterpret_cast<const char*>(chunk.data()), 
                 chunk.size());
    }
    
    cipher->finish(buffer);
    out.write(reinterpret_cast<const char*>(buffer.data()), 
             buffer.size());
}
```

---

### File Format Issues

#### 12. "Can't decrypt own encrypted files"

**Problem:**
File encrypted with FileVault can't be decrypted

**Debug:**
```cpp
// Verify file format
void verify_file_format(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    
    // Check magic header
    char magic[8];
    file.read(magic, 8);
    
    if (std::memcmp(magic, "FVAULT01", 8) != 0) {
        std::cerr << "Invalid magic header: ";
        for (int i = 0; i < 8; ++i) {
            std::cerr << std::hex << (int)(uint8_t)magic[i] << " ";
        }
        std::cerr << "\n";
        return;
    }
    
    // Read version
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), 4);
    std::cout << "Version: " << version << "\n";
    
    // Read algorithm ID
    uint32_t algo_id;
    file.read(reinterpret_cast<char*>(&algo_id), 4);
    std::cout << "Algorithm: " << algo_id << "\n";
    
    // Continue parsing...
}
```

**Common mistakes:**
```cpp
// ❌ WRONG: Not saving salt
auto encrypted = encrypt(data, key, nonce);
write_file(output, encrypted);  // Lost salt and nonce!

// ✅ CORRECT: Save metadata
struct FileHeader {
    char magic[8] = {'F','V','A','U','L','T','0','1'};
    uint32_t version = 1;
    uint32_t algo_id;
    uint32_t salt_length;
    std::vector<uint8_t> salt;
    uint32_t nonce_length;
    std::vector<uint8_t> nonce;
    // ...
};

void write_encrypted_file(const std::string& path,
                         const FileHeader& header,
                         const std::vector<uint8_t>& ciphertext) {
    std::ofstream file(path, std::ios::binary);
    
    // Write header
    file.write(header.magic, 8);
    file.write(reinterpret_cast<const char*>(&header.version), 4);
    // ... write all fields
    
    // Write ciphertext
    file.write(reinterpret_cast<const char*>(ciphertext.data()),
              ciphertext.size());
}
```

---

### Testing Issues

#### 13. "NIST test vectors failing"

**Problem:**
```
Test failed: AES-256-GCM NIST vector
Expected: a1b2c3...
Got:      d4e5f6...
```

**Debug:**
```cpp
void debug_encryption(const std::vector<uint8_t>& plaintext,
                     const std::vector<uint8_t>& key,
                     const std::vector<uint8_t>& nonce) {
    std::cout << "=== Debug Encryption ===\n";
    
    std::cout << "Plaintext (" << plaintext.size() << " bytes):\n";
    print_hex(plaintext);
    
    std::cout << "Key (" << key.size() << " bytes):\n";
    print_hex(key);
    
    std::cout << "Nonce (" << nonce.size() << " bytes):\n";
    print_hex(nonce);
    
    auto cipher = Botan::Cipher_Mode::create("AES-256/GCM",
                                             Botan::Cipher_Dir::Encryption);
    cipher->set_key(key);
    
    Botan::secure_vector<uint8_t> buffer(plaintext.begin(), plaintext.end());
    cipher->start(nonce);
    cipher->finish(buffer);
    
    std::cout << "Ciphertext + Tag (" << buffer.size() << " bytes):\n";
    print_hex(buffer);
}

void print_hex(const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)data[i] << " ";
        if ((i + 1) % 16 == 0) std::cout << "\n";
    }
    std::cout << std::dec << "\n";
}
```

**Common issues:**
1. Wrong byte order (endianness)
2. Not including authentication tag in comparison
3. Using wrong mode (CBC instead of GCM)
4. Including extra data in comparison

---

## 🔍 Debugging Tools

### 1. Verbose Logging

```cpp
// Add logging macro
#ifdef DEBUG
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << "\n"
#else
#define LOG_DEBUG(msg)
#endif

// Usage
LOG_DEBUG("Derived key: " << hex_encode(key));
LOG_DEBUG("Nonce: " << hex_encode(nonce));
```

### 2. Memory Leak Detection

```bash
# Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./filevault encrypt test.txt

# Expected output for clean code:
# All heap blocks were freed -- no leaks are possible
```

### 3. AddressSanitizer

```bash
# Compile with sanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
make

# Run
./filevault encrypt test.txt

# Will catch:
# - Use after free
# - Buffer overflows
# - Memory leaks
```

### 4. UndefinedBehaviorSanitizer

```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g" ..
make

# Catches:
# - Integer overflow
# - Null pointer dereference
# - Invalid shifts
```

---

## 📞 Getting Help

### Before Asking

1. **Read error message carefully**
   - Note exact error text
   - Check line numbers
   - Look for stack trace

2. **Minimal reproducible example**
   ```cpp
   // Simplify to smallest code that shows problem
   #include <botan/cipher_mode.h>
   
   int main() {
       auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", ...);
       // ... minimal code that triggers issue
   }
   ```

3. **Gather information**
   ```bash
   # System info
   uname -a
   g++ --version
   cmake --version
   
   # Botan info
   pkg-config --modversion botan-3
   
   # Build info
   cat CMakeCache.txt | grep BOTAN
   ```

### Where to Ask

1. **FileVault Issues**: GitHub Issues
2. **Botan Questions**: https://github.com/randombit/botan/discussions
3. **C++ Questions**: Stack Overflow (tag: c++, cryptography)

### Bug Report Template

```markdown
**Environment:**
- OS: Ubuntu 22.04
- Compiler: GCC 11.3
- Botan version: 3.2.0
- FileVault commit: abc123

**Issue:**
Brief description

**Steps to Reproduce:**
1. Step 1
2. Step 2
3. Step 3

**Expected behavior:**
What should happen

**Actual behavior:**
What actually happens

**Code:**
```cpp
// Minimal example
```

**Error output:**
```
Paste full error message
```

**Additional context:**
Any other relevant information
```

---

## ✅ Prevention Checklist

Before committing code:

- [ ] Code compiles with no warnings (-Wall -Wextra -Wpedantic)
- [ ] All tests pass
- [ ] Valgrind shows no leaks
- [ ] AddressSanitizer shows no errors
- [ ] Code formatted (clang-format)
- [ ] Documentation updated
- [ ] NIST vectors pass (if applicable)
- [ ] Security review done
- [ ] Performance acceptable

---

## 🎯 Quick Fixes

| Problem | Quick Fix |
|---------|-----------|
| Botan not found | `export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig` |
| Wrong C++ version | `cmake -DCMAKE_CXX_STANDARD=20 ..` |
| Link error | Add `-lbotan-3` to link flags |
| Slow encryption | Check KDF iterations (100k max for testing) |
| Memory leak | Use `Botan::secure_vector` |
| Segfault | Check for nullptr before use |
| Auth failure | Verify nonce and tag are saved |
| Same ciphertext | Generate new nonce each time |

Remember: **Read the error message**, **check the docs**, **write a test**!

---

# FileVault Project Checklist

## 📋 Development Phases

### Phase 1: Core Infrastructure ✅

#### Setup (Week 1)
- [ ] Initialize Git repository
- [ ] Setup CMake build system
- [ ] Configure Conan for dependencies
- [ ] Install Botan 3.x
- [ ] Create basic project structure
- [ ] Setup CI/CD (GitHub Actions)
- [ ] Write README.md

#### Core Components (Week 2)
- [ ] Implement `CryptoEngine` class
- [ ] Create `ICryptoAlgorithm` interface
- [ ] Implement `FileHandler` for I/O
- [ ] Design file format specification
- [ ] Implement secure memory utilities
- [ ] Create error handling system

### Phase 2: Algorithms ⏳

#### Symmetric Encryption (Week 3)
- [ ] AES-128-GCM
- [ ] AES-192-GCM  
- [ ] AES-256-GCM ⭐ (Priority)
- [ ] ChaCha20-Poly1305
- [ ] Serpent-256-GCM
- [ ] Each with NIST test vectors

#### Classic Ciphers (Week 3-4)
- [ ] Caesar cipher
- [ ] Vigenère cipher
- [ ] Playfair cipher
- [ ] Substitution cipher
- [ ] Educational mode with visualization

#### Hash Functions (Week 4)
- [ ] SHA-256 ⭐
- [ ] SHA-512
- [ ] SHA3-256
- [ ] BLAKE2b
- [ ] HMAC support
- [ ] File checksum utility

#### Key Derivation (Week 4)
- [ ] Argon2id ⭐ (Priority)
- [ ] Argon2i
- [ ] PBKDF2-SHA256
- [ ] PBKDF2-SHA512
- [ ] scrypt
- [ ] Parameter tuning function

### Phase 3: Advanced Features ⏳

#### Compression (Week 5)
- [ ] zlib integration
- [ ] bzip2 integration
- [ ] LZMA integration
- [ ] Auto-detection on decompress
- [ ] Compression benchmarks

#### Security Enhancements (Week 5-6)
- [ ] Password strength checker
- [ ] Secure random generation
- [ ] Memory locking
- [ ] Constant-time operations
- [ ] Anti-rainbow table measures
- [ ] Secure deletion

#### Post-Quantum Crypto (Week 6)
- [ ] Kyber-768 (KEM)
- [ ] Dilithium-3 (Signature)
- [ ] Hybrid mode (Classical + PQC)
- [ ] Migration tools

### Phase 4: CLI & UX 🔄

#### Command Line Interface (Week 7)
- [ ] Argument parser (CLI11)
- [ ] `encrypt` command
- [ ] `decrypt` command
- [ ] `hash` command
- [ ] `compress` command
- [ ] `benchmark` command
- [ ] `list` command
- [ ] Help system
- [ ] Tab completion

#### User Experience (Week 7-8)
- [ ] Progress bars (indicators)
- [ ] Colored output (fmt)
- [ ] Pretty tables (tabulate)
- [ ] Logging system (spdlog)
- [ ] Interactive prompts
- [ ] Drag & drop support (GUI)
- [ ] Config file support (JSON)

### Phase 5: Testing 🧪

#### Unit Tests (Continuous)
- [ ] Test every algorithm
- [ ] Test file I/O
- [ ] Test error handling
- [ ] Test edge cases
- [ ] Code coverage >80%

#### Integration Tests (Week 8)
- [ ] Encrypt → Decrypt round-trip
- [ ] Multi-file operations
- [ ] Large file handling (>1GB)
- [ ] Cross-platform compatibility
- [ ] Stress testing

#### Security Tests (Week 9)
- [ ] NIST test vectors (all algorithms)
- [ ] Nonce uniqueness tests
- [ ] Salt uniqueness tests
- [ ] Timing attack tests
- [ ] Memory leak tests (Valgrind)
- [ ] Fuzz testing (24+ hours)

#### Performance Tests (Week 9)
- [ ] Encryption benchmarks
- [ ] KDF benchmarks
- [ ] Compression benchmarks
- [ ] Memory usage profiling
- [ ] Throughput measurements

### Phase 6: Documentation 📚

#### Code Documentation (Continuous)
- [ ] Doxygen comments for all public APIs
- [ ] Inline comments for complex logic
- [ ] Security notes for critical sections
- [ ] Reference links to standards

#### User Documentation (Week 10)
- [ ] Installation guide (all platforms)
- [ ] Quick start guide
- [ ] Usage examples
- [ ] CLI reference
- [ ] FAQ
- [ ] Troubleshooting guide

#### Developer Documentation (Week 10)
- [ ] Architecture overview
- [ ] Algorithm descriptions
- [ ] File format specification
- [ ] Contribution guidelines
- [ ] Security guidelines
- [ ] Botan 3.x reference

### Phase 7: GUI Development 🎨

#### Qt Desktop GUI (Week 11-12)
- [ ] Main window design
- [ ] Encrypt tab
- [ ] Decrypt tab
- [ ] Hash calculator tab
- [ ] Settings dialog
- [ ] About dialog
- [ ] Drag & drop support
- [ ] System tray integration
- [ ] Dark/light theme

#### Web UI (Optional - Week 13)
- [ ] React/Vue frontend
- [ ] Electron wrapper
- [ ] File upload/download
- [ ] Progress tracking
- [ ] Responsive design

### Phase 8: Extensions 🔌

#### VSCode Extension (Week 13-14)
- [ ] Extension scaffold
- [ ] Context menu integration
- [ ] Command palette commands
- [ ] WebView panels
- [ ] Native module binding
- [ ] Settings integration
- [ ] Marketplace submission

#### Browser Extension (Optional - Week 15)
- [ ] Chrome extension
- [ ] Firefox extension
- [ ] WebAssembly compilation
- [ ] Web-based encryption
- [ ] Store submission

### Phase 9: Polish & Release 🚀

#### Code Quality (Week 16)
- [ ] Code review
- [ ] Refactoring
- [ ] Performance optimization
- [ ] Memory optimization
- [ ] Remove dead code
- [ ] Update dependencies

#### Packaging (Week 16)
- [ ] Debian package (.deb)
- [ ] RPM package (.rpm)
- [ ] Windows installer (.msi)
- [ ] macOS DMG
- [ ] Homebrew formula
- [ ] AUR package (Arch)

#### Release (Week 17)
- [ ] Version tagging
- [ ] Release notes
- [ ] Binary builds (all platforms)
- [ ] GitHub release
- [ ] Website/landing page
- [ ] Demo video
- [ ] Blog post

---

## 🎯 Priority Features

### Must Have (Core)
1. ✅ AES-256-GCM encryption
2. ✅ Argon2id key derivation
3. ✅ Unique salt/nonce per file
4. ✅ File format with metadata
5. ✅ CLI with basic commands
6. ✅ NIST test vectors
7. ✅ Security guidelines
8. ✅ Cross-platform support

### Should Have (Polish)
9. ⏳ Progress indicators
10. ⏳ Password strength meter
11. ⏳ Compression support
12. ⏳ Multiple hash algorithms
13. ⏳ Benchmarking tools
14. ⏳ Educational mode
15. ⏳ Qt GUI

### Nice to Have (Advanced)
16. 📅 Post-quantum crypto
17. 📅 VSCode extension
18. 📅 Cloud sync (E2EE)
19. 📅 Mobile app
20. 📅 Hardware security module support

---

## 📊 Quality Metrics

### Code Quality
- [ ] No compiler warnings (-Wall -Wextra -Wpedantic -Werror)
- [ ] clang-tidy clean
- [ ] cppcheck clean
- [ ] Code coverage >80%
- [ ] Cyclomatic complexity <15

### Performance
- [ ] AES-GCM: >500 MB/s (software) or >2 GB/s (AES-NI)
- [ ] Argon2: <500ms for recommended params
- [ ] File I/O: >1 GB/s (SSD)
- [ ] Memory usage: <100MB for CLI
- [ ] Startup time: <1 second

### Security
- [ ] All NIST vectors pass
- [ ] Fuzz testing: 0 crashes in 24 hours
- [ ] Valgrind: 0 leaks
- [ ] AddressSanitizer: 0 errors
- [ ] Static analysis: 0 critical issues
- [ ] Security audit: passed

### Documentation
- [ ] Every public API documented
- [ ] User guide complete
- [ ] Installation tested on 3+ platforms
- [ ] 10+ usage examples
- [ ] Video tutorial

---

## 🔒 Security Checklist

### Critical Security Items
- [ ] ✅ Unique nonce per encryption (VERIFIED)
- [ ] ✅ Unique salt per file (VERIFIED)
- [ ] ✅ Secure memory clearing (IMPLEMENTED)
- [ ] ✅ Constant-time comparison (IMPLEMENTED)
- [ ] ✅ No hardcoded secrets (VERIFIED)
- [ ] ✅ AEAD mode only (GCM/ChaCha20-Poly1305)
- [ ] ✅ Minimum 256-bit keys
- [ ] ✅ Minimum 100k KDF iterations

### Security Testing
- [ ] NIST test vectors: 100% pass
- [ ] Timing attack tests: passed
- [ ] Memory dump tests: no leaks
- [ ] Fuzz testing: 0 crashes
- [ ] Penetration testing: scheduled
- [ ] Third-party audit: scheduled

---

## 📱 Platform Support

### Desktop
- [ ] Linux (Ubuntu 20.04+, Fedora 35+, Arch)
- [ ] macOS (11+, Intel + Apple Silicon)
- [ ] Windows (10+, x64)

### Mobile (Future)
- [ ] Android 10+
- [ ] iOS 14+

### Web (Future)
- [ ] Chrome 90+
- [ ] Firefox 88+
- [ ] Safari 14+

---

## 🧪 Testing Matrix

### Algorithms
| Algorithm | Unit Test | NIST Vectors | Security Test | Performance |
|-----------|-----------|--------------|---------------|-------------|
| AES-128-GCM | ✅ | ✅ | ✅ | ✅ |
| AES-256-GCM | ✅ | ✅ | ✅ | ✅ |
| ChaCha20 | ⏳ | ⏳ | ⏳ | ⏳ |
| Argon2id | ✅ | N/A | ✅ | ✅ |
| SHA-256 | ✅ | ✅ | N/A | ✅ |

### Platforms
| Platform | Build | Unit Test | Integration | GUI |
|----------|-------|-----------|-------------|-----|
| Linux x64 | ✅ | ✅ | ✅ | ⏳ |
| macOS x64 | ⏳ | ⏳ | ⏳ | ⏳ |
| macOS ARM64 | ⏳ | ⏳ | ⏳ | ⏳ |
| Windows x64 | ⏳ | ⏳ | ⏳ | ⏳ |

---

## 📦 Deliverables

### Code
- [ ] Source code (GitHub)
- [ ] Binary releases (GitHub Releases)
- [ ] Docker image
- [ ] Package managers (apt, brew, chocolatey)

### Documentation
- [ ] User manual (PDF)
- [ ] Developer guide (PDF)
- [ ] API documentation (HTML)
- [ ] Video tutorials (YouTube)
- [ ] Website/landing page

### Academic
- [ ] Project report
- [ ] Architecture diagrams
- [ ] Security analysis
- [ ] Performance benchmarks
- [ ] Presentation slides
- [ ] Demo video

---

## ⏰ Timeline

```
Week 1-2:   Core Infrastructure ████████░░░░░░░░░░ 40%
Week 3-4:   Algorithms         ░░░░░░░░░░░░░░░░░░  0%
Week 5-6:   Advanced Features  ░░░░░░░░░░░░░░░░░░  0%
Week 7-8:   CLI & UX           ░░░░░░░░░░░░░░░░░░  0%
Week 9:     Testing            ░░░░░░░░░░░░░░░░░░  0%
Week 10:    Documentation      ░░░░░░░░░░░░░░░░░░  0%
Week 11-12: GUI                ░░░░░░░░░░░░░░░░░░  0%
Week 13-14: Extensions         ░░░░░░░░░░░░░░░░░░  0%
Week 15-16: Polish             ░░░░░░░░░░░░░░░░░░  0%
Week 17:    Release            ░░░░░░░░░░░░░░░░░░  0%
```

---

## 🎓 Learning Resources

### Cryptography
- [ ] "Serious Cryptography" - Jean-Philippe Aumasson
- [ ] "Cryptography Engineering" - Ferguson, Schneier, Kohno
- [ ] NIST guidelines
- [ ] OWASP cheat sheets

### C++
- [ ] "Effective Modern C++" - Scott Meyers
- [ ] "C++ Concurrency in Action" - Anthony Williams
- [ ] Botan documentation
- [ ] CppCon talks

### Security
- [ ] "The Art of Software Security Assessment"
- [ ] "Security Engineering" - Ross Anderson
- [ ] CVE database
- [ ] Security advisories

---

## ✅ Daily Checklist

Before ending each coding session:

- [ ] Code compiles with no warnings
- [ ] Tests pass
- [ ] Changes committed with good message
- [ ] Documentation updated (if needed)
- [ ] Code reviewed (self or peer)
- [ ] TODO comments added for future work

---

## 🎯 Definition of Done

A feature is considered "done" when:

1. ✅ Code is written and compiles
2. ✅ Unit tests written and passing
3. ✅ Integration tests passing
4. ✅ Security tests passing (if applicable)
5. ✅ Documentation written
6. ✅ Code reviewed
7. ✅ No compiler warnings
8. ✅ Valgrind clean
9. ✅ Merged to main branch

---

## 🚀 Release Criteria

Ready to release when:

- [ ] All "Must Have" features complete
- [ ] All tests passing (1000+ tests)
- [ ] Security audit passed
- [ ] Documentation complete
- [ ] Tested on all target platforms
- [ ] Performance benchmarks met
- [ ] No known critical bugs
- [ ] Version tagged
- [ ] Release notes written

---

**Last Updated:** 2024-11-15
**Current Phase:** Phase 1 - Core Infrastructure
**Progress:** 40% Complete
