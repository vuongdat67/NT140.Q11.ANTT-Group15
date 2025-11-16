# FileVault - AI Agent Instructions

## Project Overview

FileVault is a cross-platform CLI file encryption tool written in C++17, designed for both educational and production use. The project implements multiple encryption algorithms from classical ciphers (Caesar, Vigenère) to modern cryptography (AES-256-GCM, Argon2id KDF).

**Current Stage:** Planning/Design phase - no implementation yet. All specifications are in `docs/Plan.md` and `docs/Prompt.md`.

## Architecture & Design Philosophy

### Layered Architecture (Bottom-Up)
```
Infrastructure → Domain → Application → Presentation (CLI)
```

- **Infrastructure Layer:** Botan crypto library wrappers, file I/O, logging (spdlog)
- **Domain Layer:** Cipher engines (ICipherEngine), KDF engines (IKDFEngine), compressors (ICompressor), file format handlers
- **Application Layer:** Services (EncryptionService, HashService, BenchmarkService)
- **Presentation Layer:** CLI parser (CLI11), interactive prompts, progress bars (indicators)

### Core Design Patterns

1. **Strategy Pattern** - Runtime cipher selection via interfaces (`ICipherEngine`, `IKDFEngine`)
2. **Factory Pattern** - `CipherFactory::create()` for object instantiation
3. **Builder Pattern** - `FileFormatBuilder` for constructing encrypted file format
4. **RAII** - `SecureMemory` class auto-wipes sensitive data on destruction

### File Format Structure

```
.fv file = [Header][Metadata][Encrypted Data][Auth Tag (if GCM)]
```

Header contains: magic bytes, version, cipher type, cipher mode, KDF type, salt, IV/nonce, KDF params.

## Key Implementation Conventions

### Security Practices

- **Password handling:** Use `Botan::secure_vector` (memory-locked, auto-wiped)
- **KDF defaults:** Argon2id (64MB memory, 3 iterations) for standard mode; PBKDF2 for basic mode
- **Cipher defaults:** AES-256-GCM (authenticated encryption) for standard/advanced modes
- **Nonce/IV:** MUST generate unique random value per encryption - reuse catastrophic for GCM
- **Compression:** ALWAYS compress before encryption (encrypted data is incompressible)

### Error Handling

- Custom exception hierarchy: `FileVaultException` → `CryptoException`, `FileNotFoundException`, etc.
- Use `safe_execute()` wrapper for user-friendly error messages
- CLI errors: Use `✗` prefix and colored output (red for errors)

### File Processing

- **Streaming encryption** for files >4MB using 4MB chunks
- Never load entire large files into memory
- Progress callbacks: `using ProgressCallback = std::function<void(double percent)>`

### Memory Management

```cpp
// Correct: Secure memory that auto-wipes
Botan::secure_vector<uint8_t> key = derive_key(password);

// Correct: RAII for resources
SecureMemory mem(size);  // Auto-wipes on scope exit
```

## Directory Structure (When Implementing)

```
filevault/
├── include/filevault/       # Public API headers
│   ├── crypto/              # ICipherEngine, IKDFEngine
│   ├── compression/         # ICompressor
│   └── core/                # FileFormat, SecureMemory
├── src/                     # Implementation
│   ├── crypto/cipher/       # aes_engine.cpp, des_engine.cpp, classical/
│   ├── crypto/kdf/          # pbkdf2_engine.cpp, argon2_engine.cpp
│   ├── compression/         # zlib_compressor.cpp, zstd_compressor.cpp
│   ├── core/                # file_format_handler.cpp, secure_memory.cpp
│   └── services/            # encryption_service.cpp
├── cli/                     # CLI application separate from library
│   ├── commands/            # encrypt_command.cpp, decrypt_command.cpp
│   └── ui/                  # progress_bar.cpp, interactive_prompt.cpp
├── tests/
│   ├── unit/                # Per-module tests matching src/ structure
│   ├── integration/         # End-to-end workflow tests
│   └── fixtures/            # NIST test vectors, sample files
├── docs/algorithms/         # Obsidian-style algorithm docs with wikilinks
└── benchmarks/              # Performance benchmarks
```

## CLI Command Patterns

### Mode System (Educational to Production)

- `--mode basic`: DES-CBC + PBKDF2 (educational, fast)
- `--mode standard`: AES-256-GCM + Argon2id (recommended)
- `--mode advanced`: AES-256-GCM + Argon2id (higher params) + Zstd compression

### Command Structure

```bash
filevault encrypt <input> [-o <output>] [--mode standard|advanced|basic]
filevault decrypt <encrypted-file> [-o <output>]
filevault hash <file> [--algorithm sha256|sha512|blake2b]
filevault stego embed <secret> <cover-image> [-o <output>]
filevault info <encrypted-file>  # Show metadata without decrypting
filevault benchmark [--algorithms aes256,chacha20] [--sizes 10MB,100MB]
```

## Development Workflow

### Environment Setup

**Automated setup scripts available for all platforms:**

```bash
# Windows (PowerShell)
powershell -ExecutionPolicy Bypass -File .\scripts\setup-windows.ps1

# Linux
bash scripts/setup-linux.sh

# macOS
bash scripts/setup-macos.sh
```

These scripts install: Git, CMake, Ninja, Python, Conan, C++ compilers (MSVC/GCC/Clang), clang-format, clang-tidy.

**Verify installation:**
```bash
# Windows
powershell -File .\scripts\verify-install.ps1

# Linux/macOS
bash scripts/verify-install.sh
```

### Building

```bash
# Windows (PowerShell)
.\scripts\build.ps1 -BuildType Release -Test

# Linux/macOS
bash scripts/build.sh --type Release --test
```

### Test-First Approach

1. **NIST test vectors** for crypto algorithms (see `docs/Plan.md` for AES-GCM example)
2. **Unit tests** per module in `tests/unit/` mirroring `src/` structure
3. **Integration tests** for encrypt→decrypt round-trips with compression, streaming

### Build System

- **CMake** with Conan for dependencies (Botan, CLI11, spdlog, indicators, Catch2)
- **Static linking** for CLI binary (single executable, no runtime deps)
- **Cross-platform** targets: Windows (MSVC), Linux (GCC/Clang), macOS

### Code Quality

- `.clang-format` for consistent style (run `scripts/format.sh`)
- `.clang-tidy` for static analysis (run `scripts/analyze.sh`)
- **Sanitizers** in debug builds: AddressSanitizer, UBSan

## Common Pitfalls to Avoid

1. **Nonce reuse in GCM** - Always generate fresh random nonce per encryption
2. **Encrypt-then-MAC vs MAC-then-Encrypt** - GCM handles both; for CBC use Encrypt-then-MAC
3. **Loading large files entirely** - Use streaming with 4MB chunks
4. **Weak password handling** - Use secure_vector, wipe after key derivation
5. **Platform-specific paths** - Use `std::filesystem` (C++17) for cross-platform paths

## Reference Key Files

- `docs/Plan.md` - Complete design document (3231 lines) with CLI design, OOP structure, Q&A guide, SDLC phases
- `docs/Prompt.md` - Original requirements and project vision
- `docs/algorithms/` (planned) - Algorithm documentation with Obsidian wikilinks

## Dependencies

- **Botan** (v3.x) - Cryptographic primitives (AES, Argon2, PBKDF2)
- **CLI11** - Command-line parsing
- **spdlog** - Logging
- **indicators** - Progress bars
- **Catch2** - Unit testing framework
- **Zstd** - Compression library (optional)
- **stb_image** - Image I/O for steganography

## Testing Strategy

- **Unit tests:** Test individual classes (AESEngine, PBKDF2Engine) against NIST vectors
- **Integration tests:** Full encrypt→compress→decrypt workflows
- **Benchmark tests:** Measure throughput (MB/s) for different algorithms
- **Cross-platform CI:** GitHub Actions running on Windows/Linux/macOS

## Questions to Ask When Implementing

1. **What is the security threat model?** (Local attacker? Physical access? Timing attacks?)
2. **What are the performance requirements?** (AES target: ≥100 MB/s on modern CPUs with AES-NI)
3. **Is backward compatibility needed?** (Yes - use versioned file format headers)
4. **How to handle password entropy?** (Warn users about weak passwords, support keyfiles)
5. **What file size limits?** (Support up to multi-GB files via streaming)

## Implementation Priority (When Ready to Code)

### Phase 1: Foundation (Week 1)
- Core: FileFormat parser, SecureMemory, FileHandler
- Unit tests for file I/O and secure memory wiping

### Phase 2: Basic Crypto (Week 2)
- Classical ciphers (Caesar, Vigenère) for `--mode basic`
- Tests against known plaintexts

### Phase 3: Modern Crypto (Week 3-4)
- AES-256-GCM with Botan wrapper
- Argon2id and PBKDF2 KDF engines
- NIST test vector validation

### Phase 4: CLI (Week 5-6)
- CLI11 integration for command parsing
- Interactive prompts, progress bars
- encrypt/decrypt/hash commands

### Phase 5: Advanced Features (Week 7-8)
- Compression (Zstd), steganography (LSB)
- Benchmark service, info command
- Polish UX and error handling

---

**Note:** This is a design-phase project. Always refer to `docs/Plan.md` for authoritative specifications before implementing any feature.
