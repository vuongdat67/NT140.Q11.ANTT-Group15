# FileVault Documentation

Tài liệu hướng dẫn sử dụng và học tập về các thuật toán mã hóa trong FileVault.

## 📚 API Documentation

### Doxygen (API Reference)
- **Location**: [`doxygen/html/index.html`](doxygen/html/index.html)
- **Features**: 
  - Class hierarchy diagrams
  - Full API reference với source code
  - Search functionality
  - Cross-referenced documentation

**Mở docs**: Double-click file `docs/doxygen/html/index.html` trong browser.

## 🔐 Algorithm Categories

### 1. Classical Ciphers (`src/algorithms/classical/`)
Các thuật toán mã hóa cổ điển - dùng cho mục đích học tập.

| Algorithm | File | Description |
|-----------|------|-------------|
| Caesar | `caesar.cpp` | Shift cipher đơn giản |
| Vigenère | `vigenere.cpp` | Polyalphabetic substitution |
| Playfair | `playfair.cpp` | Digraph substitution cipher |
| Hill | `hill.cpp` | Matrix-based cipher |
| Substitution | `substitution.cpp` | Monoalphabetic substitution |

### 2. Symmetric Encryption (`src/algorithms/symmetric/`)
Thuật toán mã hóa đối xứng hiện đại.

| Algorithm | File | Mode | Description |
|-----------|------|------|-------------|
| AES-GCM | `aes_gcm.cpp` | AEAD | ⭐ Recommended - Authenticated encryption |
| AES-CBC | `aes_cbc.cpp` | Block | Classic block cipher mode |
| AES-CTR | `aes_ctr.cpp` | Stream | Counter mode |
| AES-XTS | `aes_xts.cpp` | Disk | Disk encryption mode |
| ChaCha20-Poly1305 | `chacha20_poly1305.cpp` | AEAD | Modern stream cipher |
| Camellia-GCM | `camellia_gcm.cpp` | AEAD | Japanese standard |
| ARIA-GCM | `aria_gcm.cpp` | AEAD | Korean standard |
| Serpent-GCM | `serpent_gcm.cpp` | AEAD | AES finalist |
| Twofish-GCM | `twofish_gcm.cpp` | AEAD | AES finalist |
| SM4-GCM | `sm4_gcm.cpp` | AEAD | Chinese standard |
| 3DES | `triple_des.cpp` | Legacy | Legacy support |

### 3. Asymmetric Encryption (`src/algorithms/asymmetric/`)
Thuật toán mã hóa bất đối xứng.

| Algorithm | File | Description |
|-----------|------|-------------|
| RSA | `rsa.cpp` | 2048/3072/4096-bit RSA |
| ECC | `ecc.cpp` | ECDH, ECDSA với P-256/P-384/P-521 |

### 4. Post-Quantum Cryptography (`src/algorithms/pqc/`)
Thuật toán kháng lượng tử (NIST PQC).

| Algorithm | File | Description |
|-----------|------|-------------|
| Kyber | `post_quantum.cpp` | ML-KEM (Key Encapsulation) |
| Dilithium | `post_quantum.cpp` | ML-DSA (Digital Signature) |

## 🔄 Regenerate Documentation

### Doxygen
```powershell
cd D:\code\filevault
doxygen Doxyfile
```

## 📖 Learning Resources

- **Doxygen HTML**: Xem class diagrams và API reference
- **Source Code**: Đọc implementation với comments chi tiết
- **Test Cases**: Xem `tests/` folder cho examples

## 🛠️ Configuration Files

| File | Purpose |
|------|---------|
| `Doxyfile` | Doxygen configuration |
