# FileVault

🔐 Cross-platform CLI file encryption tool built with C++17

## Features

- **Multiple Encryption Algorithms**: AES-256-GCM, ChaCha20-Poly1305, DES (educational), classical ciphers
- **Strong Key Derivation**: Argon2id, PBKDF2
- **Optional Compression**: Zlib, Zstd
- **Educational & Production Modes**: From Caesar cipher to AES-256-GCM
- **Cross-Platform**: Windows, Linux, macOS

## Quick Start

### Installation

See detailed setup instructions in [`scripts/README.md`](scripts/README.md).

**Windows (PowerShell):**
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup\win\setup-windows.ps1
```

**Linux:**
```bash
bash scripts/setup/linux/setup-linux.sh
```

**macOS:**
```bash
bash scripts/setup/mac/setup-macos.sh
```

### Building

```bash
# Configure with Conan
conan install . --build=missing -s build_type=Release

# Build with CMake
cmake --preset conan-default
cmake --build --preset conan-release

# Or use platform-specific scripts:
# Windows: .\scripts\setup\win\build.ps1 -BuildType Release
# Linux/macOS: bash scripts/setup/linux/build.sh --type Release
```

### Usage

```bash
# Encrypt a file (standard mode: AES-256-GCM + Argon2id)
filevault encrypt secret.txt -o secret.fv

# Decrypt
filevault decrypt secret.fv -o decrypted.txt

# Advanced mode (maximum security + compression)
filevault encrypt large-file.iso --mode advanced -o encrypted.fv

# Educational mode (DES + PBKDF2, fast)
filevault encrypt demo.txt --mode basic

# View file metadata
filevault info secret.fv

# Compute file hash
filevault hash document.pdf --algorithm sha256

# Benchmark performance
filevault benchmark --algorithms aes256,chacha20 --sizes 10MB,100MB
```

## Architecture

**Layered Design:**
```
Infrastructure (Botan wrappers, file I/O)
    ↓
Domain (Cipher engines, KDF engines, compressors)
    ↓
Application (Services: EncryptionService, HashService)
    ↓
Presentation (CLI with CLI11, progress bars)
```

**Key Patterns:**
- **Strategy**: Runtime cipher selection (`ICipherEngine`, `IKDFEngine`)
- **Factory**: Object creation (`CipherFactory`, `KDFFactory`)
- **Builder**: File format construction (`FileFormatBuilder`)
- **RAII**: Secure memory (`SecureMemory` auto-wipes on destruction)

## Security Modes

| Mode | Cipher | KDF | Compression | Use Case |
|------|--------|-----|-------------|----------|
| **Basic** | DES-CBC | PBKDF2 (600K iterations) | None | Educational, fast |
| **Standard** | AES-256-GCM | Argon2id (64MB, 3 iterations) | Optional | Recommended |
| **Advanced** | AES-256-GCM | Argon2id (256MB, 5 iterations) | Zstd | Maximum security |

## File Format

`.fv` encrypted file structure:
```
[Header: 128+ bytes]
  - Magic bytes: 'FV'
  - Version: 1.0
  - Cipher type, mode
  - KDF type, parameters
  - Salt, IV/nonce
  - Original file size
[Encrypted Data]
[Authentication Tag (if GCM)]
```

## Dependencies

- **Botan** 3.2.0 - Cryptographic library
- **CLI11** 2.3.2 - Command-line parsing
- **spdlog** 1.13.0 - Logging
- **indicators** 2.3 - Progress bars
- **Catch2** 3.5.2 - Unit testing
- **Zlib** 1.3.1 - Compression

## Development

### Running Tests

```bash
cmake --build --preset conan-release --target test
# Or: ctest --preset conan-release
```

### Project Structure

```
FileVault/
├── include/filevault/        # Public API headers
│   ├── crypto/               # ICipherEngine, IKDFEngine
│   ├── compression/          # ICompressor
│   └── core/                 # FileFormat, SecureMemory
├── src/                      # Implementation
│   ├── crypto/cipher/        # AES, DES, classical ciphers
│   ├── crypto/kdf/           # PBKDF2, Argon2id
│   ├── compression/          # Zlib, Zstd wrappers
│   └── services/             # EncryptionService
├── cli/                      # CLI application
├── tests/                    # Unit & integration tests
└── docs/                     # Documentation
```

### Contributing

See [`docs/Plan.md`](docs/Plan.md) for detailed architecture and implementation guide.

## License

MIT License - see LICENSE file

## References

- [NIST AES-GCM Test Vectors](https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program)
- [Botan Crypto Library](https://botan.randombit.net/)
- [Argon2 Specification](https://github.com/P-H-C/phc-winner-argon2)

---

**Educational Notice**: This project includes classical ciphers (Caesar, Vigenère) for learning purposes. Use `--mode standard` or `--mode advanced` for actual security needs.
