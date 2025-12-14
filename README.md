# FileVault 🔐

**Professional cross-platform file encryption CLI tool** built with modern C++20 and Botan 3.x cryptographic library.

[![CI](https://github.com/vuongdat67/NT140.Q11.ANTT-Group15/actions/workflows/ci.yml/badge.svg)](https://github.com/vuongdat67/NT140.Q11.ANTT-Group15/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)



## ✨ Features

### 📚 Documentation link
- https://vuongdat67.github.io/FileVault_mkdocs/
- https://vuongdat67.github.io/NT140.Q11.ANTT-Group15/doxygen/html/md_README.html 
- https://vuongdat67.github.io/NT140.Q11.ANTT-Group15/doxygen/latex/FileVault-API.pdf 

### 🔒 Modern Encryption
- **AEAD Ciphers**: AES-GCM (128/192/256), ChaCha20-Poly1305, Serpent-GCM, Twofish-GCM
- **International Standards**: Camellia-GCM, ARIA-GCM (Korea), SM4-GCM (China)
- **Legacy Support**: AES-CBC/CTR/CFB/OFB/XTS, 3DES (for compatibility)
- **Asymmetric**: RSA (2048/3072/4096), ECC (P-256/P-384/P-521)
- **Post-Quantum Cryptography (PQC)**: 
  - Kyber-512/768/1024 (ML-KEM) - Key encapsulation
  - Dilithium-2/3/5 (ML-DSA) - Digital signatures
  - KyberHybrid - Quantum-resistant hybrid encryption
- **Classical** (educational): Caesar, Vigenère, Playfair, Hill, Substitution

### 🔑 Key Derivation
- **Argon2id** - Memory-hard, recommended
- **Scrypt** - Memory-hard alternative
- **PBKDF2** (SHA-256/SHA-512) - Legacy compatible

### 📦 Compression
- **ZLIB** - Fast, good ratio
- **LZMA** - Best ratio, slower
- **BZIP2** - Balanced (coming soon)

### 🎨 Additional Features
- **Steganography** - Hide data in images (LSB)
- **Archive** - Encrypt multiple files
- **Hashing** - SHA-256, SHA-512, SHA-3, BLAKE2b, BLAKE3
- **Benchmarks** - Performance testing

---

## 🚀 Quick Start

### Windows (MSVC)
```powershell
# Setup
.\scripts\setup-msvc.ps1

# Build
.\scripts\build-msvc.ps1 -Test
```

### Windows (MinGW/MSYS2)
```bash
# In MSYS2 UCRT64 terminal
./scripts/setup-mingw.sh
./scripts/build-mingw.sh -t
```

### Linux
```bash
./scripts/setup-linux.sh    # or --clang for Clang
./scripts/build-linux.sh -t
```

### macOS
```bash
./scripts/setup-macos.sh
./scripts/build-macos.sh -t
```

---

## 📖 Usage

### Basic Encryption
```bash
# Encrypt with AES-256-GCM (default)
filevault encrypt secret.txt

# Decrypt
filevault decrypt secret.txt.fvlt

# With options
filevault encrypt data.zip -a chacha20-poly1305 -s paranoid --compression lzma
```

### Mode Presets
```bash
filevault encrypt file.txt --mode basic      # AES-128-GCM, fast
filevault encrypt file.txt --mode standard   # AES-256-GCM, balanced
filevault encrypt file.txt --mode advanced   # ChaCha20-Poly1305, max security
```

### Asymmetric Encryption
```bash
# Generate RSA key pair
filevault keygen --algorithm rsa-4096 --output mykey

# Encrypt with public key
filevault encrypt secret.txt --pubkey mykey.pub

# Decrypt with private key
filevault decrypt secret.txt.fvlt --privkey mykey.pem
```

### Post-Quantum Cryptography (PQC)
```bash
# Generate Kyber keypair (quantum-resistant)
filevault keygen --algorithm kyber-1024 --output quantum-key

# Encrypt with Kyber-Hybrid (combines classical + PQC)
filevault encrypt secret.txt --algorithm kyber-1024-hybrid

# Generate Dilithium signing keypair
filevault keygen --algorithm dilithium-5 --output dilithium-key

# Hybrid encryption (recommended for quantum threat)
filevault encrypt data.zip -a kyber-1024-hybrid
```

### Steganography
```bash
# Hide data in image
filevault stego embed message.txt cover.png -o hidden.png

# Extract hidden data
filevault stego extract hidden.png -o recovered.txt

# Check capacity
filevault stego capacity photo.jpg
```

### Archive
```bash
# Create encrypted archive
filevault archive create documents/ -o backup.fva

# Extract archive
filevault archive extract backup.fva -o restored/

# List contents
filevault archive list backup.fva
```

### Hash
```bash
filevault hash document.pdf                    # SHA-256 (default)
filevault hash file.iso --algorithm blake2b   # BLAKE2b
filevault hash ./folder/ --recursive          # All files
```

### Benchmark
```bash
filevault benchmark                            # All algorithms
filevault benchmark --algorithm aes-256-gcm   # Specific
filevault benchmark --json -o results.json    # Export
```

### List & Info
```bash
filevault list algorithms    # Supported algorithms
filevault list kdfs          # Key derivation functions
filevault info encrypted.fvlt  # File metadata
```

---

## 🏗️ Building

### Requirements
- **C++20** compatible compiler
- **CMake** >= 3.20
- **Conan** 2.x
- **Ninja** (recommended)

### Supported Platforms

| Platform | Compiler | Status |
|----------|----------|--------|
| Linux | GCC 13+ | ✅ |
| Linux | Clang 17+ | ✅ |
| Windows | MSVC 2022/2026 | ✅ |
| Windows | MinGW GCC 14+ | ✅ |
| macOS | Apple Clang 16+ | ✅ |

### Manual Build
```bash
mkdir build && cd build
conan install .. --output-folder=. --build=missing
cmake --preset conan-release -DBUILD_TESTS=ON
cmake --build build/Release --parallel
```

See [docs/BUILD.md](docs/BUILD.md) for detailed instructions.

---

## 📊 Algorithm Comparison

### Symmetric (AEAD - Authenticated Encryption)

| Algorithm | Key Size | Speed | Security | Use Case |
|-----------|----------|-------|----------|----------|
| AES-256-GCM | 256-bit | ⚡⚡⚡ | ⭐⭐⭐⭐⭐ | General purpose |
| ChaCha20-Poly1305 | 256-bit | ⚡⚡⚡ | ⭐⭐⭐⭐⭐ | Mobile, no AES-NI |
| Serpent-256-GCM | 256-bit | ⚡⚡ | ⭐⭐⭐⭐⭐ | High security |
| Camellia-256-GCM | 256-bit | ⚡⚡⚡ | ⭐⭐⭐⭐⭐ | Japan standard |
| ARIA-256-GCM | 256-bit | ⚡⚡⚡ | ⭐⭐⭐⭐⭐ | Korea standard |
| SM4-GCM | 128-bit | ⚡⚡⚡ | ⭐⭐⭐⭐ | China standard |

### Asymmetric & Post-Quantum

| Algorithm | Key Size | Type | Security | Quantum Resistant |
|-----------|----------|------|----------|-------------------|
| RSA-4096 | 4096-bit | Classical | ⭐⭐⭐⭐ | ❌ Vulnerable |
| ECC-P521 | 521-bit | Classical | ⭐⭐⭐⭐⭐ | ❌ Vulnerable |
| Kyber-512 | N/A | PQC KEM | ⭐⭐⭐⭐ | ✅ NIST Level 1 |
| Kyber-768 | N/A | PQC KEM | ⭐⭐⭐⭐⭐ | ✅ NIST Level 3 |
| Kyber-1024 | N/A | PQC KEM | ⭐⭐⭐⭐⭐ | ✅ NIST Level 5 |
| KyberHybrid | Combined | PQC+AES | ⭐⭐⭐⭐⭐ | ✅ Defense in depth |
| Dilithium-5 | N/A | PQC Signature | ⭐⭐⭐⭐⭐ | ✅ NIST Level 5 |

### Key Derivation

| KDF | Memory | Speed | Resistance |
|-----|--------|-------|------------|
| Argon2id | 64MB+ | Slow | GPU, ASIC |
| Scrypt | 32MB+ | Slow | GPU |
| PBKDF2 | Minimal | Fast | Brute force only |

### Benchmark Results (1 MB data)

**Symmetric Encryption:**
- AES-256-GCM: ~700 MB/s (hardware accelerated)
- ChaCha20-Poly1305: ~600 MB/s (software optimized)
- Kyber-1024-Hybrid: ~650 MB/s (PQC + AES-GCM)

**Asymmetric Operations:**
- RSA-4096 Keygen: ~1.7 seconds
- ECC-P521 Keygen: ~10 ms
- Kyber-1024 Keygen: ~0.4 ms ⚡

**PQC Performance:**
- Kyber-1024 KEM Encapsulation: ~0.6 ms
- Kyber-1024 KEM Decapsulation: ~0.8 ms
- Dilithium-5 Sign: ~1.6 ms
- Dilithium-5 Verify: ~0.8 ms

---

## 🧪 Testing

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R "AES_GCM"

# With verbose output
ctest --test-dir build -V
```

### Test Categories
- **Unit tests** - Individual components
- **Integration tests** - Full encrypt/decrypt flow
- **Security tests** - Nonce uniqueness, timing attacks
- **NIST vectors** - Standard test vectors

---

## 📁 Project Structure

```
filevault/
├── include/filevault/     # Headers
│   ├── algorithms/        # Crypto algorithms
│   ├── cli/               # CLI commands
│   ├── core/              # Core types & engine
│   └── utils/             # Utilities
├── src/                   # Implementation
├── tests/                 # Test suites
├── scripts/               # Build scripts
└── docs/                  # Documentation
```

---

## 🔧 Configuration

### Security Levels

| Level | KDF Iterations | Memory | Description |
|-------|----------------|--------|-------------|
| weak | 3 | 64MB | Fast, testing |
| medium | 10 | 128MB | Balanced |
| strong | 20 | 256MB | Recommended |
| paranoid | 50 | 512MB | Maximum |

### Config File
```bash
filevault config set default-algorithm aes-256-gcm
filevault config set default-kdf argon2id
filevault config show
```

---

## 📚 Documentation

- [BUILD.md](docs/BUILD.md) - Build instructions
- [.github/copilot/](/.github/copilot/) - Architecture & coding standards

---

## 🤝 Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/amazing`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push: `git push origin feature/amazing`
5. Open Pull Request

---

## 📜 License

MIT License - see [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- [Botan](https://botan.randombit.net/) - Crypto library
- [CLI11](https://github.com/CLIUtils/CLI11) - CLI parser
- [spdlog](https://github.com/gabime/spdlog) - Logging
- [indicators](https://github.com/p-ranav/indicators) - Progress bars
