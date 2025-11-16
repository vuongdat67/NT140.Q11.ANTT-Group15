# FileVault - Tài Liệu Thiết Kế Toàn Diện

> **Version:** 1.0  
> **Date:** 2024-11-11  
> **Language:** C++17  
> **Platform:** Cross-platform (Windows/Linux/macOS)

---

## 📑 Mục Lục

1. [CLI Command Design](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#1-cli-command-design)
2. [Algorithm Documentation (Obsidian Style)](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#2-algorithm-documentation-obsidian-style)
3. [OOP Design](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#3-oop-design)
4. [Architecture Design](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#4-architecture-design)
5. [Folder Structure](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#5-folder-structure)
6. [Development Q&A Guide](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#6-development-qa-guide)
7. [SDLC Phases](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#7-sdlc-phases)
8. [Technology Stack](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#8-technology-stack)
9. [Additional Considerations](https://claude.ai/chat/03bbd09c-efbe-451e-845c-767c3626a442#9-additional-considerations)

---

## 1. CLI Command Design

### 1.1 Design Principles

**Mục tiêu:** Đơn giản, trực quan, theo chuẩn POSIX/GNU

**Nguyên tắc:**

- ✅ Verb-first (encrypt, decrypt, hash...)
- ✅ Sensible defaults (ít flags cần thiết nhất)
- ✅ Short flags (-i) + Long flags (--input)
- ✅ Interactive prompts cho password
- ✅ Progress indicators
- ✅ Colorized output

### 1.2 Command Structure

```bash
filevault <command> [options] <arguments>
```

### 1.3 Core Commands

#### **encrypt** - Mã hóa file

```bash
# Basic usage (interactive)
filevault encrypt <input-file>

# Với output path
filevault encrypt <input-file> -o <output-file>

# Advanced
filevault encrypt <input-file> \
    --output <output-file> \
    --mode standard|advanced|basic \
    --algorithm aes256|aes192|aes128|des|3des \
    --cipher-mode gcm|cbc|ctr \
    --kdf argon2id|argon2i|pbkdf2 \
    --compress none|zlib|zstd \
    --password <password> \
    --iterations 600000 \
    --memory 64 \
    --verbose \
    --no-progress

# Examples
filevault encrypt secret.pdf
filevault encrypt contract.docx -o contract.fv
filevault encrypt data.zip -m advanced -c zstd --kdf argon2id
filevault encrypt homework.txt -m basic -a des
```

#### **decrypt** - Giải mã file

```bash
# Basic
filevault decrypt <encrypted-file>

# With output
filevault decrypt <encrypted-file> -o <output-file>

# Options
filevault decrypt <file> \
    --output <output> \
    --password <password> \
    --verbose

# Examples
filevault decrypt secret.fv
filevault decrypt contract.fv -o contract.docx
```

#### **hash** - Tính hash của file

```bash
filevault hash <file> [options]

# Options
--algorithm md5|sha1|sha256|sha512|sha3-256|blake2b
--output <file>  # Save hash to file

# Examples
filevault hash document.pdf
filevault hash image.png -a sha256
filevault hash *.txt -a blake2b --output checksums.txt
```

#### **compress** - Nén file (không mã hóa)

```bash
filevault compress <file> [options]

# Options
--algorithm zlib|zstd|bzip2|lzma
--level 1-9
--output <file>

# Examples
filevault compress bigfile.log
filevault compress data.bin -a zstd -l 9 -o data.zst
```

#### **stego** - Steganography

```bash
# Embed (ẩn file vào ảnh)
filevault stego embed <secret-file> <cover-image> [options]

# Extract (trích xuất file từ ảnh)
filevault stego extract <stego-image> [options]

# Options (embed)
--output <file>           # Stego output image
--password <password>     # Encrypt before embedding
--compress                # Compress before embedding

# Options (extract)
--output <file>           # Extracted file
--password <password>     # Decrypt after extraction

# Examples
filevault stego embed secret.txt photo.png -o stego.png
filevault stego embed data.zip cover.png --password mypass --compress
filevault stego extract stego.png -o extracted.txt
```

#### **info** - Xem thông tin file đã mã hóa

```bash
filevault info <encrypted-file>

# Output example:
File: secret.fv
Version: 1.0
Cipher: AES-256-GCM
KDF: Argon2id (64MB, 3 iterations, 4 threads)
Compression: Zstd (ratio: 65%)
Original size: 2.5 MB
Encrypted size: 1.8 MB
Timestamp: 2024-11-11 10:30:00
```

#### **benchmark** - Đo hiệu năng

```bash
filevault benchmark [options]

# Options
--algorithms aes256,chacha20,des
--sizes 1MB,10MB,100MB
--kdf pbkdf2,argon2id
--output <file>  # JSON output

# Example
filevault benchmark --algorithms aes256,aes128 --sizes 10MB,100MB
```

#### **config** - Quản lý cấu hình

```bash
# Show current config
filevault config show

# Set default values
filevault config set <key> <value>

# Examples
filevault config set default.algorithm aes256
filevault config set default.kdf argon2id
filevault config set default.compression zstd
```

### 1.4 Global Options

```bash
-h, --help           Hiển thị help
-v, --version        Hiển thị version
--verbose            Chi tiết output
--quiet              Không hiển thị gì (chỉ errors)
--no-color           Tắt màu sắc
--no-progress        Tắt progress bar
```

### 1.5 Mode Presets

```bash
# --mode basic (Educational)
# Mặc định: DES-CBC, PBKDF2 (10k iterations), no compression
filevault encrypt file.txt --mode basic

# --mode standard (Recommended)
# Mặc định: AES-256-GCM, Argon2id (64MB, 3 iter), no compression
filevault encrypt file.txt --mode standard

# --mode advanced (Maximum security)
# Mặc định: AES-256-GCM, Argon2id (128MB, 5 iter), Zstd compression
filevault encrypt file.txt --mode advanced
```

### 1.6 Interactive Mode

```bash
$ filevault encrypt secret.pdf

FileVault Encryption
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Input file: secret.pdf (2.5 MB)

Select mode:
  1) Basic      - Educational (DES, simple)
  2) Standard   - Recommended (AES-256) ★
  3) Advanced   - Maximum security
  4) Custom     - Configure manually

Enter choice [1-4]: 2

Select encryption algorithm:
  1) AES-256-GCM   - Best (authenticated) ★
  2) AES-256-CBC   - Standard
  3) AES-256-CTR   - Fast

Enter choice [1-3]: 1

Select key derivation:
  1) Argon2id   - Secure (slower) ★
  2) PBKDF2     - Fast (less secure)

Enter choice [1-2]: 1

Enable compression? [y/N]: y

Select compression:
  1) Zstd   - Best ratio + speed ★
  2) Zlib   - Standard
  3) Bzip2  - High ratio (slow)

Enter choice [1-3]: 1

Enter password: ********
Confirm password: ********

Encryption Settings
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Mode:        Standard
Algorithm:   AES-256-GCM
KDF:         Argon2id (64MB, 3 iterations)
Compression: Zstd
Output:      secret.fv

Proceed? [Y/n]: y

[████████████████████] 100% | 120 MB/s | 2.5 MB

✓ Encrypted successfully!
  Output: secret.fv (1.8 MB)
  Compression ratio: 28%
  Time: 2.3s
```

### 1.7 Error Handling

```bash
# File not found
$ filevault encrypt nonexistent.txt
✗ Error: File not found: nonexistent.txt

# Wrong password
$ filevault decrypt secret.fv
Enter password: ********
✗ Error: Authentication failed - wrong password or corrupted file

# Unsupported file format
$ filevault decrypt corrupted.fv
✗ Error: Invalid file format (expected .fv version 1.0)

# Disk full
$ filevault encrypt hugefile.bin
[████████    ] 80% | 100 MB/s
✗ Error: Disk full - 500 MB required, 200 MB available
  Partial file removed: hugefile.fv.tmp
```

---

## 2. Algorithm Documentation (Obsidian Style)

### 2.1 Folder Structure

```
docs/algorithms/
├── README.md
├── _index.md                    # Overview
├── classical/
│   ├── caesar.md
│   ├── vigenere.md
│   ├── playfair.md
│   ├── hill-cipher.md
│   └── substitution.md
├── symmetric/
│   ├── des.md
│   ├── 3des.md
│   ├── aes.md
│   ├── chacha20.md
│   └── modes-of-operation.md
├── hash/
│   ├── md5.md
│   ├── sha-family.md
│   ├── sha3.md
│   └── blake2.md
├── kdf/
│   ├── pbkdf2.md
│   └── argon2.md
├── compression/
│   ├── zlib.md
│   ├── zstd.md
│   ├── bzip2.md
│   └── lzma.md
└── steganography/
    └── lsb.md
```

### 2.2 Example: caesar.md

```markdown
# Caesar Cipher

> **Category:** [[_index|Classical Ciphers]]  
> **Type:** Substitution cipher  
> **Security:** ⚠️ Insecure (educational only)  
> **Year:** ~100 BC  

## Overview

Caesar cipher là một trong những thuật toán mã hóa đơn giản nhất, được Julius Caesar sử dụng để mã hóa thông điệp quân sự. Thuật toán dịch chuyển mỗi ký tự trong bảng chữ cái đi một số vị trí cố định.

## Algorithm

### Encryption

```

E(x) = (x + k) mod 26

```

Trong đó:
- `x`: Vị trí ký tự trong bảng chữ cái (A=0, B=1, ..., Z=25)
- `k`: Khóa dịch chuyển (shift key), thường 0-25
- `E(x)`: Ký tự mã hóa

### Decryption

```

D(x) = (x - k) mod 26

```

### Example

```

Plaintext: HELLO WORLD Key: 3 Ciphertext: KHOOR ZRUOG

H → K (7 + 3 = 10) E → H (4 + 3 = 7) L → O (11 + 3 = 14) ...

````

## Implementation

```cpp
std::string caesar_encrypt(const std::string& plaintext, int shift) {
    std::string ciphertext;
    shift = ((shift % 26) + 26) % 26; // Normalize
    
    for (char c : plaintext) {
        if (std::isalpha(c)) {
            char base = std::isupper(c) ? 'A' : 'a';
            c = base + (c - base + shift) % 26;
        }
        ciphertext += c;
    }
    return ciphertext;
}

std::string caesar_decrypt(const std::string& ciphertext, int shift) {
    return caesar_encrypt(ciphertext, -shift);
}
````

## Cryptanalysis

### Brute Force

Chỉ có **25 khóa khả thi** → dễ dàng thử hết.

```cpp
void caesar_brute_force(const std::string& ciphertext) {
    for (int k = 1; k < 26; ++k) {
        std::cout << "Key " << k << ": " 
                  << caesar_decrypt(ciphertext, k) << '\n';
    }
}
```

### Frequency Analysis

Tiếng Anh: E, T, A, O, I là các ký tự phổ biến nhất.

```
Ciphertext frequency: K (most common)
→ Có thể K = E encrypted
→ Shift = K - E = 10 - 4 = 6
```

## Security

- ✅ **Khóa nhỏ**: Chỉ 25 khóa → brute force trong vài giây
- ✅ **Giữ structure**: Spaces, punctuation không thay đổi
- ✅ **Frequency patterns**: Dễ phân tích tần suất

**Kết luận:** Chỉ dùng cho mục đích học tập!

## Variations

- [[vigenere|Vigenère Cipher]]: Dùng nhiều Caesar shifts với từ khóa
- [[rot13|ROT13]]: Caesar với shift = 13 (self-inverse)

## Related

- [[substitution|Substitution Cipher]]: Tổng quát hóa Caesar
- [[affine-cipher|Affine Cipher]]: Mở rộng với phép nhân

## References

- [Wikipedia: Caesar cipher](https://en.wikipedia.org/wiki/Caesar_cipher)
- "The Code Book" by Simon Singh

## FileVault Implementation

```bash
# Encrypt with Caesar (shift=7)
filevault encrypt message.txt --mode basic --algorithm caesar --shift 7

# Brute force attack demo
filevault attack brute-force encrypted.fv --algorithm caesar
```

---

**Tags:** #classical #substitution #educational #insecure

````

### 2.3 Example: aes.md

```markdown
# AES (Advanced Encryption Standard)

> **Category:** [[symmetric/README|Symmetric Ciphers]]  
> **Type:** Block cipher  
> **Security:** ✅ Secure (2024)  
> **Standard:** FIPS 197 (2001)  
> **Key sizes:** 128, 192, 256 bits  
> **Block size:** 128 bits  

## Overview

AES là thuật toán mã hóa đối xứng được Rijndael thiết kế và NIST chuẩn hóa năm 2001, thay thế [[des|DES]]. Hiện là tiêu chuẩn toàn cầu cho mã hóa dữ liệu.

## Technical Details

### Structure

**Type:** Substitution-Permutation Network (SPN)

**Rounds:**
- AES-128: 10 rounds
- AES-192: 12 rounds
- AES-256: 14 rounds

### Round Operations

Mỗi round gồm 4 bước:

1. **SubBytes** - Thay thế byte qua S-box (16×16)
2. **ShiftRows** - Dịch hàng theo pattern fixed
3. **MixColumns** - Trộn cột (Galois Field multiplication)
4. **AddRoundKey** - XOR với round key

````

Plaintext (128-bit block) ↓ AddRoundKey (initial) ↓ ┌───────────────┐ │ Round 1 │ │ - SubBytes │ │ - ShiftRows │ │ - MixColumns │ │ - AddRoundKey│ └───────┬───────┘ ↓ [Rounds 2-9] ↓ ┌───────────────┐ │ Round 10 │ │ - SubBytes │ │ - ShiftRows │ │ - AddRoundKey│ (no MixColumns) └───────┬───────┘ ↓ Ciphertext

````

### Key Schedule

```cpp
Master Key (128/192/256-bit)
    ↓
Key Expansion Algorithm (Rijndael)
    ↓
Round Keys (11/13/15 × 128-bit)
````

## Modes of Operation

AES là block cipher → cần mode để mã hóa data > 1 block.

### ECB (Electronic Codebook)

[[modes-of-operation#ECB|Details]]

❌ **KHÔNG dùng** - patterns leak

```
[Block1] [Block2] [Block3]
   ↓         ↓         ↓
 AES(K)    AES(K)    AES(K)
   ↓         ↓         ↓
[Ciph1]  [Ciph2]  [Ciph3]
```

### CBC (Cipher Block Chaining)

[[modes-of-operation#CBC|Details]]

✅ Secure với random IV + [[mac/hmac|HMAC]]

```
IV ─────────┐
            ↓
[Plain1] → XOR → AES(K) → [Ciph1] ─┐
                                    ↓
[Plain2] ─────────────── XOR → AES(K) → [Ciph2]
```

**Cần:**

- Random IV (16 bytes)
- Padding ([[pkcs7|PKCS#7]])
- HMAC để chống padding oracle

### GCM (Galois/Counter Mode)

[[modes-of-operation#GCM|Details]]

✅✅ **Khuyên dùng** - AEAD (authenticated + encrypted)

```
Counter ──→ AES(K) ──→ XOR ──→ Ciphertext
                        ↑
                    Plaintext
                        ↓
                   [GHASH] ──→ Auth Tag (128-bit)
```

**Ưu điểm:**

- Authenticated Encryption with Associated Data
- Parallel encryption/decryption
- Không cần padding
- Detect tampering

**Cần:**

- Unique nonce (96-bit) **mỗi lần**
- Nonce reuse = **catastrophic failure**

## Implementation (Botan)

```cpp
#include <botan/cipher_mode.h>
#include <botan/auto_rng.h>

// Encrypt with AES-256-GCM
Botan::AutoSeeded_RNG rng;
auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", Botan::ENCRYPTION);

// Key from KDF (see [[kdf/argon2|Argon2]])
std::vector<uint8_t> key = derive_key(password, salt);
cipher->set_key(key);

// Nonce: MUST be unique per encryption
std::vector<uint8_t> nonce = rng.random_vec(12); // 96-bit
cipher->start(nonce);

// Encrypt (in-place)
Botan::secure_vector<uint8_t> ciphertext(plaintext.begin(), plaintext.end());
cipher->finish(ciphertext); // Appends 128-bit auth tag

// Decrypt
auto decipher = Botan::Cipher_Mode::create("AES-256/GCM", Botan::DECRYPTION);
decipher->set_key(key);
decipher->start(nonce);

Botan::secure_vector<uint8_t> decrypted(ciphertext);
decipher->finish(decrypted); // Throws if tag invalid
```

## Security Analysis

### Attacks Mitigated

- ✅ **Brute force**: 2^256 keyspace (AES-256)
- ✅ **Known-plaintext**: SPN structure resistant
- ✅ **Related-key attacks**: Strong key schedule

### Known Weaknesses

**Academic attacks only:**

- Biclique attack on AES-256: 2^254.4 (vs 2^256 brute force)
- Cache-timing attacks: Mitigated in hardware (AES-NI)

**Practical:**

- Weak passwords: Use [[kdf/argon2|Argon2]] or [[kdf/pbkdf2|PBKDF2]]
- Nonce reuse (GCM): Complete failure
- Padding oracle (CBC): Use HMAC

### Best Practices

1. ✅ Use **AES-256-GCM** for new systems
2. ✅ Random nonce/IV per encryption
3. ✅ Key derivation: [[kdf/argon2|Argon2id]] > [[kdf/pbkdf2|PBKDF2]]
4. ✅ Hardware acceleration: Use CPU AES-NI
5. ✅ Memory: Wipe keys after use (secure_vector)

## Performance

**Hardware (AES-NI):**

- Encryption: 4-8 GB/s (single thread)
- Latency: ~1 cycle/byte on modern CPUs

**Software (no AES-NI):**

- ~100-200 MB/s

**Benchmark:**

```bash
filevault benchmark --algorithm aes256-gcm --size 100MB
```

## Comparison

|Cipher|Key Size|Speed|Security|Use Case|
|---|---|---|---|---|
|[[des\|DES]]|56-bit|Fast|❌ Broken|Legacy only|
|[[3des\|3DES]]|168-bit|Slow|⚠️ Weak|Legacy|
|**AES-128**|128-bit|Fast|✅ Secure|Standard|
|**AES-256**|256-bit|Fast|✅ Secure|High security|
|[[chacha20\|ChaCha20]]|256-bit|Faster*|✅ Secure|Mobile|

*ChaCha20 nhanh hơn khi không có AES-NI

## Test Vectors (NIST)

```
# AES-256-GCM (NIST SP 800-38D)
Key:  000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f
IV:   cafebabefacedbaddecaf888
Plaintext:  d9313225f88406e5a55909c5aff5269a
Expected:   42831ec2217774244b7221b784d0d49c

# Verify implementation
```

## Related Algorithms

- [[chacha20|ChaCha20-Poly1305]]: Modern alternative
- [[des|DES]]: Historical predecessor
- [[modes-of-operation|Block cipher modes]]

## FileVault Usage

```bash
# Encrypt with AES-256-GCM (default)
filevault encrypt secret.pdf

# Explicit mode
filevault encrypt file.txt \
    --algorithm aes256 \
    --mode gcm \
    --kdf argon2id

# Compare modes
filevault benchmark --algorithms aes256-gcm,aes256-cbc --size 10MB
```

## References

- [FIPS 197: AES Specification](https://csrc.nist.gov/publications/detail/fips/197/final)
- [NIST SP 800-38D: GCM](https://csrc.nist.gov/publications/detail/sp/800-38d/final)
- [Botan AES Documentation](https://botan.randombit.net/)

---

**Tags:** #symmetric #modern #secure #standard #aes

````

### 2.4 Wikilink Pattern

**Cú pháp:**
```markdown
[[filename|Display Text]]
[[filename#section|Section Link]]
[[../folder/file|Cross-folder Link]]
````

**Ví dụ:**

```markdown
Caesar cipher là tiền thân của [[vigenere|Vigenère cipher]].

AES có nhiều [[modes-of-operation#GCM|modes]], trong đó GCM là tốt nhất.

Dùng [[../kdf/argon2|Argon2]] để derive key từ password.
```

---

## 3. OOP Design

### 3.1 Class Diagram (Text-Based UML)

```
┌─────────────────────────────────────────────────────────────┐
│                        FileVault Core                        │
└─────────────────────────────────────────────────────────────┘

┌──────────────────────┐
│   <<interface>>      │
│   ICipherEngine      │
├──────────────────────┤
│ + encrypt()          │
│ + decrypt()          │
│ + get_key_size()     │
│ + get_block_size()   │
│ + get_iv_size()      │
└──────────┬───────────┘
           │
           ├─────────────────────────────────┐
           │                                 │
┌──────────▼──────────┐           ┌─────────▼──────────┐
│  AESEngine          │           │  DESEngine         │
├─────────────────────┤           ├────────────────────┤
│ - mode: CipherMode  │           │ - variant: Variant │
│ - botan_cipher      │           │ - botan_cipher     │
├─────────────────────┤           ├────────────────────┤
│ + encrypt()         │           │ + encrypt()        │
│ + decrypt()         │           │ + decrypt()        │
└─────────────────────┘           └────────────────────┘

┌──────────────────────┐
│   <<interface>>      │
│   IKDFEngine         │
├──────────────────────┤
│ + derive_key()       │
│ + get_salt_size()    │
│ + get_params()       │
└──────────┬───────────┘
           │
           ├─────────────────────────────────┐
           │                                 │
┌──────────▼──────────┐           ┌─────────▼──────────┐
│  PBKDF2Engine       │           │  Argon2Engine      │
├─────────────────────┤           ├────────────────────┤
│ - iterations        │           │ - memory_kb        │
│ - hash_algo         │           │ - time_cost        │
│                     │           │ - parallelism      │
├─────────────────────┤           ├────────────────────┤
│ + derive_key()      │           │ + derive_key()     │
└─────────────────────┘           └────────────────────┘

┌──────────────────────┐
│   <<interface>>      │
│   ICompressor        │
├──────────────────────┤
│ + compress()         │
│ + decompress()       │
│ + get_ratio()        │
└──────────┬───────────┘
           │
           ├─────────────────┬─────────────────┐
           │                 │                 │
┌──────────▼──────┐  ┌───────▼────────┐  ┌───▼─────────┐
│  ZlibCompressor │  │ ZstdCompressor │  │ LZMACompress│
└─────────────────┘  └────────────────┘  └─────────────┘

┌──────────────────────────────────────────┐
│           EncryptionService               │
├──────────────────────────────────────────┤
│ - cipher_engine: ICipherEngine*          │
│ - kdf_engine: IKDFEngine*                │
│ - compressor: ICompressor*               │
│ - file_handler: FileHandler              │
│ - format_builder: FileFormatBuilder      │
├──────────────────────────────────────────┤
│ + encrypt_file(path, password, options)  │
│ + decrypt_file(path, password)           │
│ - derive_key(password, salt)             │
│ - build_header(options)                  │
│ - write_encrypted_file()                 │
└──────────────────────────────────────────┘

┌──────────────────────────────────────────┐
│           FileFormatHandler               │
├──────────────────────────────────────────┤
│ - header: FileHeader                     │
├──────────────────────────────────────────┤
│ + parse(file_path): FileHeader           │
│ + serialize(header, data): bytes         │
│ + validate(bytes): bool                  │
│ + get_metadata(): Metadata               │
└──────────────────────────────────────────┘

┌──────────────────────────────────────────┐
│              FileHeader                   │
├──────────────────────────────────────────┤
│ + magic: uint8_t[4]                      │
│ + version: uint8_t                       │
│ + cipher_type: CipherType                │
│ + cipher_mode: CipherMode                │
│ + kdf_type: KDFType                      │
│ + compression: CompressionType           │
│ + salt: bytes[32]                        │
│ + iv_nonce: bytes[16]                    │
│ + metadata_length: uint32_t              │
│ + kdf_params: KDFParams                  │
└──────────────────────────────────────────┘

┌──────────────────────────────────────────┐
│         ClassicalCipherEngine             │
├──────────────────────────────────────────┤
│ <<interface for classical ciphers>>      │
└──────────┬───────────────────────────────┘
           │
           ├────────────────┬────────────────┐
           │                │                │
┌──────────▼─────┐  ┌───────▼────────┐  ┌──▼──────────┐
│ CaesarCipher   │  │ VigenereCipher │  │ PlayfairCiph│
├────────────────┤  ├────────────────┤  ├─────────────┤
│ - shift: int   │  │ - keyword: str │  │ - key: str  │
├────────────────┤  ├────────────────┤  ├─────────────┤
│ + encrypt()    │  │ + encrypt()    │  │ + encrypt() │
│ + decrypt()    │  │ + decrypt()    │  │ + decrypt() │
│ + brute_force()│  │ + analyze()    │  │             │
└────────────────┘  └────────────────┘  └─────────────┘

┌──────────────────────────────────────────┐
│            HashService                    │
├──────────────────────────────────────────┤
│ - hash_algo: HashAlgorithm               │
├──────────────────────────────────────────┤
│ + hash_file(path): bytes                 │
│ + hash_string(data): bytes               │
│ + verify(file, expected_hash): bool      │
└──────────────────────────────────────────┘

┌──────────────────────────────────────────┐
│         SteganographyService              │
├──────────────────────────────────────────┤
│ - image_handler: ImageHandler            │
├──────────────────────────────────────────┤
│ + embed(secret, cover, output)           │
│ + extract(stego_image, output)           │
│ + get_capacity(image): size_t            │
│ - embed_lsb(data, pixels)                │
│ - extract_lsb(pixels): data              │
└──────────────────────────────────────────┘
```

### 3.2 Design Patterns Sử Dụng

#### **Strategy Pattern** - Cipher Selection

```cpp
// Client code không cần biết implementation details
class EncryptionService {
    std::unique_ptr<ICipherEngine> cipher_;
    
public:
    void set_cipher(std::unique_ptr<ICipherEngine> cipher) {
        cipher_ = std::move(cipher);
    }
    
    void encrypt(const Data& plaintext) {
        auto ciphertext = cipher_->encrypt(plaintext, key_, iv_);
        // ...
    }
};

// Usage
service.set_cipher(std::make_unique<AESEngine>(CipherMode::GCM));
service.encrypt(data);
```

#### **Factory Pattern** - Object Creation

```cpp
class CipherFactory {
public:
    static std::unique_ptr<ICipherEngine> create(
        CipherType type, 
        CipherMode mode
    ) {
        switch(type) {
            case CipherType::AES256:
                return std::make_unique<AESEngine>(256, mode);
            case CipherType::AES128:
                return std::make_unique<AESEngine>(128, mode);
            case CipherType::DES:
                return std::make_unique<DESEngine>();
            default:
                throw std::invalid_argument("Unknown cipher");
        }
    }
};

// Usage
auto cipher = CipherFactory::create(CipherType::AES256, CipherMode::GCM);
```

#### **Builder Pattern** - File Format Construction

```cpp
class FileFormatBuilder {
    FileHeader header_;
    std::vector<uint8_t> data_;
    
public:
    FileFormatBuilder& set_cipher(CipherType type, CipherMode mode) {
        header_.cipher_type = type;
        header_.cipher_mode = mode;
        return *this;
    }
    
    FileFormatBuilder& set_kdf(KDFType type, const KDFParams& params) {
        header_.kdf_type = type;
        header_.kdf_params = params;
        return *this;
    }
    
    FileFormatBuilder& set_data(std::vector<uint8_t> data) {
        data_ = std::move(data);
        return *this;
    }
    
    std::vector<uint8_t> build() {
        validate();
        return serialize(header_, data_);
    }
};

// Usage
auto file = FileFormatBuilder()
    .set_cipher(CipherType::AES256, CipherMode::GCM)
    .set_kdf(KDFType::Argon2id, params)
    .set_data(encrypted_data)
    .build();
```

#### **RAII** - Resource Management

```cpp
class SecureMemory {
    std::vector<uint8_t> data_;
    
public:
    explicit SecureMemory(size_t size) : data_(size) {}
    
    ~SecureMemory() {
        // Wipe memory before destruction
        OPENSSL_cleanse(data_.data(), data_.size());
    }
    
    uint8_t* data() { return data_.data(); }
    size_t size() const { return data_.size(); }
    
    // Delete copy, allow move
    SecureMemory(const SecureMemory&) = delete;
    SecureMemory& operator=(const SecureMemory&) = delete;
    SecureMemory(SecureMemory&&) = default;
    SecureMemory& operator=(SecureMemory&&) = default;
};
```

---

## 4. Architecture Design

### 4.1 Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  CLI Parser  │  │  Interactive │  │  Progress UI │      │
│  │  (CLI11)     │  │  Prompts     │  │  (indicators)│      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│                   Application Layer                          │
│  ┌────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │ Encryption     │  │ Compression     │  │ Steganography│ │
│  │ Service        │  │ Service         │  │ Service      │ │
│  └────────────────┘  └─────────────────┘  └──────────────┘ │
│  ┌────────────────┐  ┌─────────────────┐                   │
│  │ Hash Service   │  │ Benchmark Svc   │                   │
│  └────────────────┘  └─────────────────┘                   │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│                      Domain Layer                            │
│  ┌──────────────┐  ┌────────────────┐  ┌────────────────┐  │
│  │ Cipher       │  │ KDF            │  │ Compressor     │  │
│  │ Engines      │  │ Engines        │  │ Engines        │  │
│  └──────────────┘  └────────────────┘  └────────────────┘  │
│  ┌──────────────┐  ┌────────────────┐                      │
│  │ File Format  │  │ Secure Memory  │                      │
│  │ Handler      │  │ Manager        │                      │
│  └──────────────┘  └────────────────┘                      │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│                  Infrastructure Layer                        │
│  ┌──────────────┐  ┌────────────────┐  ┌────────────────┐  │
│  │ Botan Crypto │  │ File I/O       │  │ Image I/O      │  │
│  │ Wrapper      │  │ (std::fstream) │  │ (stb_image)    │  │
│  └──────────────┘  └────────────────┘  └────────────────┘  │
│  ┌──────────────┐  ┌────────────────┐                      │
│  │ Config       │  │ Logger         │                      │
│  │ Manager      │  │ (spdlog)       │                      │
│  └──────────────┘  └────────────────┘                      │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 Plugin Architecture (Mở rộng)

```cpp
// Plugin interface
class ICipherPlugin {
public:
    virtual ~ICipherPlugin() = default;
    
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
    
    virtual std::unique_ptr<ICipherEngine> create_cipher() = 0;
};

// Plugin registry
class PluginRegistry {
    std::map<std::string, std::unique_ptr<ICipherPlugin>> plugins_;
    
public:
    void register_plugin(std::unique_ptr<ICipherPlugin> plugin) {
        auto name = plugin->name();
        plugins_[name] = std::move(plugin);
    }
    
    ICipherEngine* get_cipher(const std::string& name) {
        if (auto it = plugins_.find(name); it != plugins_.end()) {
            return it->second->create_cipher();
        }
        throw std::runtime_error("Plugin not found: " + name);
    }
    
    std::vector<std::string> list_plugins() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : plugins_) {
            names.push_back(name);
        }
        return names;
    }
};

// Usage: Thêm thuật toán mới mà không sửa core code
class ChaCha20Plugin : public ICipherPlugin {
public:
    std::string name() const override { return "chacha20"; }
    std::string version() const override { return "1.0"; }
    
    std::unique_ptr<ICipherEngine> create_cipher() override {
        return std::make_unique<ChaCha20Engine>();
    }
};

// Register at startup
registry.register_plugin(std::make_unique<ChaCha20Plugin>());
```

### 4.3 Error Handling Architecture

```cpp
// Exception hierarchy
class FileVaultException : public std::exception {
protected:
    std::string message_;
    std::string context_;
    
public:
    FileVaultException(std::string msg, std::string ctx = "")
        : message_(std::move(msg)), context_(std::move(ctx)) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    const std::string& context() const { return context_; }
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

// Error handling wrapper
template<typename Func>
auto safe_execute(Func&& func, const std::string& operation) {
    try {
        return func();
    } catch (const AuthenticationFailedException& e) {
        std::cerr << "✗ Authentication failed\n";
        throw;
    } catch (const FileNotFoundException& e) {
        std::cerr << "✗ File not found: " << e.context() << '\n';
        throw;
    } catch (const FileVaultException& e) {
        std::cerr << "✗ Error in " << operation << ": " << e.what() << '\n';
        if (!e.context().empty()) {
            std::cerr << "  Context: " << e.context() << '\n';
        }
        throw;
    } catch (const std::exception& e) {
        std::cerr << "✗ Unexpected error: " << e.what() << '\n';
        throw;
    }
}
```

---

## 5. Folder Structure

### 5.1 Complete Project Structure

```
filevault/
│
├── .github/
│   └── workflows/
│       ├── build.yml                 # CI/CD: Build trên Win/Linux/Mac
│       ├── test.yml                  # Run tests
│       └── release.yml               # Tạo release binaries
│
├── cmake/
│   ├── CompilerWarnings.cmake        # Warning flags
│   ├── Sanitizers.cmake              # AddressSanitizer, UBSan
│   └── StaticAnalyzers.cmake         # Clang-tidy, cppcheck
│
├── docs/
│   ├── algorithms/                   # Algorithm docs (Obsidian style)
│   │   ├── _index.md
│   │   ├── classical/
│   │   │   ├── caesar.md
│   │   │   ├── vigenere.md
│   │   │   ├── playfair.md
│   │   │   └── substitution.md
│   │   ├── symmetric/
│   │   │   ├── aes.md
│   │   │   ├── des.md
│   │   │   └── modes-of-operation.md
│   │   ├── hash/
│   │   │   └── sha-family.md
│   │   ├── kdf/
│   │   │   ├── pbkdf2.md
│   │   │   └── argon2.md
│   │   ├── compression/
│   │   │   └── zstd.md
│   │   └── steganography/
│   │       └── lsb.md
│   │
│   ├── api/
│   │   ├── reference.md              # API documentation
│   │   └── examples.md
│   │
│   ├── design/
│   │   ├── architecture.md
│   │   ├── file-format.md
│   │   ├── security-considerations.md
│   │   └── performance.md
│   │
│   ├── user-guide/
│   │   ├── installation.md
│   │   ├── quick-start.md
│   │   ├── cli-reference.md
│   │   └── faq.md
│   │
│   └── development/
│       ├── building.md
│       ├── contributing.md
│       ├── testing.md
│       └── qa-guide.md               # Q&A for developers
│
├── include/                          # Public headers
│   └── filevault/
│       ├── filevault.hpp             # Main public API
│       ├── types.hpp                 # Common types/enums
│       ├── crypto/
│       │   ├── cipher.hpp
│       │   ├── kdf.hpp
│       │   └── hash.hpp
│       ├── compression/
│       │   └── compressor.hpp
│       ├── stego/
│       │   └── steganography.hpp
│       ├── core/
│       │   ├── file_format.hpp
│       │   └── secure_memory.hpp
│       └── exceptions.hpp
│
├── src/                              # Implementation
│   ├── crypto/
│   │   ├── cipher/
│   │   │   ├── aes_engine.cpp
│   │   │   ├── des_engine.cpp
│   │   │   └── classical/
│   │   │       ├── caesar.cpp
│   │   │       ├── vigenere.cpp
│   │   │       └── playfair.cpp
│   │   ├── kdf/
│   │   │   ├── pbkdf2_engine.cpp
│   │   │   └── argon2_engine.cpp
│   │   └── hash/
│   │       └── hash_service.cpp
│   │
│   ├── compression/
│   │   ├── zlib_compressor.cpp
│   │   ├── zstd_compressor.cpp
│   │   └── bzip2_compressor.cpp
│   │
│   ├── stego/
│   │   ├── lsb_steganography.cpp
│   │   └── image_handler.cpp
│   │
│   ├── core/
│   │   ├── file_format_handler.cpp
│   │   ├── file_handler.cpp
│   │   ├── secure_memory.cpp
│   │   └── random_generator.cpp
│   │
│   ├── services/
│   │   ├── encryption_service.cpp
│   │   ├── compression_service.cpp
│   │   ├── benchmark_service.cpp
│   │   └── config_service.cpp
│   │
│   ├── utils/
│   │   ├── base64.cpp
│   │   ├── hex.cpp
│   │   └── string_utils.cpp
│   │
│   └── CMakeLists.txt
│
├── cli/                              # CLI application
│   ├── main.cpp
│   ├── commands/
│   │   ├── encrypt_command.cpp
│   │   ├── decrypt_command.cpp
│   │   ├── hash_command.cpp
│   │   ├── compress_command.cpp
│   │   ├── stego_command.cpp
│   │   ├── info_command.cpp
│   │   ├── benchmark_command.cpp
│   │   └── config_command.cpp
│   │
│   ├── ui/
│   │   ├── progress_bar.cpp
│   │   ├── interactive_prompt.cpp
│   │   └── color_output.cpp
│   │
│   └── CMakeLists.txt
│
├── tests/                            # Unit tests
│   ├── unit/
│   │   ├── crypto/
│   │   │   ├── test_aes.cpp
│   │   │   ├── test_des.cpp
│   │   │   ├── test_classical.cpp
│   │   │   ├── test_kdf.cpp
│   │   │   └── test_hash.cpp
│   │   ├── compression/
│   │   │   └── test_compression.cpp
│   │   ├── stego/
│   │   │   └── test_lsb.cpp
│   │   └── core/
│   │       ├── test_file_format.cpp
│   │       └── test_secure_memory.cpp
│   │
│   ├── integration/
│   │   ├── test_encrypt_decrypt_flow.cpp
│   │   ├── test_compression_encryption.cpp
│   │   └── test_stego_encryption.cpp
│   │
│   ├── fixtures/                     # Test data
│   │   ├── sample.txt
│   │   ├── test_image.png
│   │   └── test_vectors/            # NIST test vectors
│   │       ├── aes_gcm.json
│   │       └── pbkdf2.json
│   │
│   └── CMakeLists.txt
│
├── benchmarks/                       # Performance benchmarks
│   ├── bench_crypto.cpp
│   ├── bench_compression.cpp
│   ├── bench_kdf.cpp
│   └── CMakeLists.txt
│
├── examples/                         # Example code
│   ├── simple_encrypt.cpp
│   ├── advanced_usage.cpp
│   ├── library_integration.cpp
│   └── CMakeLists.txt
│
├── scripts/                          # Build/utility scripts
│   ├── build.sh                      # Cross-platform build
│   ├── test.sh                       # Run all tests
│   ├── format.sh                     # Code formatting
│   ├── analyze.sh                    # Static analysis
│   └── release.sh                    # Create release package
│
├── third_party/                      # External dependencies (header-only)
│   ├── stb_image.h
│   ├── stb_image_write.h
│   └── CLI11.hpp
│
├── .clang-format                     # Code style
├── .clang-tidy                       # Linter config
├── .gitignore
├── CMakeLists.txt                    # Root CMake
├── conanfile.txt                     # Dependencies
├── LICENSE
├── README.md
└── CHANGELOG.md

```

### 5.2 Giải Thích Cấu Trúc

**Nguyên tắc tổ chức:**

1. **Separation of Concerns**
    
    - `include/`: Public API (headers người dùng import)
    - `src/`: Implementation details (private)
    - `cli/`: CLI application code riêng biệt với library core
2. **Modularity**
    
    - Mỗi module (crypto, compression, stego) có folder riêng
    - Dễ disable/enable modules khi build
    - Dễ thay thế implementation
3. **Testability**
    
    - Unit tests theo cấu trúc source code
    - Integration tests riêng biệt
    - Test fixtures/data tách ra
4. **Documentation First**
    
    - `docs/` ở top-level, quan trọng như code
    - Obsidian-compatible (wikilinks)
    - Sections: algorithms, API, design, user guide
5. **Clean Build Artifacts**
    
    ```
    # .gitignore
    build/
    build-*/
    *.o
    *.a
    *.so
    *.dylib
    *.exe
    .cache/
    compile_commands.json
    ```
    

---

## 6. Development Q&A Guide

### 6.1 Architecture & Design Questions

**Q: Tại sao dùng layered architecture thay vì monolithic?**

A:

- ✅ **Separation**: Business logic (services) tách khỏi infrastructure (Botan)
- ✅ **Testability**: Mock infrastructure layer dễ dàng
- ✅ **Maintainability**: Thay thế Botan → chỉ sửa infrastructure layer
- ✅ **Reusability**: Core library dùng được cho CLI, GUI, mobile

**Q: Tại sao dùng interface (abstract class) thay vì template?**

A:

```cpp
// ❌ Template approach
template<typename CipherT>
class EncryptionService {
    CipherT cipher_;
    // Phải biết concrete type lúc compile
    // Không thể runtime selection
};

// ✅ Interface approach
class EncryptionService {
    std::unique_ptr<ICipherEngine> cipher_;
    // Runtime selection: AES, DES, ChaCha20...
    // Plugin architecture possible
};
```

**Q: Làm sao mở rộng thêm thuật toán mới mà không sửa code cũ?**

A: Open-Closed Principle (OCP)

1. Implement interface:

```cpp
class NewCipherEngine : public ICipherEngine {
    // Implement required methods
};
```

2. Register vào factory:

```cpp
// In factory.cpp
case CipherType::NewCipher:
    return std::make_unique<NewCipherEngine>();
```

3. Update enum (1 dòng):

```cpp
enum class CipherType {
    AES256, DES, NewCipher  // ← Add here
};
```

**Q: File format có backward compatible không?**

A: Có, qua version field:

```cpp
struct FileHeader {
    uint8_t major_version;  // Breaking changes
    uint8_t minor_version;  // Backward compatible
};

// Parser
if (header.major_version != CURRENT_MAJOR) {
    throw UnsupportedVersionException();
}
// Minor version khác → vẫn parse được
```

### 6.2 Security Questions

**Q: Password được xử lý như thế nào trong memory?**

A:

1. **Input**: Dùng terminal echo off
2. **Storage**: `Botan::secure_vector` (locked memory, không swap)
3. **Cleanup**: Wipe ngay sau khi derive key

```cpp
void derive_and_wipe(const std::string& password) {
    Botan::secure_vector<uint8_t> pwd(password.begin(), password.end());
    auto key = kdf->derive_key(pwd, salt);
    
    // pwd tự động wipe khi destructor chạy
    return key;
}
```

**Q: Làm sao chống padding oracle attack?**

A:

1. **Dùng GCM** (AEAD) → không có padding, có auth tag
2. **CBC mode**: Encrypt-then-MAC

```cpp
// ❌ BAD: MAC-then-Encrypt
mac = HMAC(plaintext)
ciphertext = encrypt(plaintext || mac)

// ✅ GOOD: Encrypt-then-MAC
ciphertext = encrypt(plaintext)
mac = HMAC(ciphertext)
```

**Q: Nonce/IV reuse có nguy hiểm không?**

A: **CỰC KỲ nguy hiểm!**

- **CBC + IV reuse**: Leak XOR of plaintexts
- **GCM + nonce reuse**: CATASTROPHIC - leak key
- **CTR + nonce reuse**: Ciphertext XOR → plaintext

**Giải pháp:**

```cpp
Botan::AutoSeeded_RNG rng;
auto nonce = rng.random_vec(12);  // Mỗi lần encrypt
```

**Q: Argon2 vs PBKDF2 - chọn gì?**

A:

|Feature|PBKDF2|Argon2id|
|---|---|---|
|CPU cost|✅ High|✅ High|
|Memory cost|❌ Low (~KB)|✅ High (MB-GB)|
|ASIC resistance|⚠️ Medium|✅ High|
|Speed|Fast|Slow (feature!)|
|Standard|NIST SP 800-132|RFC 9106|

**Kết luận**: Argon2id cho production, PBKDF2 cho legacy/fast mode

### 6.3 Implementation Questions

**Q: Xử lý file lớn (>1GB) như thế nào?**

A: **Streaming encryption**

```cpp
// ❌ BAD: Load toàn bộ vào RAM
auto data = read_entire_file(path);  // OOM với file lớn
auto encrypted = cipher->encrypt(data);

// ✅ GOOD: Streaming
std::ifstream in(path, std::ios::binary);
std::ofstream out(output, std::ios::binary);

const size_t CHUNK_SIZE = 4 * 1024 * 1024;  // 4MB
std::vector<uint8_t> buffer(CHUNK_SIZE);

while (in.read((char*)buffer.data(), CHUNK_SIZE)) {
    size_t bytes_read = in.gcount();
    auto encrypted_chunk = cipher->process(buffer, bytes_read);
    out.write((char*)encrypted_chunk.data(), encrypted_chunk.size());
}
```

**Q: Progress bar cho CLI update như thế nào?**

A: Callback pattern

```cpp
using ProgressCallback = std::function<void(double percent)>;

class EncryptionService {
public:
    void encrypt_file(path, password, ProgressCallback cb = nullptr) {
        size_t total = get_file_size(path);
        size_t processed = 0;
        
        while (/* processing */) {
            // ... encrypt chunk ...
            processed += chunk_size;
            
            if (cb) {
                cb(100.0 * processed / total);
            }
        }
    }
};

// CLI usage
service.encrypt_file(path, pwd, [](double p) {
    std::cout << "\r[" << progress_bar(p) << "] " << p << "%";
    std::cout.flush();
});
```

**Q: Cross-platform path handling?**

A: Dùng `std::filesystem` (C++17)

```cpp
#include <filesystem>
namespace fs = std::filesystem;

// ✅ Works on Windows/Linux/Mac
fs::path input = "C:\\data\\file.txt";  // Windows
fs::path output = input.parent_path() / (input.stem().string() + ".fv");
// → C:\data\file.fv

// Path separator tự động: Windows '\', Unix '/'
```

**Q: Làm sao test code crypto mà không biết output?**

A: **NIST test vectors**

```cpp
TEST_CASE("AES-256-GCM NIST test vector") {
    // From NIST SP 800-38D
    auto key = hex_decode("000102030405060708090a0b0c0d0e0f"
                          "101112131415161718191a1b1c1d1e1f");
    auto iv = hex_decode("cafebabefacedbaddecaf888");
    auto plaintext = hex_decode("d9313225f88406e5a55909c5aff5269a");
    
    auto expected = hex_decode("42831ec2217774244b7221b784d0d49c");
    
    auto actual = aes_gcm_encrypt(plaintext, key, iv);
    
    REQUIRE(actual == expected);
}
```

### 6.4 Performance Questions

**Q: AES-NI có ảnh hưởng bao nhiêu?**

A:

- **Với AES-NI**: 4-8 GB/s (hardware)
- **Không AES-NI**: 100-200 MB/s (software)
- **Speed up**: ~40x

Check CPU support:

```bash
# Linux
grep aes /proc/cpuinfo

# Windows
wmic cpu get caption, /value | findstr AES
```

**Q: Nén trước hay mã hóa trước?**

A: **NÉN TRƯỚC**

```
Plaintext → Compress → Encrypt → Ciphertext
```

**Lý do:**

1. Encrypted data có entropy cao → không compress được
2. Compressed data nhỏ hơn → encrypt nhanh hơn

```cpp
// ✅ CORRECT
auto compressed = compressor->compress(plaintext);
auto encrypted = cipher->encrypt(compressed);

// ❌ WRONG
auto encrypted = cipher->encrypt(plaintext);
auto compressed = compressor->compress(encrypted);  // No effect!
```

**Q: Benchmark có ý nghĩa không khi máy khác nhau?**

A: Benchmark **relative** performance

```
AES-256-GCM:    1000 MB/s  (baseline)
AES-256-CBC:     850 MB/s  (0.85x)
ChaCha20:       1200 MB/s  (1.2x)
```

Tỷ lệ này **stable** giữa các máy (với cùng CPU generation)

### 6.5 Build & Deployment Questions

**Q: CMake vs Meson vs Bazel?**

A:

|Tool|Pros|Cons|Verdict|
|---|---|---|---|
|CMake|Phổ biến, IDE support|Verbose|✅ Chọn|
|Meson|Nhanh, clean syntax|Ít người dùng|⚠️ OK|
|Bazel|Reproducible, Google|Phức tạp|❌ Overkill|

**Q: Static linking hay dynamic linking?**

A: **Static** cho CLI tool

```cmake
# CMakeLists.txt
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Static linking Botan
find_package(Botan REQUIRED)
target_link_libraries(filevault PRIVATE Botan::Botan)
```

**Ưu điểm:**

- ✅ Single binary, không cần install deps
- ✅ Versioning đơn giản
- ❌ Binary size lớn hơn (~2-5 MB)

**Q: Conan vs vcpkg vs manual?**

A:

- **Conan**: Cross-platform tốt nhất, Python-based
- **vcpkg**: Microsoft, Windows-friendly
- **Manual**: Build from source (chậm, phức tạp)

**Khuyến nghị**: Conan cho development, static binary cho release

**Q: CI/CD setup ra sao?**

A: GitHub Actions (free)

```yaml
# .github/workflows/build.yml
name: Build
on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-22.04, windows-2022, macos-12]
        build_type: [Debug, Release]
    
    runs-on: ${{ matrix.os }}
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Conan
        run: pip install conan
      
      - name: Install dependencies
        run: |
          conan install . --build=missing \
            -s build_type=${{ matrix.build_type }}
      
      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
      
      - name: Build
        run: cmake --build build --config ${{ matrix.build_type }}
      
      - name: Test
        run: ctest --test-dir build -C ${{ matrix.build_type }}
      
      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: filevault-${{ matrix.os }}-${{ matrix.build_type }}
          path: build/cli/filevault*
```

---

## 7. SDLC Phases

### 7.1 Phase Overview

```
Requirements → Design → Implementation → Testing → Deployment → Maintenance
    ↑______________________________________________________________|
                        (Iterative feedback loop)
```

### 7.2 Phase 1: Requirements Analysis (1 tuần)

**Deliverables:**

- ✅ `requirements.md`: Functional + non-functional requirements
- ✅ `use_cases.md`: User personas, scenarios
- ✅ `constraints.md`: Technical, security, platform constraints

**Activities:**

1. Identify stakeholders (sinh viên, dev, security researcher)
2. Define use cases (educational, production, research)
3. List algorithms cần implement
4. Define success criteria

**Checklist:**

- [ ] 3 user personas documented
- [ ] 10+ use cases mapped
- [ ] Algorithm list finalized
- [ ] Performance targets defined (e.g., AES >= 100 MB/s)
- [ ] Security requirements reviewed

### 7.3 Phase 2: Design (2 tuần)

**Deliverables:**

- ✅ `architecture.md`: System architecture, layers
- ✅ `api_design.md`: Public API specifications
- ✅ `file_format.md`: .fv format specification
- ✅ `algorithm_docs/`: Chi tiết từng thuật toán
- ✅ `oop_design.md`: Class diagrams, patterns

**Activities:**

1. Design layered architecture
2. Define interfaces (ICipherEngine, IKDFEngine...)
3. Specify file format
4. Document algorithms
5. Design CLI commands
6. Plan folder structure

**Checklist:**

- [ ] Architecture diagram complete
- [ ] All interfaces defined
- [ ] File format versioned
- [ ] 10+ algorithm docs written
- [ ] CLI syntax finalized
- [ ] Design review passed

### 7.4 Phase 3: Environment Setup (3-4 ngày)

**Deliverables:**

- ✅ Working build system (CMake + Conan)
- ✅ CI/CD pipeline (GitHub Actions)
- ✅ Code quality tools (clang-format, clang-tidy)
- ✅ Test framework setup (Catch2)

**Activities:**

1. Setup CMake build
2. Configure Conan dependencies
3. Write GitHub Actions workflows
4. Setup linters/formatters
5. Create project template

**Checklist:**

- [ ] `cmake --build build` works on all platforms
- [ ] CI passes (green badge)
- [ ] Auto-formatting on commit
- [ ] Test command ready: `ctest`

### 7.5 Phase 4: Implementation (6-8 tuần)

**Sprint breakdown:**

#### Sprint 1: Foundation (1 tuần)

- [ ] File I/O utilities
- [ ] FileFormat parser/serializer
- [ ] Secure memory management
- [ ] Random number generator wrapper
- [ ] Unit tests for core

#### Sprint 2: Classical Ciphers (1 tuần)

- [ ] Caesar cipher
- [ ] Vigenère cipher
- [ ] Playfair cipher
- [ ] Unit tests + docs
- [ ] CLI `--mode basic`

#### Sprint 3: Modern Crypto - AES (1.5 tuần)

- [ ] Botan integration
- [ ] AES-256-GCM implementation
- [ ] AES-256-CBC implementation
- [ ] NIST test vectors
- [ ] Benchmarks

#### Sprint 4: KDF (1 tuần)

- [ ] PBKDF2 implementation
- [ ] Argon2 implementation
- [ ] KDF parameter tuning
- [ ] Tests: known answer, timing

#### Sprint 5: Compression (1 tuần)

- [ ] Zlib wrapper
- [ ] Zstd wrapper (optional)
- [ ] Compression ratio tests
- [ ] Integration: compress → encrypt

#### Sprint 6: CLI Application (1.5 tuần)

- [ ] CLI11 integration
- [ ] Command handlers (encrypt, decrypt, hash...)
- [ ] Interactive prompts
- [ ] Progress bars
- [ ] Error handling

#### Sprint 7: Steganography (1 tuần)

- [ ] stb_image integration
- [ ] LSB embedding
- [ ] LSB extraction
- [ ] Capacity calculation
- [ ] Tests with sample images

#### Sprint 8: Advanced Features (1 tuần)

- [ ] `info` command
- [ ] `benchmark` command
- [ ] `config` command
- [ ] Batch processing
- [ ] Polish UX

**Daily workflow:**

```bash
# 1. Pull latest
git pull origin main

# 2. Create feature branch
git checkout -b feature/aes-gcm

# 3. Code + test
# Write code, run tests locally

# 4. Format + lint
./scripts/format.sh
./scripts/analyze.sh

# 5. Commit
git add .
git commit -m "feat: implement AES-256-GCM"

# 6. Push + PR
git push origin feature/aes-gcm
# Open pull request on GitHub
```

### 7.6 Phase 5: Testing (2 tuần)

**Test levels:**

#### Unit Tests (per sprint)

```cpp
// tests/unit/crypto/test_aes.cpp
TEST_CASE("AES-256-GCM encryption round-trip") {
    auto key = random_bytes(32);
    auto nonce = random_bytes(12);
    auto plaintext = "Secret message";
    
    auto encrypted = aes_gcm_encrypt(plaintext, key, nonce);
    auto decrypted = aes_gcm_decrypt(encrypted, key, nonce);
    
    REQUIRE(decrypted == plaintext);
}

TEST_CASE("AES-256-GCM wrong password fails") {
    auto encrypted = aes_gcm_encrypt("data", key1, nonce);
    REQUIRE_THROWS_AS(
        aes_gcm_decrypt(encrypted, key2, nonce),
        AuthenticationFailedException
    );
}
```

#### Integration Tests

```cpp
// tests/integration/test_encrypt_decrypt_flow.cpp
TEST_CASE("Full encryption flow") {
    // Create temp file
    auto temp_file = create_temp_file("Hello World");
    auto encrypted_file = temp_file + ".fv";
    
    // Encrypt
    encrypt_file(temp_file, encrypted_file, "password");
    
    // Decrypt
    auto decrypted_file = encrypted_file + ".dec";
    decrypt_file(encrypted_file, decrypted_file, "password");
    
    // Verify
    REQUIRE(read_file(decrypted_file) == "Hello World");
}
```

#### End-to-End Tests (CLI)

```bash
# tests/e2e/test_cli.sh
#!/bin/bash

# Test encrypt + decrypt
echo "Secret" > test.txt
./filevault encrypt test.txt --password mypass
./filevault decrypt test.txt.fv --password mypass

# Verify
diff test.txt test.txt.fv.dec || exit 1

# Test wrong password
./filevault decrypt test.txt.fv --password wrong && exit 1

echo "✓ E2E tests passed"
```

#### Performance Tests

```cpp
// benchmarks/bench_crypto.cpp
static void BM_AES256_GCM_1MB(benchmark::State& state) {
    auto data = random_bytes(1024 * 1024);
    auto key = random_bytes(32);
    
    for (auto _ : state) {
        auto encrypted = aes_gcm_encrypt(data, key);
        benchmark::DoNotOptimize(encrypted);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_AES256_GCM_1MB);
```

**Test coverage target:** 80%+

```bash
# Generate coverage report
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build
ctest --test-dir build
gcovr -r . --html --html-details -o coverage.html
```

### 7.7 Phase 6: Documentation (1 tuần, parallel với testing)

**Deliverables:**

- ✅ README.md: Quick start, installation
- ✅ docs/user-guide/: CLI reference, tutorials
- ✅ docs/api/: API documentation (Doxygen)
- ✅ docs/algorithms/: Algorithm explanations
- ✅ CHANGELOG.md: Version history
- ✅ Video demo (5-10 phút)

**README.md structure:**

```markdown
# FileVault

[![Build](badge)](link) [![Coverage](badge)](link)

> Secure file encryption tool with multiple algorithms

## Features
- 🔒 AES-256, DES, classical ciphers
- 🔑 Argon2, PBKDF2 key derivation
- 📦 Zstd, Zlib compression
- 🖼️ LSB steganography
- 💻 Cross-platform CLI

## Quick Start
...

## Installation
...

## Usage Examples
...

## Documentation
- [User Guide](docs/user-guide/)
- [API Reference](docs/api/)
- [Algorithm Details](docs/algorithms/)

## Contributing
...

## License
MIT
```

### 7.8 Phase 7: Deployment & Release (3-4 ngày)

**Release checklist:**

```markdown
## Version 1.0.0 Release Checklist

### Code Quality
- [ ] All tests pass (unit, integration, e2e)
- [ ] Code coverage >= 80%
- [ ] No compiler warnings (-Wall -Wextra -Werror)
- [ ] Static analysis clean (clang-tidy, cppcheck)
- [ ] Memory leaks checked (Valgrind, ASan)

### Documentation
- [ ] README complete
- [ ] User guide written
- [ ] API docs generated
- [ ] CHANGELOG updated
- [ ] Video demo uploaded

### Binaries
- [ ] Windows x64 build
- [ ] Linux x64 build
- [ ] macOS x64/ARM build
- [ ] All binaries tested manually

### Distribution
- [ ] GitHub release created
- [ ] Binaries attached to release
- [ ] Installation instructions verified
- [ ] Homebrew formula (optional)
- [ ] Chocolatey package (optional)

### Marketing
- [ ] Demo video on YouTube
- [ ] Blog post written
- [ ] Social media announcement
- [ ] Submit to awesome-cpp list
```

**Release process:**

```bash
# 1. Version bump
./scripts/bump_version.sh 1.0.0

# 2. Update CHANGELOG
# Edit CHANGELOG.md

# 3. Tag release
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0

# 4. GitHub Actions auto-builds binaries

# 5. Create GitHub release
gh release create v1.0.0 \
  --title "FileVault 1.0.0" \
  --notes-file CHANGELOG.md \
  build/filevault-*.{exe,tar.gz,dmg}
```

### 7.9 Phase 8: Maintenance (Ongoing)

**Activities:**

- 🐛 Bug fixes: Hotfix releases (1.0.1, 1.0.2...)
- ✨ Feature requests: Minor releases (1.1.0, 1.2.0...)
- 🔒 Security patches: Immediate releases
- 📚 Documentation updates
- 💬 Community support (GitHub issues)

**Maintenance workflow:**

```
Issue reported → Triage → Fix → Test → Release
                  ↓
            Priority: Critical (hotfix) / Normal (next version)
```

---

## 8. Technology Stack

### 8.1 Core Stack

```yaml
Language: C++17
  Rationale: Performance, cross-platform, mature crypto libs
  Alternatives: Go (simpler), Rust (safer memory)

Build System: CMake 3.20+
  Rationale: Industry standard, IDE integration
  Alternatives: Meson (faster), Bazel (reproducible)

Package Manager: Conan 2.0
  Rationale: Cross-platform, binary caching
  Alternatives: vcpkg (Windows-focused)

Build Tool: Ninja
  Rationale: 2-3x faster than Make
  Alternatives: Make (standard), MSBuild (Windows)
```

### 8.2 Libraries

#### **Cryptography: Botan 3.2+**

```yaml
License: BSD-2-Clause (permissive)
Features:
  - AES, DES, 3DES (ECB, CBC, CTR, GCM...)
  - SHA-1, SHA-2, SHA-3, BLAKE2
  - PBKDF2, Argon2
  - Zlib, Bzip2, LZMA built-in
  - Hardware acceleration (AES-NI, SSE2)

Why not OpenSSL?
  - API: Botan modern C++ vs OpenSSL C-style
  - Docs: Botan excellent vs OpenSSL wiki-style
  - Argon2: Built-in vs external lib needed

Why not Crypto++?
  - No Argon2 built-in
  - Verbose API
  - Botan faster on modern CPUs
```

#### **CLI Parsing: CLI11**

```yaml
License: BSD-3-Clause
Type: Header-only
Features:
  - Intuitive syntax
  - Subcommands (encrypt, decrypt, hash...)
  - Validators (file exists, range check...)
  - Help generation

Alternative: Boost.Program_options (heavy)
```

#### **Testing: Catch2 v3**

```yaml
License: BSL-1.0
Type: Header-only (single-header available)
Features:
  - BDD-style (TEST_CASE, SECTION)
  - Rich assertions (REQUIRE, CHECK)
  - Benchmark integration
  - Data generators

Alternative: Google Test (more verbose)
```

#### **Benchmarking: Google Benchmark**

```yaml
License: Apache 2.0
Features:
  - Microsecond precision
  - Statistical analysis
  - Comparison between runs
  - CPU cycle counting

Usage:
  BENCHMARK(BM_AES256)->Range(1<<10, 1<<20);
```

#### **Image I/O: stb_image**

```yaml
License: MIT / Public Domain
Type: Single-header
Features:
  - PNG, JPG, BMP, TGA support
  - Simple API: stbi_load(), stbi_write_png()
  - No external dependencies

Size: ~7000 lines in 1 file
Perfect for LSB steganography
```

#### **Compression: Zstd (optional)**

```yaml
License: BSD + GPLv2
Features:
  - Better ratio than Zlib
  - Faster than Bzip2
  - Tunable levels (1-22)
  
Fallback: Botan's built-in Zlib sufficient
```

#### **Logging: spdlog (optional)**

```yaml
License: MIT
Type: Header-only
Features:
  - Fast (async logging)
  - Formatters (console, file, rotating)
  - Log levels

Usage:
  spdlog::info("Encrypted {} bytes", size);
```

### 8.3 Development Tools

```yaml
Compiler:
  - GCC 11+ (Linux)
  - Clang 14+ (macOS)
  - MSVC 2022 (Windows)
  Flags: -std=c++17 -Wall -Wextra -Werror

Linter: clang-tidy
  Checks: modernize-*, performance-*, bugprone-*

Formatter: clang-format
  Style: Google / LLVM (configurable)

Static Analysis:
  - cppcheck (open source)
  - PVS-Studio (commercial, optional)

Memory Checker:
  - Valgrind (Linux)
  - AddressSanitizer (all platforms)
  - LeakSanitizer

IDE Support:
  - VSCode (C++ extension)
  - CLion (CMake integration)
  - Visual Studio (Windows)
```

### 8.4 CI/CD

```yaml
Platform: GitHub Actions (free for public repos)

Workflows:
  - build.yml: Build on Win/Linux/Mac
  - test.yml: Run unit + integration tests
  - coverage.yml: Code coverage report
  - release.yml: Create release binaries

Alternatives:
  - GitLab CI (self-hosted)
  - Travis CI (limited free tier)
```

### 8.5 Documentation Tools

```yaml
Markdown: Standard markdown + Obsidian wikilinks
  Tools: Obsidian, VSCode + Markdown Preview

API Docs: Doxygen (optional)
  Generate: HTML reference from comments
  
Diagrams: Mermaid (in markdown)
  ```mermaid
  graph TD
    A[Plaintext] --> B[Compress]
    B --> C[Encrypt]
```

Video: OBS Studio (screen recording)

````

### 8.6 Complete Dependency List

```ini
# conanfile.txt
[requires]
botan/3.2.0               # Crypto (REQUIRED)
cli11/2.3.2               # CLI parsing (REQUIRED)
catch2/3.5.0              # Testing (REQUIRED)
benchmark/1.8.3           # Benchmarking (REQUIRED)
spdlog/1.12.0             # Logging (OPTIONAL)
zstd/1.5.5                # Compression (OPTIONAL)

[generators]
CMakeDeps
CMakeToolchain

[options]
botan/*:shared=False
botan/*:with_zlib=True
botan/*:with_bzip2=True
botan/*:with_sqlite=False
````

**Total binary size (static):**

- Core library: ~2-3 MB
- CLI + deps: ~4-5 MB
- With debug symbols: ~15-20 MB

---

## 9. Additional Considerations

### 9.1 Extensibility Design

#### Adding New Algorithms

**Plugin pattern giúp thêm algorithm mới:**

```cpp
// 1. Implement interface (10-50 lines)
class BlowfishEngine : public ICipherEngine {
    // ... implement methods
};

// 2. Register factory (1 line)
REGISTER_CIPHER(CipherType::Blowfish, BlowfishEngine);

// 3. Update enum (1 line)
enum class CipherType { AES256, DES, Blowfish };

// 4. Add CLI support (2 lines)
// In CLI parser:
.add_option("--algorithm", algo)
    .check(CLI::IsMember({"aes256", "des", "blowfish"}));
```

**Zero changes needed:**

- ❌ Không sửa `EncryptionService`
- ❌ Không sửa `FileFormatHandler`
- ❌ Không sửa existing tests

### 9.2 Performance Optimization

#### Compiler Optimizations

```cmake
# CMakeLists.txt
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(filevault PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-O3 -march=native -flto>
        $<$<CXX_COMPILER_ID:Clang>:-O3 -march=native -flto=thin>
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /GL>
    )
endif()
```

**Flags explained:**

- `-O3`: Aggressive optimizations
- `-march=native`: Use CPU-specific instructions (AES-NI, AVX2)
- `-flto`: Link-time optimization (inline across files)

#### Memory Optimization

```cpp
// ✅ Reserve capacity
std::vector<uint8_t> buffer;
buffer.reserve(file_size);  // Avoid reallocation

// ✅ Move instead of copy
return std::move(large_vector);

// ✅ In-place operations
cipher->finish(ciphertext);  // Modifies in-place, no copy
```

#### Parallel Processing (Future)

```cpp
// Encrypt multiple files in parallel
std::vector<std::future<void>> tasks;
for (const auto& file : files) {
    tasks.push_back(std::async(std::launch::async, 
        [&] { encrypt_file(file, password); }
    ));
}
// Wait all
for (auto& task : tasks) task.get();
```

### 9.3 Security Hardening

#### Constant-Time Operations

```cpp
// ❌ BAD: Timing attack possible
bool compare_password(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    for (size_t i = 0; i < a.length(); ++i) {
        if (a[i] != b[i]) return false;  // Early exit leaks info
    }
    return true;
}

// ✅ GOOD: Constant-time
bool compare_constant_time(const bytes& a, const bytes& b) {
    return Botan::constant_time_compare(a.data(), b.data(), a.size());
}
```

#### Memory Wiping

```cpp
// ✅ Wipe sensitive data before free
class SecureString {
    std::string data_;
public:
    ~SecureString() {
        OPENSSL_cleanse(&data_[0], data_.size());
    }
};
```

#### Input Validation

```cpp
// Validate before processing
void encrypt_file(const std::string& path, const std::string& password) {
    // File checks
    if (!fs::exists(path)) 
        throw FileNotFoundException(path);
    if (fs::file_size(path) > MAX_FILE_SIZE)
        throw FileTooLargeException();
    
    // Password checks
    if (password.length() < 8)
        throw WeakPasswordException("Min 8 characters");
    if (password.length() > 1024)
        throw PasswordTooLongException();
}
```

### 9.4 Error Recovery

#### Atomic File Operations

```cpp
void encrypt_file_safe(const std::string& input, 
                       const std::string& output) {
    fs::path temp_output = output + ".tmp";
    
    try {
        // Write to temporary file first
        std::ofstream temp(temp_output, std::ios::binary);
        // ... encrypt and write ...
        temp.close();
        
        // Atomic rename (commits the operation)
        fs::rename(temp_output, output);
    } catch (...) {
        // Cleanup on failure
        fs::remove(temp_output);
        throw;
    }
}
```

#### Progress Checkpointing (Advanced)

```cpp
// For very large files (>10GB)
struct EncryptionCheckpoint {
    size_t bytes_processed;
    std::vector<uint8_t> cipher_state;
};

void encrypt_with_checkpoint(path, output) {
    auto checkpoint_file = output + ".checkpoint";
    
    // Resume from checkpoint if exists
    if (fs::exists(checkpoint_file)) {
        auto cp = load_checkpoint(checkpoint_file);
        resume_encryption(cp);
    }
    
    // Save checkpoint every 1GB
    if (bytes_processed % (1024*1024*1024) == 0) {
        save_checkpoint(checkpoint_file, state);
    }
}
```

### 9.5 Cross-Platform Considerations

#### File Paths

```cpp
// ✅ Use std::filesystem
fs::path output = fs::path(input).replace_extension(".fv");

// ❌ Don't hardcode separators
std::string output = input + "\\" + "encrypted";  // Windows only
```

#### Terminal Colors

```cpp
// Detect color support
bool supports_color() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    return isatty(STDOUT_FILENO);
#endif
}

// Use ANSI codes only if supported
if (supports_color()) {
    std::cout << "\033[32m✓\033[0m Success\n";  // Green
} else {
    std::cout << "Success\n";
}
```

#### Line Endings

```cpp
// Binary mode for crypto (no CRLF conversion)
std::ifstream file(path, std::ios::binary);
std::ofstream out(path, std::ios::binary);
```

### 9.6 Future Enhancements

#### Phase 2 Features (Post-MVP)

```markdown
## Version 2.0 Roadmap

### Cryptography
- [ ] ChaCha20-Poly1305 (modern stream cipher)
- [ ] X25519 (Elliptic Curve Diffie-Hellman)
- [ ] Ed25519 signatures (file integrity)

### Compression
- [ ] LZMA (maximum ratio)
- [ ] Brotli (web-optimized)

### Steganography
- [ ] DCT-based (JPEG resistance)
- [ ] Audio steganography (WAV, MP3)

### Key Management
- [ ] Key derivation from keyfile (not just password)
- [ ] Multiple recipient support (encrypt to N keys)
- [ ] Key rotation (re-encrypt with new key)

### UI
- [ ] GUI application (Qt or Electron)
- [ ] VSCode extension
- [ ] Web interface (WASM)

### Advanced
- [ ] Cloud storage integration (encrypt before upload)
- [ ] Plausible deniability (hidden volumes)
- [ ] Secure deletion (overwrite files)
```

#### Library as Package

```cmake
# Install as system library
install(TARGETS filevault_lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/filevault
    DESTINATION include
)

# Generate pkg-config file
configure_file(filevault.pc.in filevault.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/filevault.pc
    DESTINATION lib/pkgconfig
)
```

**Usage by other projects:**

```cmake
# Other project's CMakeLists.txt
find_package(FileVault REQUIRED)
target_link_libraries(myapp PRIVATE FileVault::FileVault)
```

### 9.7 Community & Open Source

#### GitHub Repository Setup

```markdown
## Repository Structure

filevault/
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md
│   │   ├── feature_request.md
│   │   └── question.md
│   ├── PULL_REQUEST_TEMPLATE.md
│   └── workflows/
├── CODE_OF_CONDUCT.md
├── CONTRIBUTING.md
├── LICENSE
└── SECURITY.md            # Security policy, vulnerability reporting
```

#### CONTRIBUTING.md Template

```markdown
# Contributing to FileVault

## Development Setup
1. Fork the repository
2. Clone: `git clone https://github.com/you/filevault`
3. Build: `./scripts/build.sh`
4. Test: `./scripts/test.sh`

## Code Style
- Format: `./scripts/format.sh` (clang-format)
- Lint: `./scripts/analyze.sh` (clang-tidy)
- Follow Google C++ Style Guide

## Pull Request Process
1. Create feature branch: `git checkout -b feature/my-feature`
2. Write tests for new code (coverage >= 80%)
3. Update documentation if API changes
4. Run all tests locally
5. Submit PR with clear description

## Commit Messages
Format: `type(scope): description`

Types: feat, fix, docs, style, refactor, test, chore

Examples:
- `feat(crypto): add ChaCha20 cipher`
- `fix(cli): handle empty password input`
- `docs(api): update encryption examples`

## Reporting Bugs
Use [bug report template](.github/ISSUE_TEMPLATE/bug_report.md)

Include:
- OS and version
- FileVault version
- Steps to reproduce
- Expected vs actual behavior

## Security Issues
Email: security@example.com (private disclosure)
Do NOT open public issues for security bugs
```

### 9.8 Licensing

**Recommendation: MIT License**

```
MIT License

Copyright (c) 2024 [Your Name]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

[Full MIT text...]
```

**Why MIT?**

- ✅ Permissive (commercial use allowed)
- ✅ Compatible with Botan (BSD)
- ✅ Simple, widely understood
- ✅ OSI approved

**Alternatives:**

- **Apache 2.0**: Patent protection
- **GPL v3**: Copyleft (derivatives must be open-source)
- **BSD 3-Clause**: Similar to MIT

### 9.9 Marketing & Visibility

#### README Badges

```markdown
[![Build](https://github.com/you/filevault/workflows/build/badge.svg)](link)
[![Coverage](https://codecov.io/gh/you/filevault/badge.svg)](link)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/you/filevault)](releases)
```

#### SEO Keywords

```yaml
GitHub Topics:
  - cryptography
  - encryption
  - aes
  - security
  - cli-tool
  - cross-platform
  - cpp17
  - file-encryption
```

#### Submission Lists

```markdown
- [Awesome C++](https://github.com/fffaraz/awesome-cpp)
- [Awesome Security](https://github.com/sbilly/awesome-security)
- [Awesome CLI Apps](https://github.com/agarrharr/awesome-cli-apps)
- Reddit: r/cpp, r/programming, r/netsec
- Hacker News (Show HN)
```

---

## 10. Summary & Next Steps

### 10.1 Design Principles Summary

```
✅ SOLID Principles
  - Single Responsibility
  - Open/Closed (extensible)
  - Liskov Substitution (interfaces)
  - Interface Segregation
  - Dependency Inversion

✅ Clean Architecture
  - Layered design
  - Domain logic independent of UI/infrastructure
  - Dependency rule (inward only)

✅ KISS (Keep It Simple)
  - Simple CLI commands
  - Sensible defaults
  - Progressive disclosure

✅ DRY (Don't Repeat Yourself)
  - Strategy pattern for algorithms
  - Factory for object creation
  - Base classes for shared logic

✅ Security by Design
  - Secure defaults (AES-256-GCM, Argon2)
  - Input validation
  - Constant-time operations
  - Memory wiping
```

### 10.2 Project Checklist

**Before starting code:**

- [ ] Read all algorithm docs (docs/algorithms/)
- [ ] Understand file format spec
- [ ] Review OOP design + interfaces
- [ ] Setup development environment
- [ ] Create GitHub repository
- [ ] Setup CI/CD (GitHub Actions)

**During development:**

- [ ] Write tests FIRST (TDD)
- [ ] Document as you go
- [ ] Commit frequently (atomic commits)
- [ ] Run linters before push
- [ ] Keep PRs small (<500 lines)

**Before release:**

- [ ] All tests pass
- [ ] Code coverage >= 80%
- [ ] Documentation complete
- [ ] Manual testing on all platforms
- [ ] Security review
- [ ] Performance benchmarks
- [ ] Create demo video

### 10.3 Timeline Summary

```
Week 1:    Requirements + Design + Setup
Week 2-3:  Classical ciphers + Foundation
Week 4-5:  Modern crypto (AES, KDF)
Week 6:    Compression + Integration
Week 7-8:  CLI + UX + Steganography
Week 9:    Testing + Bug fixes
Week 10:   Documentation + Demo
Week 11:   Polishing + Performance
Week 12:   Release preparation

Total: 12 weeks (3 months)
```

### 10.4 Success Metrics

```yaml
Code Quality:
  - Test coverage: >= 80%
  - Compiler warnings: 0
  - Static analysis issues: 0
  - Code review approval: Required

Performance:
  - AES-256 encryption: >= 100 MB/s (software)
  - Argon2 KDF: <= 3 seconds (default params)
  - Binary size: <= 5 MB (static)
  - Memory usage: <= 100 MB (streaming mode)

Usability:
  - CLI help text completeness
  - Error messages clarity
  - Installation instructions
  - Example code provided

Security:
  - No hardcoded keys/passwords
  - Memory wiping verified
  - Input validation complete
  - External security audit (optional)

Documentation:
  - README: Complete
  - User guide: 10+ pages
  - API docs: 100% coverage
  - Algorithm docs: 10+ files
  - Video demo: Published
```

### 10.5 Risk Mitigation

|Risk|Impact|Mitigation|
|---|---|---|
|Crypto bug|HIGH|Use battle-tested Botan, NIST test vectors|
|Cross-platform issues|MED|CI/CD on all platforms, std::filesystem|
|Performance slow|MED|Benchmark early, use hardware acceleration|
|Scope creep|MED|MVP first, v2.0 features later|
|Deadline slip|LOW|Iterative sprints, cut non-essential features|

---

## 📚 References

### Books

- "Applied Cryptography" by Bruce Schneier
- "The Code Book" by Simon Singh
- "Serious Cryptography" by Jean-Philippe Aumasson

### Standards

- FIPS 197: AES
- NIST SP 800-38D: GCM
- RFC 9106: Argon2
- NIST SP 800-132: PBKDF2

### Documentation

- [Botan Handbook](https://botan.randombit.net/handbook/)
- [Cryptography Best Practices](https://gist.github.com/atoponce/07d8d4c833873be2f68c34f9afc5a78a)
- [OWASP Cryptographic Storage Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Cryptographic_Storage_Cheat_Sheet.html)

### Tools

- [Compiler Explorer](https://godbolt.org/) - View assembly
- [CppInsights](https://cppinsights.io/) - See compiler transformations
- [Quick Bench](https://quick-bench.com/) - Online benchmarks

---

## 🎯 Final Words

> "Premature optimization is the root of all evil" - Donald Knuth

**Focus order:**

1. **Correctness** - Crypto must be correct
2. **Security** - No shortcuts on security
3. **Usability** - Simple, intuitive CLI
4. **Performance** - Optimize after profiling
5. **Features** - MVP first, v2.0 later

**Development philosophy:**

- ✅ Make it work
- ✅ Make it right
- ✅ Make it fast
- ✅ Make it beautiful

**Remember:**

- Document decisions (this file!)
- Test everything
- Review code thoroughly
- Ask for help when stuck
- Iterate and improve

---

**Document Version:** 1.0  
**Last Updated:** 2024-11-11  
**Maintained by:** [Your Name]  
**Repository:** https://github.com/you/filevault

**Questions?** Open an issue or discussion on GitHub.

---

_End of Design Document_




---

Đã hoàn thành! 🎉

Tôi vừa tạo một **tài liệu thiết kế toàn diện** cho dự án FileVault của bạn, bao gồm:

## 📋 Nội dung đã cover:

### ✅ 1. **CLI Command Design** (Thiết kế lệnh)

- Cú pháp đơn giản, trực quan
- 8 commands chính: encrypt, decrypt, hash, compress, stego, info, benchmark, config
- Interactive mode với prompts đẹp
- Error handling rõ ràng

### ✅ 2. **Algorithm Documentation** (Obsidian style)

- Cấu trúc folder algorithms/ với wikilinks
- 2 ví dụ chi tiết: `caesar.md` và `aes.md`
- Template cho việc viết docs các thuật toán khác
- Bao gồm cả classical (Caesar, Vigenère) và modern (AES, DES)

### ✅ 3. **OOP Design**

- Class diagrams (text-based UML)
- Interfaces: ICipherEngine, IKDFEngine, ICompressor
- Design patterns: Strategy, Factory, Builder, RAII
- Plugin architecture để mở rộng

### ✅ 4. **Architecture Design**

- Layered architecture (4 layers)
- Data flow diagrams
- Error handling hierarchy
- Plugin system cho extensibility

### ✅ 5. **Folder Structure**

- Cấu trúc thư mục chi tiết, clean
- Tách biệt: include/ (public) vs src/ (private)
- Tests, benchmarks, examples riêng biệt
- Docs tổ chức theo topics

### ✅ 6. **Development Q&A Guide**

- 20+ câu hỏi thường gặp + trả lời
- Architecture, Security, Implementation, Performance
- Build & Deployment questions
- Code examples minh họa

### ✅ 7. **SDLC Phases**

- 8 phases chi tiết
- Timeline: 12 tuần (3 tháng)
- Sprint breakdown cụ thể
- Checklist cho từng phase

### ✅ 8. **Technology Stack**

- Core: C++17 + CMake + Conan
- Libraries: Botan (crypto), CLI11, Catch2, stb_image
- Development tools: clang-tidy, clang-format, Valgrind
- CI/CD: GitHub Actions

### ✅ 9. **Additional Considerations**

- Extensibility (plugin pattern)
- Performance optimization
- Security hardening
- Error recovery
- Cross-platform considerations
- Future enhancements (v2.0 roadmap)
- Open source strategy

## 🎯 Điểm nổi bật:

1. **Logic & Clean**:
    
    - Layered architecture rõ ràng
    - Interfaces tách biệt concerns
    - SOLID principles throughout
2. **Cross-platform First**:
    
    - `std::filesystem` cho paths
    - CMake + Conan cho build
    - CI/CD test trên 3 OS
3. **Extensible**:
    
    - Thêm algorithm mới: chỉ 1 class + 2 dòng code
    - Plugin architecture
    - Factory pattern
4. **Security by Design**:
    
    - Constant-time operations
    - Memory wiping
    - Input validation
    - Authenticated encryption (GCM)

## 📝 Next Steps cho bạn:

### Bước 1: Đọc kỹ tài liệu (1-2 ngày)

- [ ] Section 2: Algorithm docs (hiểu thuật toán)
- [ ] Section 3-4: OOP + Architecture (hiểu design)
- [ ] Section 6: Q&A (hiểu implementation)

### Bước 2: Setup môi trường (1 ngày)

```bash
# Install tools
choco install cmake ninja conan -y  # Windows
# hoặc
sudo apt install cmake ninja-build  # Ubuntu

# Clone template
git clone https://github.com/you/filevault
cd filevault

# Setup
./scripts/build.sh
```

### Bước 3: Viết docs algorithms còn lại (2-3 ngày)

Dựa theo template `aes.md`, viết:

- `des.md`, `3des.md`
- `vigenere.md`, `playfair.md`
- `sha-family.md`
- `pbkdf2.md`, `argon2.md`
- `zstd.md`, `lsb.md`

### Bước 4: Bắt đầu code (từ tuần 2)

Theo sprint plan trong Section 7.5

## 💡 Tips:

1. **Không cần code mọi thứ từ đầu**:
    
    - Botan đã có sẵn AES, DES, PBKDF2, Argon2
    - Bạn chỉ cần wrap vào interfaces của mình
2. **TDD (Test-Driven Development)**:
    
    - Viết test trước → code sau
    - Dùng NIST test vectors
3. **Incremental**:
    
    - Làm classical ciphers trước (đơn giản)
    - Sau đó mới AES-GCM (phức tạp)
4. **Git workflow**:
    
    ```bash
    git checkout -b feature/caesar-cipher
    # Code + test
    git commit -m "feat(crypto): implement Caesar cipher"
    git push origin feature/caesar-cipher
    # Open PR
    ```
    

Bạn muốn tôi giải thích chi tiết phần nào, hoặc cần thêm ví dụ code không? 😊