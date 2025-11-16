# FileVault Development Tasks

**Project:** FileVault - CLI File Encryption Tool  
**Language:** C++17/20  
**Started:** November 12, 2025  
**Status:** Skeleton/Planning Phase  

---

## 📋 PROJECT PHASES

### PHASE 0: Project Setup ✅ (Current)
**Goal:** Establish project structure, build system, and development environment

#### Task 0.1: Project Structure
- [x] Create `.github/copilot/` documentation folder
- [x] Create `conanfile.txt` with dependencies
- [ ] Create complete folder structure
  - [ ] `src/` - Implementation files
  - [ ] `include/filevault/` - Public headers
  - [ ] `cli/` - CLI application
  - [ ] `tests/` - Test files
  - [ ] `scripts/` - Build and setup scripts
  - [ ] `docs/` - Documentation
  - [ ] `build/` - Build output (gitignored)

#### Task 0.2: Cross-Platform Setup Scripts
- [ ] `scripts/setup/windows/setup-windows.ps1` - PowerShell setup
- [ ] `scripts/setup/linux/setup-linux.sh` - Bash setup for Linux
- [ ] `scripts/setup/macos/setup-macos.sh` - Bash setup for macOS
- [ ] `scripts/common/check-deps.sh` - Common dependency checker
- [ ] `.gitignore` - Ignore build artifacts
- [ ] `.clang-format` - Code formatting rules

#### Task 0.3: Build System Configuration
- [ ] Root `CMakeLists.txt` - Project configuration
- [ ] `src/CMakeLists.txt` - Library build
- [ ] `cli/CMakeLists.txt` - CLI executable
- [ ] `tests/CMakeLists.txt` - Test configuration
- [ ] CMake presets for cross-platform builds

#### Task 0.4: Documentation
- [ ] `README.md` - Project overview and quick start
- [ ] `docs/BUILD.md` - Detailed build instructions
- [ ] `docs/ARCHITECTURE.md` - System design
- [ ] `docs/API.md` - API reference
- [ ] `scripts/README.md` - Scripts documentation

---

### PHASE 1: Core Cryptography (Sprint 1-2) 🔐
**Goal:** Implement AES-GCM encryption/decryption with Botan

#### Task 1.1: Type Definitions & Exceptions
- [ ] `include/filevault/types.hpp`
  - [ ] Enum classes: `CipherType`, `KDFType`, `CompressionType`
  - [ ] Type aliases: `Bytes`, `SecureBytes`
  - [ ] Constants: Key sizes, block sizes, nonce lengths
- [ ] `include/filevault/exceptions.hpp`
  - [ ] `FileVaultException` - Base exception
  - [ ] `CryptoException` - Crypto errors
  - [ ] `FileFormatException` - Format errors
  - [ ] `AuthenticationException` - Wrong password

#### Task 1.2: Cipher Interfaces
- [ ] `include/filevault/crypto/cipher.hpp`
  - [ ] `ICipherEngine` - Abstract cipher interface
  - [ ] `CipherFactory` - Factory for creating ciphers
  - [ ] `CipherParams` - Encryption parameters struct

#### Task 1.3: AES Implementation with Botan
- [ ] `src/crypto/cipher/aes_engine.cpp`
  - [ ] `AESGCMEngine` class implementation
  - [ ] Botan API integration:
    - [ ] `Botan::Cipher_Mode::create("AES-256/GCM")`
    - [ ] `set_key()` method
    - [ ] `start()` with nonce
    - [ ] `finish()` for encryption/decryption
  - [ ] Nonce/IV generation with `Botan::AutoSeeded_RNG`
  - [ ] Authentication tag handling
  - [ ] Error handling for decryption failures

#### Task 1.4: Key Derivation Functions
- [ ] `include/filevault/crypto/kdf.hpp` - KDF interface
- [ ] `src/crypto/kdf/kdf_engines.cpp`
  - [ ] `PBKDF2Engine` implementation
    - [ ] Use `Botan::PBKDF2` with SHA-256
    - [ ] Default 100,000 iterations
    - [ ] 32-byte salt generation
  - [ ] `Argon2Engine` implementation (bonus)
    - [ ] Use `Botan::Argon2id` if available
    - [ ] Memory parameter: 64 MB
    - [ ] Time parameter: 3 iterations

#### Task 1.5: Secure Memory Management
- [ ] `include/filevault/core/secure_memory.hpp`
  - [ ] `SecureMemory` RAII class
  - [ ] Automatic zeroing on destruction
- [ ] `src/core/secure_memory.cpp`
  - [ ] Use `Botan::secure_scrub_memory()`
  - [ ] Volatile pointer technique

---

### PHASE 2: File Format & I/O (Sprint 3) 📁
**Goal:** Define and implement .fv file format

#### Task 2.1: File Format Specification
- [ ] `include/filevault/core/file_format.hpp`
  - [ ] `FileHeader` struct - Magic, version, metadata
  - [ ] `FileFormatBuilder` - Builder pattern for header
  - [ ] `FileFormatParser` - Parse header from bytes

#### Task 2.2: File Format Implementation
- [ ] `src/core/file_format.cpp`
  - [ ] Serialize header to bytes (little-endian)
  - [ ] Deserialize header from bytes
  - [ ] Validate magic number and version
  - [ ] Calculate and verify checksums

#### File Format Layout:
```
+----------------+
| Magic (2 bytes) | "FV"
| Version (2)     | 0x0001
| Cipher Type (1) | AES-256-GCM
| KDF Type (1)    | PBKDF2-SHA256
| Compression (1) | Zlib/None
| Reserved (1)    | Future use
+----------------+
| Salt (32 bytes) | KDF salt
| Nonce (12)      | AES-GCM nonce
| KDF Params (8)  | Iterations, memory
| Original Size(8)| Uncompressed size
| Compressed (8)  | Compressed size
+----------------+
| Encrypted Data  | Variable length
+----------------+
| Auth Tag (16)   | GCM tag
+----------------+
```

---

### PHASE 3: Compression (Sprint 3) 🗜️
**Goal:** Implement compression before encryption

#### Task 3.1: Compression Interface
- [ ] `include/filevault/compression/compressor.hpp`
  - [ ] `ICompressor` interface
  - [ ] `CompressorFactory`
  - [ ] Compression levels

#### Task 3.2: Zlib Wrapper
- [ ] `src/compression/zlib_compressor.cpp`
  - [ ] Compress with zlib (level 6 default)
  - [ ] Decompress with error handling
  - [ ] Use streaming for large files

#### Task 3.3: Zstd Wrapper (Optional)
- [ ] `src/compression/zstd_compressor.cpp`
  - [ ] Compress with Zstandard
  - [ ] Better compression ratio than zlib

---

### PHASE 4: Core Service (Sprint 4) 🎯
**Goal:** Main encryption/decryption service

#### Task 4.1: Encryption Service Interface
- [ ] `include/filevault/filevault.hpp`
  - [ ] `EncryptionService` class
  - [ ] `EncryptOptions` struct
  - [ ] `DecryptOptions` struct
  - [ ] Public API methods

#### Task 4.2: Encryption Service Implementation
- [ ] `src/services/encryption_service.cpp`
  - [ ] `encrypt_file()` method:
    1. Read input file
    2. Compress data (if enabled)
    3. Generate salt and nonce
    4. Derive key from password
    5. Encrypt with AES-GCM
    6. Build file header
    7. Write .fv file
  - [ ] `decrypt_file()` method:
    1. Read .fv file
    2. Parse header
    3. Derive key from password
    4. Decrypt with AES-GCM
    5. Verify authentication tag
    6. Decompress (if needed)
    7. Write output file
  - [ ] `get_file_info()` - Read metadata without decrypting
  - [ ] `verify_file()` - Check integrity

---

### PHASE 5: CLI Application (Sprint 5) 💻
**Goal:** User-friendly command-line interface

#### Task 5.1: CLI Structure
- [ ] `cli/main.cpp`
  - [ ] CLI11 app initialization
  - [ ] Global options: `--help`, `--version`, `--verbose`
  - [ ] Subcommands setup

#### Task 5.2: Core Commands
- [ ] `encrypt` command:
  ```bash
  filevault encrypt <file> [options]
    -o, --output <path>    # Output file
    -k, --keep             # Keep original
    -f, --force            # Overwrite existing
    --iterations <n>       # KDF iterations
    --compress             # Enable compression
    --secure-delete        # Wipe original
  ```
- [ ] `decrypt` command:
  ```bash
  filevault decrypt <file> [options]
    -o, --output <path>    # Output file
    -f, --force            # Overwrite existing
  ```

#### Task 5.3: Information Commands
- [ ] `info` command - Show file metadata
- [ ] `verify` command - Check integrity
- [ ] `check` command - Detect if file is encrypted

#### Task 5.4: Configuration Commands
- [ ] `config show` - Display settings
- [ ] `config set` - Modify defaults

#### Task 5.5: Password Handling
- [ ] Secure password input (no echo)
- [ ] Password confirmation for encryption
- [ ] Password strength checker

#### Task 5.6: Progress Indicators
- [ ] Progress bar with `indicators` library
- [ ] Percentage and speed display
- [ ] Auto-enable for files > 10MB

---

### PHASE 6: Testing (Sprint 6) 🧪
**Goal:** Comprehensive test coverage

#### Task 6.1: Unit Tests - Crypto
- [ ] `tests/unit/crypto/test_aes.cpp`
  - [ ] NIST test vectors for AES-GCM
  - [ ] Test all key sizes (128, 192, 256 bits)
  - [ ] Test authentication tag validation
  - [ ] Test wrong password detection
- [ ] `tests/unit/crypto/test_kdf.cpp`
  - [ ] PBKDF2 test vectors
  - [ ] Test iteration counts
  - [ ] Test salt uniqueness

#### Task 6.2: Unit Tests - File Format
- [ ] `tests/unit/core/test_file_format.cpp`
  - [ ] Header serialization/deserialization
  - [ ] Version compatibility
  - [ ] Magic number validation

#### Task 6.3: Unit Tests - Compression
- [ ] `tests/unit/compression/test_compression.cpp`
  - [ ] Compress/decompress round-trip
  - [ ] Test compression ratios
  - [ ] Test large file handling

#### Task 6.4: Integration Tests
- [ ] `tests/integration/test_encryption_workflow.cpp`
  - [ ] Full encrypt/decrypt cycle
  - [ ] Test with various file types
  - [ ] Test batch operations
- [ ] `tests/integration/test_cli.cpp`
  - [ ] CLI command execution
  - [ ] Error handling
  - [ ] Exit codes

#### Task 6.5: Test Data
- [ ] Generate test files (text, binary, large files)
- [ ] NIST test vector files
- [ ] Sample .fv files for parsing tests

---

### PHASE 7: Classical Ciphers (Optional) 🏛️
**Goal:** Educational/legacy cipher support

#### Task 7.1: Classical Cipher Interface
- [ ] `include/filevault/crypto/classical.hpp`
  - [ ] `IClassicalCipher` interface
  - [ ] Caesar, Vigenère, Playfair, Rail Fence

#### Task 7.2: Implementation
- [ ] `src/crypto/cipher/classical/caesar.cpp`
- [ ] `src/crypto/cipher/classical/vigenere.cpp`
- [ ] `src/crypto/cipher/classical/playfair.cpp`
- [ ] `src/crypto/cipher/classical/rail_fence.cpp`

#### Task 7.3: Tests
- [ ] `tests/unit/crypto/test_classical.cpp`
  - [ ] Known plaintext/ciphertext pairs
  - [ ] Edge cases (special characters, numbers)

---

### PHASE 8: Documentation & Polish (Sprint 7-8) 📚
**Goal:** Professional documentation and code quality

#### Task 8.1: API Documentation
- [ ] Add Doxygen comments to all public APIs
- [ ] Generate HTML documentation
- [ ] API usage examples

#### Task 8.2: User Documentation
- [ ] `docs/USER_GUIDE.md` - End-user manual
- [ ] `docs/SECURITY.md` - Security considerations
- [ ] `docs/TROUBLESHOOTING.md` - Common issues
- [ ] `docs/EXAMPLES.md` - Usage examples

#### Task 8.3: Developer Documentation
- [ ] `docs/CONTRIBUTING.md` - Contribution guidelines
- [ ] `docs/ARCHITECTURE.md` - System design
- [ ] `docs/ALGORITHMS.md` - Crypto algorithm details

#### Task 8.4: Code Quality
- [ ] Run `clang-format` on all files
- [ ] Run `cppcheck` static analysis
- [ ] Run `valgrind` memory leak detection
- [ ] Address all compiler warnings

#### Task 8.5: Performance
- [ ] Benchmark encryption speeds
- [ ] Optimize hot paths
- [ ] Profile memory usage
- [ ] Large file (> 1GB) testing

---

## 🎯 MILESTONES

### Milestone 1: "Crypto Foundation" (Week 2)
- ✅ Conan dependencies installed
- ✅ CMake builds successfully
- ✅ AES-GCM encryption works with Botan
- ✅ PBKDF2 key derivation works
- ✅ Basic unit tests pass

### Milestone 2: "File Operations" (Week 4)
- ✅ .fv file format implemented
- ✅ Compression working
- ✅ Full encrypt/decrypt cycle functional
- ✅ Integration tests pass

### Milestone 3: "CLI Complete" (Week 6)
- ✅ All CLI commands implemented
- ✅ Password input secure
- ✅ Progress bars working
- ✅ Error messages helpful

### Milestone 4: "Production Ready" (Week 8)
- ✅ All tests passing
- ✅ Documentation complete
- ✅ Cross-platform builds verified
- ✅ Performance benchmarks acceptable
- ✅ Code review completed

---

## 🔧 DEVELOPMENT WORKFLOW

### Daily Workflow
1. Pull latest changes: `git pull`
2. Create feature branch: `git checkout -b feature/task-name`
3. Implement task with tests
4. Run tests: `ctest --preset conan-release`
5. Format code: `clang-format -i <files>`
6. Commit: `git commit -m "feat: description"`
7. Push and create PR

### Build Workflow
```bash
# First time setup
./scripts/setup/windows/setup-windows.ps1  # Or linux/macos script

# Install dependencies
conan install . --build=missing -s build_type=Release

# Configure
cmake --preset conan-default

# Build
cmake --build --preset conan-release

# Test
ctest --preset conan-release

# Run
./build/Release/filevault --help
```

---

## 📊 PROGRESS TRACKING

### Phase 0: Setup
- [x] 10% - Initial structure
- [ ] 50% - All folders created
- [ ] 75% - Setup scripts working
- [ ] 100% - CMake builds successfully

### Phase 1: Crypto Core
- [ ] 0% - Not started

### Phase 2: File Format
- [ ] 0% - Not started

### Phase 3: Compression
- [ ] 0% - Not started

### Phase 4: Core Service
- [ ] 0% - Not started

### Phase 5: CLI
- [ ] 0% - Not started

### Phase 6: Testing
- [ ] 0% - Not started

### Overall Progress: 5%

---

## 🐛 KNOWN ISSUES

*(None yet - project just started)*

---

## 📝 NOTES

### Security Considerations
- Never log passwords or keys
- Always use secure random for salts/nonces
- Wipe sensitive data from memory after use
- Use constant-time comparison for auth tags
- Validate all inputs before processing

### Cross-Platform Notes
- Use `std::filesystem` for path handling
- Test on Windows, Linux, and macOS
- Use CMake for build portability
- Avoid platform-specific APIs

### Botan Tips
- Always check Botan documentation for API changes
- Use `Botan::secure_vector` for key material
- Handle exceptions from Botan operations
- Test with different Botan versions

---

**Last Updated:** November 12, 2025  
**Next Review:** End of Sprint 1
