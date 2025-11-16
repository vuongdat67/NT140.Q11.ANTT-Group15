# FileVault - Copilot Instructions

> **Purpose:** Complete development guide for GitHub Copilot to understand the entire FileVault project architecture, coding standards, and best practices.
>
> **Last Updated:** 2024-11-12  
> **Version:** 1.0  
> **Language:** C++17/20

---

## 📋 Table of Contents

1. [Project Overview](#1-project-overview)
2. [Technology Stack](#2-technology-stack)
3. [Project Architecture](#3-project-architecture)
4. [Folder Structure](#4-folder-structure)
5. [Coding Standards](#5-coding-standards)
6. [Common Patterns](#6-common-patterns)
7. [CLI Design](#7-cli-design)
8. [Cryptography Implementation](#8-cryptography-implementation)
9. [Testing Strategy](#9-testing-strategy)
10. [Error Handling](#10-error-handling)
11. [Build & Deployment](#11-build--deployment)
12. [Debugging Guide](#12-debugging-guide)
13. [Common Tasks](#13-common-tasks)
14. [API Reference](#14-api-reference)

---

## 1. Project Overview

### 1.1 What is FileVault?

**FileVault** is a cross-platform CLI file encryption tool written in C++17/20 that supports:
- Classical ciphers (Caesar, Vigenère, Playfair, Substitution, Rail Fence)
- Modern symmetric encryption (AES-128/192/256 with GCM, CBC, CTR, ECB modes)
- Key derivation (PBKDF2, Argon2, Argon2i)
- Compression (Zlib)
- Steganography (LSB in images)
- Multiple hash algorithms (MD5, SHA1, SHA2, SHA3, BLAKE2)

### 1.2 Target Users

1. **Students** learning cryptography (classical ciphers, educational modes)
2. **Developers** needing secure file encryption (production-ready AES-GCM)
3. **Security Researchers** benchmarking algorithms

### 1.3 Design Philosophy

```
✅ Security First: Default to AES-256-GCM + Argon2
✅ Simplicity: Minimal CLI commands, sensible defaults
✅ Extensibility: Plugin architecture for new algorithms
✅ Cross-platform: Windows/Linux/macOS with single codebase
✅ Educational: Verbose mode explains crypto operations
```

---

## 2. Technology Stack

### 2.1 Core Technologies

```yaml
Language: C++17 (minimum), C++20 (preferred)
Build System: CMake 3.20+
Build Tool: Ninja (faster than Make)
Package Manager: Conan 2.0
Compiler: GCC 11+, Clang 14+, MSVC 2022
```

### 2.2 Dependencies (via Conan)

```ini
# conanfile.txt
[requires]
botan/3.6.1              # Cryptography library (CRITICAL)
cli11/2.4.2              # CLI parsing (REQUIRED)
catch2/3.10.0            # Unit testing (REQUIRED)
spdlog/1.14.1            # Logging (REQUIRED)
fmt/11.0.2               # String formatting (spdlog dependency)
indicators/2.3           # Progress bars (REQUIRED)
zlib/1.3.1               # Compression (REQUIRED)
benchmark/1.9.0          # Performance testing (REQUIRED)
```

### 2.3 Manual Dependencies (third_party/)

```
stb_image.h              # Image I/O for steganography
stb_image_write.h        # Image writing
```

### 2.4 Development Tools

```yaml
Linter: clang-tidy
Formatter: clang-format (Google style modified)
Static Analysis: cppcheck
Memory Checker: Valgrind (Linux), AddressSanitizer (all platforms)
IDE: VSCode with C++ extension, CLion
```

---

## 3. Project Architecture

### 3.1 Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    CLI Layer (cli/)                          │
│  - Command parsing (CLI11)                                   │
│  - User interaction (prompts, progress bars)                 │
│  - Output formatting (colors, tables)                        │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│              Application Layer (src/services/)               │
│  - EncryptionService                                         │
│  - CompressionService                                        │
│  - SteganographyService                                      │
│  - BenchmarkService                                          │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                 Domain Layer (src/crypto/)                   │
│  - CipherEngine (interface)                                 │
│  - KDFEngine (interface)                                    │
│  - Compressor (interface)                                   │
│  - FileFormatHandler                                         │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│           Infrastructure Layer (Botan, filesystem)           │
│  - Botan crypto primitives                                   │
│  - File I/O (std::filesystem)                                │
│  - Image I/O (stb_image)                                     │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Design Patterns

**Strategy Pattern** - Algorithm selection:
```cpp
class ICipherEngine { virtual Bytes encrypt(...) = 0; };
class AESEngine : public ICipherEngine { ... };
class DESEngine : public ICipherEngine { ... };
```

**Factory Pattern** - Object creation:
```cpp
auto cipher = CipherFactory::create(CipherType::AES256, CipherMode::GCM);
```

**Builder Pattern** - File format construction:
```cpp
auto file = FileFormatBuilder()
    .set_cipher(CipherType::AES256, CipherMode::GCM)
    .set_kdf(KDFType::Argon2id, params)
    .build();
```

**RAII** - Resource management:
```cpp
class SecureMemory {
    ~SecureMemory() { OPENSSL_cleanse(data_.data(), data_.size()); }
};
```

---

## 4. Folder Structure

```
filevault/
│
├── .github/
│   └── workflows/
│       ├── build.yml              # CI: Multi-platform build
│       ├── test.yml               # CI: Run tests
│       └── release.yml            # CI: Create releases
│
├── .vscode/
│   ├── settings.json              # Editor config
│   ├── launch.json                # Debug config
│   └── tasks.json                 # Build tasks
│
├── cmake/
│   ├── CompilerWarnings.cmake     # -Wall -Wextra -Werror
│   ├── Sanitizers.cmake           # ASAN, UBSAN, MSAN
│   └── StaticAnalyzers.cmake      # clang-tidy, cppcheck
│
├── docs/
│   ├── algorithms/                # Algorithm documentation (Obsidian)
│   ├── api/                       # API reference
│   ├── design/                    # Architecture, file format
│   └── user-guide/                # CLI usage guide
│
├── include/filevault/             # Public headers
│   ├── filevault.hpp              # Main API
│   ├── types.hpp                  # Enums, aliases
│   ├── exceptions.hpp             # Exception hierarchy
│   ├── crypto/
│   │   ├── cipher.hpp
│   │   ├── kdf.hpp
│   │   └── hash.hpp
│   ├── compression/
│   │   └── compressor.hpp
│   ├── stego/
│   │   └── steganography.hpp
│   └── core/
│       ├── file_format.hpp
│       └── secure_memory.hpp
│
├── src/                           # Implementation
│   ├── crypto/
│   │   ├── cipher/
│   │   │   ├── aes_engine.cpp
│   │   │   ├── des_engine.cpp
│   │   │   └── classical/        # Caesar, Vigenère, etc.
│   │   ├── kdf/
│   │   │   ├── pbkdf2_engine.cpp
│   │   │   └── argon2_engine.cpp
│   │   ├── hash/
│   │   │   └── hash_service.cpp
│   │   └── factory.cpp
│   ├── compression/
│   │   └── zlib_compressor.cpp
│   ├── stego/
│   │   └── lsb_steganography.cpp
│   ├── core/
│   │   ├── file_format.cpp
│   │   └── file_handler.cpp
│   ├── services/
│   │   └── encryption_service.cpp
│   └── utils/
│       ├── hex.cpp
│       └── string_utils.cpp
│
├── cli/                           # CLI application
│   ├── main.cpp
│   ├── app.hpp/cpp
│   ├── commands/
│   │   ├── encrypt_cmd.cpp
│   │   ├── decrypt_cmd.cpp
│   │   ├── info_cmd.cpp
│   │   └── benchmark_cmd.cpp
│   └── ui/
│       ├── progress.cpp
│       └── colors.cpp
│
├── tests/
│   ├── unit/
│   │   ├── crypto/
│   │   │   ├── test_aes_nist.cpp
│   │   │   └── test_classical.cpp
│   │   └── core/
│   │       └── test_file_format.cpp
│   ├── integration/
│   │   └── test_encrypt_decrypt.cpp
│   └── fixtures/
│       ├── sample.txt
│       └── test_image.png
│
├── benchmarks/
│   └── bench_crypto.cpp
│
├── scripts/
│   ├── build.sh
│   ├── test.sh
│   ├── format.sh
│   └── analyze.sh
│
├── third_party/
│   ├── stb_image.h
│   └── stb_image_write.h
│
├── .clang-format
├── .clang-tidy
├── .gitignore
├── CMakeLists.txt
├── conanfile.txt
├── copilot-instructions.md        # This file
├── LICENSE
├── README.md
└── CHANGELOG.md
```

---

## 5. Coding Standards

### 5.1 C++ Style Guide

**Base Style:** Google C++ Style Guide (modified)

**Key Rules:**

```cpp
// 1. Naming Conventions
namespace filevault { }             // lowercase, no underscores
class CipherEngine { };             // PascalCase
void encrypt_file();                // snake_case for functions
int iteration_count_;               // snake_case + trailing underscore for members
const int kMaxFileSize = 1024;      // kCamelCase for constants

// 2. Pointer/Reference Alignment
int* ptr;                           // Left alignment
const std::string& name;            // Left alignment

// 3. Indentation
// 4 spaces (not 2)
void function() {
    if (condition) {
        do_something();
    }
}

// 4. Line Length
// 100 characters max (not 80)

// 5. Braces
void function() {                   // Opening brace on same line
    // ...
}

// 6. Comments
// Use // for single-line comments
/* Use block comments for multi-line */

/**
 * @brief Doxygen-style comments for public APIs
 * @param input Input data
 * @return Encrypted data
 */
```

### 5.2 File Organization

```cpp
// header.hpp
#pragma once                        // Use #pragma once (not include guards)

#include <system_headers>           // System headers first
#include <library_headers>          // Library headers second
#include "filevault/headers.hpp"    // Project headers last

namespace filevault {

// Forward declarations
class ForwardDeclared;

// Constants
constexpr int kBufferSize = 4096;

// Type aliases
using Bytes = std::vector<uint8_t>;
using SecureBytes = Botan::secure_vector<uint8_t>;

// Enums
enum class CipherType : uint8_t {
    AES256 = 0x01,
    AES192 = 0x02,
    // ...
};

// Class declaration
class MyClass {
public:
    // Public methods
    
private:
    // Private methods
    // Private members
};

} // namespace filevault
```

### 5.3 Modern C++ Features (Preferred)

```cpp
// ✅ Use auto when type is obvious
auto cipher = CipherFactory::create(type);

// ✅ Use nullptr (not NULL)
CipherEngine* ptr = nullptr;

// ✅ Use std::make_unique/std::make_shared
auto ptr = std::make_unique<AESEngine>();

// ✅ Use range-based for loops
for (const auto& byte : data) { }

// ✅ Use std::filesystem (C++17)
namespace fs = std::filesystem;
if (fs::exists(path)) { }

// ✅ Use std::optional (C++17)
std::optional<int> find_value();

// ✅ Use structured bindings (C++17)
auto [key, value] = map.find(id);

// ✅ Use constexpr for compile-time constants
constexpr int kMaxSize = 1024;

// ✅ Use [[nodiscard]] for important return values
[[nodiscard]] bool validate_input();

// ❌ Avoid raw pointers (use smart pointers)
// MyClass* ptr = new MyClass();  // BAD
auto ptr = std::make_unique<MyClass>();  // GOOD

// ❌ Avoid C-style casts
// int x = (int)value;  // BAD
int x = static_cast<int>(value);  // GOOD
```

---

## 6. Common Patterns

### 6.1 Type Definitions

```cpp
// include/filevault/types.hpp

namespace filevault {

// Byte containers
using Bytes = std::vector<uint8_t>;
using SecureBytes = Botan::secure_vector<uint8_t>;

// String types
using String = std::string;
using SecureString = std::string;  // TODO: Use secure allocator

// File paths
using Path = std::filesystem::path;

// Enumerations
enum class CipherType : uint8_t {
    // Classical
    CAESAR = 0x01,
    VIGENERE = 0x02,
    PLAYFAIR = 0x03,
    SUBSTITUTION = 0x04,
    RAIL_FENCE = 0x05,
    
    // Modern Symmetric
    AES128 = 0x10,
    AES192 = 0x11,
    AES256 = 0x12,
    DES = 0x13,
    TRIPLE_DES = 0x14,
};

enum class CipherMode : uint8_t {
    ECB = 0x01,
    CBC = 0x02,
    CTR = 0x03,
    GCM = 0x04,  // AEAD mode (recommended)
    CFB = 0x05,
    XTS = 0x06,
};

enum class KDFType : uint8_t {
    PBKDF2 = 0x01,
    ARGON2 = 0x02,
    ARGON2I = 0x03,
    ARGON2ID = 0x04,  // Recommended
};

enum class CompressionType : uint8_t {
    NONE = 0x00,
    ZLIB = 0x01,
    ZSTD = 0x02,  // Optional
};

enum class HashAlgorithm : uint8_t {
    MD5 = 0x01,
    SHA1 = 0x02,
    SHA256 = 0x03,
    SHA512 = 0x04,
    SHA3_256 = 0x05,
    SHA3_512 = 0x06,
    BLAKE2B = 0x07,
};

// Security modes (presets)
enum class SecurityMode {
    BASIC,      // Classical ciphers, PBKDF2, no compression
    STANDARD,   // AES-256-GCM, Argon2, no compression
    ADVANCED,   // AES-256-GCM, Argon2 (high params), Zlib
};

} // namespace filevault
```

### 6.2 Exception Hierarchy

```cpp
// include/filevault/exceptions.hpp

namespace filevault {

// Base exception
class FileVaultException : public std::exception {
protected:
    std::string message_;
    std::string context_;
    
public:
    explicit FileVaultException(std::string msg, std::string ctx = "")
        : message_(std::move(msg)), context_(std::move(ctx)) {}
    
    const char* what() const noexcept override { return message_.c_str(); }
    const std::string& context() const noexcept { return context_; }
};

// Specific exceptions
class CryptoException : public FileVaultException {
    using FileVaultException::FileVaultException;
};

class AuthenticationFailedException : public CryptoException {
public:
    AuthenticationFailedException()
        : CryptoException("Wrong password or corrupted file") {}
};

class FileNotFoundException : public FileVaultException {
public:
    explicit FileNotFoundException(const std::string& path)
        : FileVaultException("File not found", path) {}
};

class InvalidFormatException : public FileVaultException {
public:
    explicit InvalidFormatException(const std::string& reason)
        : FileVaultException("Invalid file format", reason) {}
};

class UnsupportedVersionException : public FileVaultException {
public:
    explicit UnsupportedVersionException(uint8_t version)
        : FileVaultException("Unsupported file version: " + std::to_string(version)) {}
};

class InsufficientPermissionException : public FileVaultException {
public:
    explicit InsufficientPermissionException(const std::string& path)
        : FileVaultException("Permission denied", path) {}
};

} // namespace filevault
```

### 6.3 Interface Pattern

```cpp
// include/filevault/crypto/cipher.hpp

namespace filevault::crypto {

// Abstract base class
class ICipherEngine {
public:
    virtual ~ICipherEngine() = default;
    
    // Core operations
    virtual Bytes encrypt(
        const Bytes& plaintext,
        const SecureBytes& key,
        const Bytes& iv_or_nonce
    ) = 0;
    
    virtual Bytes decrypt(
        const Bytes& ciphertext,
        const SecureBytes& key,
        const Bytes& iv_or_nonce
    ) = 0;
    
    // Metadata
    virtual CipherType get_type() const = 0;
    virtual CipherMode get_mode() const = 0;
    virtual size_t get_key_size() const = 0;
    virtual size_t get_block_size() const = 0;
    virtual size_t get_iv_size() const = 0;
    
    // Optional: Progress callback
    using ProgressCallback = std::function<void(double percent)>;
    virtual void set_progress_callback(ProgressCallback callback) {
        progress_callback_ = std::move(callback);
    }
    
protected:
    ProgressCallback progress_callback_;
};

// Concrete implementation
class AESEngine : public ICipherEngine {
public:
    explicit AESEngine(size_t key_bits, CipherMode mode);
    
    Bytes encrypt(const Bytes& plaintext, const SecureBytes& key,
                  const Bytes& iv_or_nonce) override;
    
    Bytes decrypt(const Bytes& ciphertext, const SecureBytes& key,
                  const Bytes& iv_or_nonce) override;
    
    CipherType get_type() const override { return cipher_type_; }
    CipherMode get_mode() const override { return cipher_mode_; }
    size_t get_key_size() const override { return key_size_; }
    size_t get_block_size() const override { return 16; }  // AES block size
    size_t get_iv_size() const override;
    
private:
    CipherType cipher_type_;
    CipherMode cipher_mode_;
    size_t key_size_;
    std::unique_ptr<Botan::Cipher_Mode> botan_cipher_;
};

// Factory
class CipherFactory {
public:
    static std::unique_ptr<ICipherEngine> create(
        CipherType type,
        CipherMode mode
    );
};

} // namespace filevault::crypto
```

### 6.4 Factory Pattern

```cpp
// src/crypto/factory.cpp

namespace filevault::crypto {

std::unique_ptr<ICipherEngine> CipherFactory::create(
    CipherType type,
    CipherMode mode
) {
    switch (type) {
        case CipherType::AES128:
            return std::make_unique<AESEngine>(128, mode);
        
        case CipherType::AES192:
            return std::make_unique<AESEngine>(192, mode);
        
        case CipherType::AES256:
            return std::make_unique<AESEngine>(256, mode);
        
        case CipherType::DES:
            return std::make_unique<DESEngine>(mode);
        
        case CipherType::CAESAR:
            return std::make_unique<CaesarCipher>();
        
        case CipherType::VIGENERE:
            return std::make_unique<VigenereCipher>();
        
        default:
            throw std::invalid_argument("Unsupported cipher type");
    }
}

} // namespace filevault::crypto
```

### 6.5 Botan Wrapper Pattern

```cpp
// src/crypto/cipher/aes_engine.cpp

namespace filevault::crypto {

AESEngine::AESEngine(size_t key_bits, CipherMode mode)
    : key_size_(key_bits / 8),
      cipher_mode_(mode) {
    
    // Determine cipher type
    if (key_bits == 128) cipher_type_ = CipherType::AES128;
    else if (key_bits == 192) cipher_type_ = CipherType::AES192;
    else if (key_bits == 256) cipher_type_ = CipherType::AES256;
    else throw std::invalid_argument("Invalid AES key size");
    
    // Create Botan cipher
    std::string algo_name = "AES-" + std::to_string(key_bits);
    
    switch (mode) {
        case CipherMode::GCM:
            algo_name += "/GCM";
            break;
        case CipherMode::CBC:
            algo_name += "/CBC/PKCS7";  // With padding
            break;
        case CipherMode::CTR:
            algo_name += "/CTR";
            break;
        case CipherMode::ECB:
            algo_name += "/ECB/PKCS7";
            break;
        default:
            throw std::invalid_argument("Unsupported cipher mode");
    }
    
    botan_cipher_ = Botan::Cipher_Mode::create(
        algo_name,
        Botan::Cipher_Dir::Encryption
    );
    
    if (!botan_cipher_) {
        throw CryptoException("Failed to create cipher: " + algo_name);
    }
}

Bytes AESEngine::encrypt(
    const Bytes& plaintext,
    const SecureBytes& key,
    const Bytes& iv_or_nonce
) {
    // Validate inputs
    if (key.size() != key_size_) {
        throw std::invalid_argument("Invalid key size");
    }
    
    if (iv_or_nonce.size() != get_iv_size()) {
        throw std::invalid_argument("Invalid IV/nonce size");
    }
    
    // Set key
    botan_cipher_->set_key(key.data(), key.size());
    
    // Start encryption
    botan_cipher_->start(iv_or_nonce.data(), iv_or_nonce.size());
    
    // Encrypt (in-place)
    Botan::secure_vector<uint8_t> buffer(plaintext.begin(), plaintext.end());
    botan_cipher_->finish(buffer);
    
    // Return as regular vector
    return Bytes(buffer.begin(), buffer.end());
}

} // namespace filevault::crypto
```

---

## 7. CLI Design

### 7.1 Command Structure

```bash
filevault <command> [options] <arguments>

Commands:
  encrypt         Encrypt a file
  decrypt         Decrypt a file
  info            Show file information
  verify          Verify file integrity
  benchmark       Benchmark algorithms
  config          Manage configuration
```

### 7.2 CLI Implementation Pattern

```cpp
// cli/app.hpp

namespace filevault::cli {

class App {
public:
    App();
    int run(int argc, char** argv);
    
private:
    void setup_global_options();
    void setup_commands();
    
    CLI::App cli_app_;
    std::vector<std::unique_ptr<CommandBase>> commands_;
    
    // Global options
    bool verbose_ = false;
    bool quiet_ = false;
    bool no_color_ = false;
};

} // namespace filevault::cli
```

```cpp
// cli/commands/command_base.hpp

namespace filevault::cli {

class CommandBase {
public:
    CommandBase(CLI::App& app, const std::string& name,
                const std::string& description, bool verbose)
        : verbose_(verbose) {
        subcommand_ = app.add_subcommand(name, description);
        setup();
        subcommand_->callback([this]() { execute(); });
    }
    
    virtual ~CommandBase() = default;
    
protected:
    virtual void setup() = 0;
    virtual void execute() = 0;
    
    CLI::App* subcommand_;
    bool verbose_;
};

} // namespace filevault::cli
```

### 7.3 Example Command Implementation

```cpp
// cli/commands/encrypt_cmd.cpp

namespace filevault::cli {

class EncryptCommand : public CommandBase {
public:
    EncryptCommand(CLI::App& app, bool verbose)
        : CommandBase(app, "encrypt", "Encrypt a file", verbose) {}
    
protected:
    void setup() override {
        subcommand_->add_option("input", input_file_, "Input file")
            ->required()
            ->check(CLI::ExistingFile);
        
        subcommand_->add_option("-o,--output", output_file_, "Output file");
        
        subcommand_->add_option("-a,--algorithm", algorithm_, "Algorithm")
            ->default_val("aes256")
            ->check(CLI::IsMember({"aes256", "aes192", "aes128", "des"}));
        
        subcommand_->add_option("-m,--mode", mode_, "Cipher mode")
            ->default_val("gcm")
            ->check(CLI::IsMember({"gcm", "cbc", "ctr", "ecb"}));
        
        subcommand_->add_option("-p,--password", password_, "Password");
        
        subcommand_->add_flag("--compress", compress_, "Enable compression");
        subcommand_->add_flag("--no-progress", no_progress_, "Hide progress");
    }
    
    void execute() override {
        try {
            // Prompt for password if not provided
            if (password_.empty()) {
                password_ = prompt_password("Enter password: ", true);
            }
            
            // Auto-generate output filename
            if (output_file_.empty()) {
                output_file_ = input_file_;
                output_file_.replace_extension(".enc");
            }
            
            // Create encryption service
            auto cipher_type = parse_cipher_type(algorithm_);
            auto cipher_mode = parse_cipher_mode(mode_);
            
            EncryptionService service(cipher_type, cipher_mode);
            
            // Setup progress callback
            ui::ProgressBar progress("Encrypting", fs::file_size(input_file_));
            auto callback = [&](double percent, const std::string& msg) {
                if (!no_progress_) progress.update(percent, msg);
            };
            
            // Encrypt
            service.encrypt_file(input_file_, output_file_, password_, callback);
            
            if (!no_progress_) progress.finish();
            
            // Show summary
            spdlog::info("✓ Encrypted: {} → {}", 
                        input_file_.string(), output_file_.string());
            
        } catch (const std::exception& e) {
            spdlog::error("❌ Encryption failed: {}", e.what());
            throw;
        }
    }
    
private:
    Path input_file_;
    Path output_file_;
    std::string algorithm_ = "aes256";
    std::string mode_ = "gcm";
    std::string password_;
    bool compress_ = false;
    bool no_progress_ = false;
};

} // namespace filevault::cli
```

---

## 8. Cryptography Implementation

### 8.1 Security Best Practices

```cpp
// ✅ ALWAYS use these practices:

// 1. Use AEAD modes (GCM) for authenticated encryption
auto cipher = CipherFactory::create(CipherType::AES256, CipherMode::GCM);

// 2. Generate random salt/IV for each encryption
Botan::AutoSeeded_RNG rng;
auto salt = rng.random_vec(32);  // 256-bit salt
auto nonce = rng.random_vec(12); // 96-bit nonce for GCM

// 3. Use strong KDF (Argon2id preferred)
auto kdf = KDFFactory::create(KDFType::ARGON2ID);
auto key = kdf->derive_key(password, salt, 32);  // 256-bit key

// 4. Wipe sensitive data from memory
void wipe_password(std::string& password) {
    OPENSSL_cleanse(&password[0], password.size());
    password.clear();
}

// 5. Use secure_vector for keys
Botan::secure_vector<uint8_t> key(32);  // Auto-wiped on destruction

// 6. Never reuse n