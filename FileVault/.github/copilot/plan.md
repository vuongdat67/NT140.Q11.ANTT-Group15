1/ Stack
c++ 17/20
cmake
ninja
conan
botan 
catch2
zlib # for compression or zstd(optional but i dont want to third party submodule)
lsb 
stb_image
...

2/conanfile.txt
[requires]
botan/3.6.1
cli11/2.4.2
catch2/3.10.0
spdlog/1.14.1
fmt/11.0.2
indicators/2.3
zlib/1.3.1
benchmark/1.9.0

[options]
botan/*:shared=False
botan/*:with_zlib=True
botan/*:with_bzip2=False
botan/*:with_sqlite3=False
botan/*:with_boost=False
spdlog/*:shared=False
catch2/*:with_main=True

[generators]
CMakeDeps
CMakeToolchain

[layout]
cmake_layout
EOF
3/ 
3.1 Install Dependencies
# Profile detection (first time only)
conan profile detect
# Install dependencies
conan install . --build=missing -s compiler.cppstd=17 -s build_type=Release
# For debug build
conan install . --build=missing -s compiler.cppstd=17 -s build_type=Debug
3.2 Configure & Build
# Configure
cmake --preset conan-release
# Or manually:
cmake -B build/Release -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
# Build
cmake --build build/Release
# Run tests
ctest --test-dir build/Release --output-on-failure
3.3 Lock Dependencies
# Generate lockfile for reproducible builds
conan lock create . --build=missing
# Use lockfile in CI/CD
conan install . --lockfile=conan.lock
3.4 Multiple Build Types
# Debug build (with sanitizers)
conan install . -s build_type=Debug -s compiler.cppstd=17
cmake --preset conan-debug -DENABLE_ASAN=ON
cmake --build build/Debug
# Release build (optimized)
conan install . -s build_type=Release -s compiler.cppstd=17
cmake --preset conan-release
cmake --build build/Release
3.5 Clean Rebuild
# Remove all build artifacts
rm -rf build/
# Reinstall dependencies
conan install . --build=missing -s build_type=Release
# Rebuild
cmake --preset conan-release && cmake --build build/Release
4/Algorithm:
Classical:
- Caesar
- Vigenère
- Playfair
- Substitution
- Rail Fence
Symmetric:
- Full AES mode: ECB, CBC, GCM(need to have), CTR, optional(CFB, XTS) with:
key-size 128,192, 256 or 16, 24, 32 bytes, 
block size: 128 bit or 16 bytes 
--> AES_GCM_256(require)
- Full hash: md5, sha1, sha2(256,512,...), sha3,BLAKE2,...
- Full DES mode (optional)
Key Derivation:
- Pdkdf2
- Argon2, Argon2i


[Require]
## Setup Checklist

### Dependencies
- [x] Check Botan version: `conan search botan`
- [x] Check Catch2 version: `conan search catch2`
- [x] Create `conanfile.txt` (Option B recommended)
- [ ] Install dependencies: `conan install . --build=missing`
- [ ] Verify installation: `ls build/Release/generators/`

### Project Structure
- [ ] Create folder structure (see above)
- [ ] Add `.clang-format` file
- [ ] Add `.gitignore`
- [ ] Add `CMakeLists.txt` (root)
- [ ] Add `README.md`

### Code Setup
- [ ] Create `include/filevault/types.hpp` (enums)
- [ ] Create `include/filevault/exceptions.hpp`
- [ ] Create `src/crypto/cipher/cipher_base.hpp`
- [ ] Create `cli/main.cpp` (minimal)

### Build System
- [ ] Test CMake configuration
- [ ] Test build: `cmake --build build`
- [ ] Setup VSCode tasks (optional)

### Testing
- [ ] Create `tests/unit/test_basic.cpp` (hello world)
- [ ] Run tests: `ctest --test-dir build`
- [ ] Add NIST test vectors
using botan test or nist test vectors, catch2v3

### Documentation
- [ ] Write `README.md`
- [ ] Create algorithm docs folder structure
- [ ] Write first algorithm doc (Caesar cipher)


5/ CLI design
v1:
filevault [OPTIONS] COMMAND [ARGS]...
├── encrypt <file> [options]
│   --algo, -a: Algorithm name (AES256-GCM, ...)
│   --output, -o: Output file
│   --password, -p: Password (hoặc no password default)
│   --compress: Enable compression
│   --stego: Hide in image
│
├── decrypt <file> [options]
│   --output, -o: Output file
│   --password, -p: Password
│
├── list-algorithms [options]
│   --category: classical, symmetric, asymmetric, hash, all
│   --detailed: Show parameters
│
├── benchmark [options]
│   --algo: Algorithm to benchmark
│   --size: Test file size (default: 100MB)
|   also store   
│
├── generate-key [options]
│   --type: rsa, ecc, ...
│   --size: Key size
│
└── hide-message (steganography)
    --image: Cover image
    --message: Secret file
    --output: Stego image
v2: reference from https://github.com/FiloSottile/age but less function than filevault
Usage:
    age [--encrypt] (-r RECIPIENT | -R PATH)... [--armor] [-o OUTPUT] [INPUT]
    age [--encrypt] --passphrase [--armor] [-o OUTPUT] [INPUT]
    age --decrypt [-i PATH]... [-o OUTPUT] [INPUT]

Options:
    -e, --encrypt               Encrypt the input to the output. Default if omitted.
    -d, --decrypt               Decrypt the input to the output.
    -o, --output OUTPUT         Write the result to the file at path OUTPUT.
    -a, --armor                 Encrypt to a PEM encoded format.
    -p, --passphrase            Encrypt with a passphrase.
    -r, --recipient RECIPIENT   Encrypt to the specified RECIPIENT. Can be repeated.
    -R, --recipients-file PATH  Encrypt to recipients listed at PATH. Can be repeated.
    -i, --identity PATH         Use the identity file at PATH. Can be repeated.
v3: 
## CLI Commands

### **SPRINT 1-2: Commands Bắt Buộc**
```bash
# Mã hóa/giải mã cơ bản
filevault encrypt <file>              # Mã hóa file
filevault decrypt <file>              # Giải mã file
filevault encrypt <file> -o <path>    # Chỉ định output
filevault decrypt <file> -o <path>

# Hỗ trợ cơ bản
filevault --help                      # Trợ giúp chung
filevault encrypt --help              # Trợ giúp mã hóa
filevault --version                   # Phiên bản
```

### **SPRINT 3: Commands Quan Trọng**
```bash
# Thông tin file
filevault info <encrypted-file>       # Xem thông tin file đã mã hóa
filevault verify <encrypted-file>     # Kiểm tra tính toàn vẹn
filevault check <file>               # Kiểm tra file có bị mã hóa không

# Xử lý nhiều file
filevault encrypt *.txt              # Mã hóa nhiều file
filevault encrypt *.pdf -o encrypted/ # Batch với thư mục output

# Tùy chọn bảo mật
filevault encrypt <file> --iterations 200000  # Tùy chỉnh số vòng lặp
filevault encrypt <file> --secure-delete      # Xóa an toàn file gốc
filevault encrypt <file> --verbose            # Hiển thị chi tiết
```

### **SPRINT 4: Commands Bonus**
```bash
# Cấu hình
filevault config show               # Xem cấu hình hiện tại
filevault config set default-iterations 150000

# Nén (nếu có thời gian)
filevault encrypt <file> --compress  # Nén trước khi mã hóa
```

## Bổ Sung Thêm - Nếu được

### **Commands An Toàn Quan Trọng:**
```bash
# Test password trước khi decrypt (không ghi file)
filevault test-decrypt <file> --dry-run

# Backup tự động trước khi encrypt
filevault encrypt <file> --backup

# Liệt kê file encrypted trong thư mục
filevault list-encrypted /path/to/dir/
```

### **Error Handling:**
```bash
# Exit codes chuẩn
0 - Thành công
1 - Lỗi chung
2 - File không tồn tại  
3 - Không có quyền truy cập
4 - Sai mật khẩu
5 - File bị hỏng
```

v4:

## CLI Commands 

### Phase 1: Core Commands (Sprint 1-2) - BẮT BUỘC

```bash
# Basic encryption/decryption
filevault encrypt <file>
filevault decrypt <file>

# Essential options only
filevault encrypt <file> --output <path>
filevault encrypt <file> -o <path>
filevault decrypt <file> --output <path>

# Help system
filevault --help
filevault encrypt --help
filevault decrypt --help
filevault version
```

### Phase 2: Important Features (Sprint 3) - NÊN CÓ

```bash
# File information
filevault info <encrypted-file>
filevault verify <encrypted-file>

# Basic batch operations
filevault encrypt file1.txt file2.txt file3.txt
filevault encrypt *.txt --output-dir encrypted/

# Security options
filevault encrypt <file> --iterations 150000
filevault encrypt <file> --secure-delete

# Progress and verbose
filevault encrypt <file> --verbose
filevault encrypt <file> --quiet
```

### Phase 3: Nice-to-Have (Sprint 4) - BONUS

```bash
# Configuration
filevault config show
filevault config set default-iterations 200000

# Compression (nếu có thời gian)
filevault encrypt <file> --compress
```

# FileVault CLI Commands - Final Specification

## CORE COMMANDS (Sprint 1-2) - CRITICAL

### Basic Encryption/Decryption
```bash
# Encrypt single file
filevault encrypt <input-file> [output-file]
# Examples:
filevault encrypt document.pdf                    # → document.pdf.enc  
filevault encrypt document.pdf secure.enc         # → secure.enc
filevault encrypt document.pdf -o encrypted/      # → encrypted/document.pdf.enc

# Decrypt file
filevault decrypt <encrypted-file> [output-file]
# Examples:  
filevault decrypt document.pdf.enc               # → document.pdf
filevault decrypt secure.enc original.pdf        # → original.pdf
filevault decrypt secure.enc -o decrypted/       # → decrypted/original.pdf
```

### Essential Options
```bash
# Output specification
-o, --output <path>          # Output file or directory
-f, --force                  # Overwrite existing files
-k, --keep                   # Keep original file after encryption

# Basic information
-h, --help                   # Show help
-V, --version                # Show version
-v, --verbose                # Detailed output
-q, --quiet                  # Suppress non-error output
```

### Help System
```bash
filevault --help             # General help
filevault encrypt --help     # Encryption help  
filevault decrypt --help     # Decryption help
filevault version           # Version information
```

## ENHANCED COMMANDS (Sprint 3) - IMPORTANT

### File Information
```bash
# File analysis (without decryption)
filevault info <encrypted-file>
# Output: Algorithm, file size, creation date, format version

filevault verify <encrypted-file>  
# Output: File integrity check, format validation

filevault check <file>
# Output: Detect if file is FileVault-encrypted
```

### Batch Operations  
```bash
# Multiple files
filevault encrypt file1.txt file2.pdf file3.docx
filevault encrypt *.txt
filevault encrypt *.pdf --output-dir encrypted/

# Directory operations
filevault encrypt /path/to/files/* -o encrypted/
```

### Security Options
```bash
# Key derivation customization
--iterations <number>        # PBKDF2 iterations (default: 100000)
--secure-delete             # Secure deletion of original file

# Examples:
filevault encrypt sensitive.doc --iterations 200000
filevault encrypt temp.txt --secure-delete
```

### Progress Display
```bash
# Progress indication (automatic for files >10MB)
filevault encrypt largefile.zip --progress      # Force show progress
filevault encrypt largefile.zip --no-progress   # Hide progress
```

## OPTIONAL COMMANDS (Sprint 4) - BONUS

### Configuration Management
```bash
filevault config show                           # Show current settings
filevault config set default-iterations 150000  # Set defaults
filevault config reset                          # Reset to defaults
```

### Compression (If Time Permits)
```bash
filevault encrypt <file> --compress             # Enable compression
filevault encrypt <file> --compress-level 6     # Compression level 1-9
```

## COMMAND EXAMPLES

### Basic Workflow
```bash
# Encrypt important document
$ filevault encrypt financial_report.pdf
Enter password: ********
Confirm password: ********
✅ Encrypted: financial_report.pdf → financial_report.pdf.enc

# Decrypt when needed
$ filevault decrypt financial_report.pdf.enc  
Enter password: ********
✅ Decrypted: financial_report.pdf.enc → financial_report.pdf
```

### Advanced Usage
```bash
# High security encryption
$ filevault encrypt classified.docx --iterations 500000 --secure-delete -v
Using PBKDF2 with 500000 iterations...
Generating random salt...
Encrypting with AES-256-GCM...
Securely deleting original file...
✅ Encrypted: classified.docx → classified.docx.enc

# Batch encrypt with custom output
$ filevault encrypt *.txt --output-dir backup/encrypted/ --keep
Processing: note1.txt
Processing: note2.txt  
Processing: note3.txt
Enter password: ********
✅ 3 files encrypted to backup/encrypted/
```

### File Information
```bash
$ filevault info report.pdf.enc
File: report.pdf.enc
Format: FileVault v1.0
Algorithm: AES-256-GCM
Key Derivation: PBKDF2-SHA256 (100,000 iterations)
Original Size: 2.4 MB
Encrypted Size: 2.4 MB
Created: 2024-09-18 14:30:22 UTC
Status: ✅ Valid

$ filevault verify suspicious.enc
⚠️ Warning: File header corrupted
❌ Integrity check failed
```

## ERROR HANDLING

### Standard Error Messages
```bash
# Authentication errors
❌ Error: Authentication failed. Wrong password or corrupted file.

# File system errors  
❌ Error: Permission denied accessing '/protected/file.txt'
❌ Error: File 'missing.txt' not found
❌ Error: Insufficient disk space

# Input validation
❌ Error: Password too short (minimum 8 characters)
❌ Error: Invalid file format - not a FileVault encrypted file
```

### Exit Codes
```bash
0   # Success
1   # General error
2   # File not found
3   # Permission denied
4   # Authentication failed
5   # Invalid format/corrupted file
6   # Insufficient resources
7   # Invalid arguments
```

## IMPLEMENTATION PRIORITY

### Sprint 1 (Week 1-2): Core Foundation
- [x] `filevault encrypt <file>`
- [x] `filevault decrypt <file>`
- [x] Password prompting (secure input)
- [x] Basic error handling
- [x] `--help`, `--version`

### Sprint 2 (Week 3-4): Essential Features  
- [x] `--output`, `--force`, `--keep` options
- [x] `--verbose`, `--quiet` modes
- [x] Progress bar for large files
- [x] Input validation

### Sprint 3 (Week 5-6): Enhanced Features
- [x] `filevault info`, `verify`, `check` commands
- [x] Batch operations (multiple files)
- [x] `--iterations`, `--secure-delete` options
- [x] Configuration basics

### Sprint 4 (Week 7-8): Polish & Documentation
- [x] Advanced configuration management
- [x] Optional compression feature
- [x] Comprehensive help system
- [x] Documentation and examples

## TECHNICAL NOTES

### Password Input
```bash
# Always use secure terminal input (no echo)
# Password confirmation for encryption
# No password storage in memory after use
```

### File Naming Convention
```bash
# Default encrypted extension: .enc
original.pdf → original.pdf.enc
document.txt → document.txt.enc

# Custom output preserves name
filevault encrypt report.pdf backup.vault → backup.vault
```

### Performance Considerations
```bash
# Automatic progress bar threshold: 10MB
# Streaming encryption for large files
# Memory limit: <100MB regardless of file size
```

## VALIDATION RULES

### Input Validation
- File paths must exist and be readable
- Output paths must be writable  
- Password minimum 8 characters
- Iterations between 10,000 and 1,000,000
- File size limit: 10GB (practical limit)

### Security Requirements
- No password logging or storage
- Secure memory cleanup after operations
- Constant-time password comparison
- Unique salt per encryption operation


### 1. Safety & Recovery Commands:
```bash
# Backup file trước khi encrypt (safety net)
filevault encrypt file.txt --backup

# Test decrypt without writing file (verify password)
filevault test-decrypt file.enc --dry-run
```

### 2. Practical Workflow Commands:
```bash
# Quick file check (is this encrypted by FileVault?)  
filevault is-encrypted file.enc

# List all .enc files in directory
filevault list-encrypted /path/to/dir/

# Rename encrypted files (maintain metadata)
filevault rename old.enc new.enc
```

### 3. Security Audit Commands (cho sinh viên IT):
```bash
# Test password strength
filevault test-password

# Show crypto parameters used
filevault show-params file.enc
```

