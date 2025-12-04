# FileVault CLI Usage Guide

Complete usage examples for all FileVault commands.

## Table of Contents
- [Encryption & Decryption](#encryption--decryption)
- [Hash Operations](#hash-operations)
- [Compression](#compression)
- [Archive Operations](#archive-operations)
- [Steganography](#steganography)
- [Key Generation](#key-generation)
- [Configuration](#configuration)
- [List Algorithms](#list-algorithms)
- [Benchmark](#benchmark)
- [Info](#info)

---

## Encryption & Decryption

### Basic Encryption
```bash
# Encrypt a file with password prompt (recommended)
filevault encrypt document.txt

# Encrypt with specific algorithm
filevault encrypt document.txt -a aes-256-gcm

# Encrypt with password from command line (less secure)
filevault encrypt document.txt -p mypassword

# Encrypt with custom output filename
filevault encrypt document.txt encrypted_doc.fvlt -p mypassword

# Encrypt with compression
filevault encrypt document.txt --compression zlib -p mypassword
```

### Mode Presets
```bash
# Basic mode: Fast encryption, good security (casual use)
# - Algorithm: AES-256-GCM
# - KDF: PBKDF2-SHA256
# - Security: medium
filevault encrypt document.txt -m basic -p mypassword

# Standard mode: Balanced security and performance (recommended)
# - Algorithm: AES-256-GCM
# - KDF: Argon2id
# - Security: strong
filevault encrypt document.txt -m standard -p mypassword

# Advanced mode: Maximum security, slower (high-value data)
# - Algorithm: AES-256-GCM
# - KDF: Argon2id
# - Security: paranoid
# - Compression: LZMA
filevault encrypt document.txt -m advanced -p mypassword
```

### Decryption
```bash
# Decrypt a file with password prompt
filevault decrypt document.txt.fvlt

# Decrypt with custom output filename
filevault decrypt document.txt.fvlt original_document.txt -p mypassword

# Decrypt with verbose output
filevault decrypt document.txt.fvlt -p mypassword -v
```

### Advanced Encryption Options
```bash
# Custom security level
filevault encrypt document.txt -s paranoid -p mypassword

# Custom KDF (Key Derivation Function)
filevault encrypt document.txt -k argon2id -p mypassword

# Combine multiple options
filevault encrypt document.txt -a chacha20-poly1305 -k argon2id -s strong --compression lzma -p mypassword

# Disable progress bars
filevault encrypt large_file.dat -p mypassword --no-progress
```

---

## Hash Operations

### Generate Hash
```bash
# Hash a file with SHA-256 (default)
filevault hash document.txt

# Hash with specific algorithm
filevault hash document.txt -a sha256
filevault hash document.txt -a sha512
filevault hash document.txt -a sha3-256
filevault hash document.txt -a blake2b

# Legacy hash algorithms (not recommended for security)
filevault hash document.txt -a md5
filevault hash document.txt -a sha1

# Save hash to file
filevault hash document.txt -a sha256 -o document.hash
```

### Verify Hash
```bash
# Verify file against expected hash
filevault hash document.txt -a sha256 -v abc123def456...

# Verify and save result to file
filevault hash document.txt -a sha256 -v abc123def456... -o verify_result.txt
```

---

## Compression

### Compress Files
```bash
# Compress with default algorithm (zlib)
filevault compress large_file.txt

# Compress with specific algorithm
filevault compress large_file.txt -a zlib
filevault compress large_file.txt -a bzip2
filevault compress large_file.txt -a lzma

# Compress with custom output name
filevault compress large_file.txt -o compressed.dat -a lzma

# Set compression level (1-9, higher = better compression but slower)
filevault compress large_file.txt -a lzma -l 9
```

### Decompress Files
```bash
# Decompress (auto-detect algorithm from file header/extension)
filevault decompress large_file.txt.zlib
filevault decompress file.xz      # LZMA
filevault decompress file.bz2     # BZIP2

# Decompress with custom output name
filevault decompress compressed.dat -o original_file.txt

# Specify algorithm manually (if auto-detect fails)
filevault decompress file.dat -a zlib -o output.txt

# Using compress command with -d flag (alternative syntax)
filevault compress large_file.txt.zlib -d
```

---

## Archive Operations

### Create Archive
```bash
# Create encrypted archive from multiple files
filevault archive create file1.txt file2.txt file3.txt -o my_archive.fva -p mypassword

# Create archive with specific encryption algorithm
filevault archive create *.txt -o docs_archive.fva -p mypassword -a aes-256-gcm

# Create archive with compression
filevault archive create folder/*.txt -o archive.fva -p mypassword -c lzma

# Create archive with custom security settings
filevault archive create file1.txt file2.txt -o secure.fva -p mypassword -s paranoid -k argon2id
```

### Extract Archive
```bash
# Extract archive to current directory
filevault archive extract my_archive.fva -p mypassword

# Extract to specific directory
filevault archive extract my_archive.fva -o extracted_files/ -p mypassword

# Extract with verbose output
filevault archive extract my_archive.fva -p mypassword -v
```

### List Archive Contents
```bash
# List files in archive (currently extracts files)
filevault archive list my_archive.fva -p mypassword
```

---

## Steganography

### Hide Data in Image
```bash
# Embed secret file into image (positional arguments: secret cover output)
filevault stego embed secret.txt cover_image.png output_image.png

# The output image looks identical to the original but contains hidden data
```

### Extract Hidden Data
```bash
# Extract hidden data from image
filevault stego extract output_image.png extracted_secret.txt

# Verify extracted content matches original
cat extracted_secret.txt
```

### Check Image Capacity
```bash
# Check how much data an image can hide
filevault stego capacity cover_image.png

# Shows capacity at different bit levels (1-4 bits per channel)
```

---

## Key Generation

### Generate RSA Keys
```bash
# Generate RSA-2048 key pair
filevault keygen -a rsa-2048 -o my_rsa_key

# Generate RSA-3072 key pair
filevault keygen -a rsa-3072 -o my_rsa_key

# Generate RSA-4096 key pair
filevault keygen -a rsa-4096 -o my_rsa_key
```

### Generate ECC Keys
```bash
# Generate ECC P-256 key pair
filevault keygen -a ecc-p256 -o my_ecc_key

# Generate ECC P-384 key pair
filevault keygen -a ecc-p384 -o my_ecc_key

# Generate ECC P-521 key pair
filevault keygen -a ecc-p521 -o my_ecc_key
```

### Generate Post-Quantum Keys
```bash
# Generate Kyber-512 hybrid key pair (quantum-resistant)
filevault keygen -a kyber-512-hybrid -o my_kyber_key

# Generate Kyber-768 hybrid key pair
filevault keygen -a kyber-768-hybrid -o my_kyber_key

# Generate Kyber-1024 hybrid key pair (highest security)
filevault keygen -a kyber-1024-hybrid -o my_kyber_key

# Generate Dilithium digital signature key
filevault keygen -a dilithium -o my_dilithium_key
```

---

## Configuration

### View Configuration
```bash
# Show current configuration
filevault config show
```

### Set Configuration
```bash
# Set default encryption mode
filevault config set default.mode basic
filevault config set default.mode standard
filevault config set default.mode advanced

# Set default algorithm
filevault config set default.algorithm aes-256-gcm
filevault config set default.algorithm chacha20-poly1305
filevault config set default.algorithm aes-256-ctr

# Set default KDF
filevault config set default.kdf argon2id
filevault config set default.kdf pbkdf2-sha256
filevault config set default.kdf scrypt

# Set default compression
filevault config set default.compression none
filevault config set default.compression zlib
filevault config set default.compression lzma

# Set compression level (1-9)
filevault config set compression_level 6

# Enable/disable features
filevault config set show_progress true
filevault config set show_progress false
filevault config set verbose true
filevault config set verbose false
```

### Reset Configuration
```bash
# Reset to default settings
filevault config reset
```

---

## List Algorithms

### List All Available Algorithms
```bash
# Show all supported algorithms in tables
filevault list

# Shows:
# - Symmetric encryption algorithms (AEAD and non-AEAD)
# - Asymmetric encryption algorithms (RSA, ECC)
# - Post-Quantum algorithms (Kyber, Dilithium)
# - Hash functions (including MD5/SHA1 marked as BROKEN)
# - Classical ciphers (Caesar, Vigenere, etc.)
```

---

## Benchmark

### Performance Testing
```bash
# Benchmark all algorithms (default: 1MB, 5 iterations)
filevault benchmark

# Benchmark specific algorithm
filevault benchmark -a aes-256-gcm

# Benchmark all symmetric algorithms only
filevault benchmark --symmetric

# Benchmark all asymmetric algorithms only
filevault benchmark --asymmetric

# Benchmark all post-quantum algorithms only
filevault benchmark --pqc

# Custom data size and iterations
filevault benchmark -s 10485760 -i 10  # 10MB, 10 iterations

# Save results to JSON file
filevault benchmark -o benchmark_results.json --json
```

---

## Info

### View File Information
```bash
# Display information about encrypted file
filevault info document.txt.fvlt

# Shows:
# - File format version
# - Encryption algorithm
# - Key derivation function
# - Compression algorithm
# - File sizes
# - Creation timestamp
# - Additional metadata
```

---

## Common Use Cases

### Secure File Backup
```bash
# Encrypt important files with maximum security
filevault encrypt important_document.pdf -m advanced -p strong_password

# Create encrypted archive of multiple files
filevault archive create *.doc *.pdf -o backup.fva -p strong_password -s paranoid
```

### Quick File Sharing
```bash
# Encrypt with basic mode for fast sharing
filevault encrypt shared_file.txt -m basic -p sharepassword

# Recipient decrypts with same password
filevault decrypt shared_file.txt.fvlt -p sharepassword
```

### Stealth Communication
```bash
# Hide secret message in innocent-looking image
filevault stego embed secret_message.txt family_photo.png vacation_photo.png

# Send vacation_photo.png (looks normal, contains hidden data)
# Recipient extracts hidden message
filevault stego extract vacation_photo.png received_secret.txt
```

### Verify File Integrity
```bash
# Generate hash before sending
filevault hash important_file.zip -a sha256 -o file_hash.txt

# Recipient verifies hash after receiving
filevault hash important_file.zip -a sha256 -v <hash_from_file_hash.txt>
```

### Large File Compression
```bash
# Compress large files before encryption
filevault compress large_dataset.csv -a lzma -l 9

# Then encrypt compressed file
filevault encrypt large_dataset.csv.lzma -p mypassword
```

---

## Algorithm Reference

### Symmetric Encryption (AEAD)
- `aes-128-gcm`, `aes-192-gcm`, `aes-256-gcm` - AES with GCM mode (recommended)
- `chacha20-poly1305` - ChaCha20 stream cipher with Poly1305 MAC
- `serpent-256-gcm` - Serpent cipher (AES finalist)
- `twofish-256-gcm` - Twofish cipher (AES finalist)
- `camellia-256-gcm` - Camellia cipher (ISO standard)
- `aria-256-gcm` - ARIA cipher (Korean standard)

### Symmetric Encryption (Non-AEAD)
- `aes-256-cbc` - AES with CBC mode (legacy)
- `aes-256-ctr` - AES with CTR mode (stream)
- `aes-256-cfb` - AES with CFB mode (stream)
- `aes-256-ofb` - AES with OFB mode (stream)
- `aes-128-xts`, `aes-256-xts` - AES-XTS for disk encryption
- `aes-256-ecb` - AES with ECB mode (INSECURE, not recommended)

### Hash Functions
- `sha256`, `sha512` - SHA-2 family (recommended)
- `sha3-256`, `sha3-512` - SHA-3 family
- `blake2b` - BLAKE2b hash function
- `md5`, `sha1` - Legacy functions (BROKEN, use only for compatibility)

### Compression Algorithms
- `zlib` - Fast, good compression ratio
- `bzip2` - Better compression, slower
- `lzma` - Best compression, slowest
- `none` - No compression

### Key Derivation Functions
- `argon2id` - Argon2id (recommended, memory-hard)
- `argon2i` - Argon2i variant
- `pbkdf2-sha256` - PBKDF2 with SHA-256
- `pbkdf2-sha512` - PBKDF2 with SHA-512
- `scrypt` - Scrypt (memory-hard)

### Security Levels
- `weak` - Minimal security, fastest
- `medium` - Balanced (default)
- `strong` - Enhanced security
- `paranoid` - Maximum security, slowest

---

## Tips and Best Practices

1. **Never use `-p password` in production** - Always use interactive password prompts for better security

2. **Use mode presets** - Instead of manually configuring every option, use `-m basic/standard/advanced`

3. **Verify after encryption** - Always test decryption after encrypting important files

4. **Keep backups** - Don't delete original files until you've verified encrypted versions work

5. **Use strong passwords** - FileVault will warn you about weak passwords, listen to it!

6. **For maximum security** - Use `advanced` mode with a strong password (>16 chars, mixed case, numbers, symbols)

7. **For quantum resistance** - Use Kyber-1024 hybrid keys for future-proof encryption

8. **Progress bars** - Use `--no-progress` flag when running in scripts or pipelines

9. **Compression tips** - Already compressed files (ZIP, JPG, MP4) don't benefit from compression

10. **Config defaults** - Set your preferred defaults with `config set` to save typing

---

## Exit Codes

- `0` - Success
- `1` - General error (invalid arguments, file not found, etc.)
- Other non-zero values indicate specific error conditions

---

## Support

For more information:
- Read the main [README.md](README.md)
- Check [ARCHITECTURE.md](docs/ARCHITECTURE.md) for technical details
- See [ALGORITHMS.md](docs/ALGORITHMS.md) for cryptography information
- Visit the repository: https://github.com/vuongdat67/NT140.Q11.ANTT-Group15
