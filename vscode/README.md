# FileVault Extension for Visual Studio Code

Secure file encryption directly in VS Code using the FileVault cryptographic toolkit.

## Features

- 🔐 **Encrypt Files** - Right-click any file to encrypt with modern cryptographic algorithms
- 🔓 **Decrypt Files** - Right-click `.fvlt` files to decrypt with password
- ℹ️ **File Info** - View encryption metadata (algorithm, timestamps, security level)
- 🔑 **Key Generation** - Generate RSA, ECC, Kyber, and Dilithium key pairs
- 📊 **Benchmark** - Run performance benchmarks on cryptographic algorithms
- #️⃣ **Hash Files** - Calculate file hashes (SHA-256, SHA-512, BLAKE2, SHA3)

## Requirements

- FileVault CLI must be installed and accessible
- Configure the path to FileVault executable in settings

## Installation

1. Build the extension: `npm install && npm run compile`
2. Package: `npx @vscode/vsce package`
3. Install the `.vsix` file in VS Code

## Configuration

| Setting | Description | Default |
|---------|-------------|---------|
| `filevault.executablePath` | Path to FileVault executable | `filevault` |
| `filevault.defaultAlgorithm` | Default encryption algorithm | `AES-256-GCM` |
| `filevault.securityLevel` | Security level for PQC algorithms | `3` |

## Usage

### Encrypt a File
1. Right-click a file in Explorer
2. Select **FileVault: Encrypt File**
3. Enter a password
4. The encrypted file will be created with `.fvlt` extension

### Decrypt a File
1. Right-click a `.fvlt` file in Explorer
2. Select **FileVault: Decrypt File**
3. Enter the password
4. The decrypted file will be created without the `.fvlt` extension

### Generate Keys
1. Open Command Palette (`Ctrl+Shift+P`)
2. Run **FileVault: Generate Key Pair**
3. Select algorithm type (RSA, ECC, Kyber, Dilithium)
4. Key files will be created in your workspace

## Supported Algorithms

### AEAD (Authenticated Encryption)
- AES-256-GCM (Recommended)
- AES-128-GCM
- AES-256-SIV
- ChaCha20-Poly1305
- XChaCha20-Poly1305

### Block Cipher Modes
- AES-256-CBC
- AES-128-CBC
- Camellia-256-CBC
- Serpent-256-CBC
- Twofish-256-CBC

### Post-Quantum Cryptography
- Kyber-512/768/1024 (Key Encapsulation)
- Dilithium2/3/5 (Digital Signatures)
- SPHINCS+-SHA2-128f/256s (Stateless Signatures)
- FrodoKEM-640/976/1344 (Conservative KEM)

### Classical Ciphers
- Caesar, Vigenère, Playfair
- Hill, Substitution

## Development

```bash
# Install dependencies
npm install

# Compile TypeScript
npm run compile

# Watch mode
npm run watch

# Package extension
npx @vscode/vsce package
```
1. Using Command Palette
- `Ctrl + Shift + P`
- FileVault: Set Executable Path
- Select `filevault.exe`
or
2. Settings
- `Ctrl + ,` (Settings)
- Find filevault.executablePath
- Type filevault.exe
or
3. settings.json
```json
{
    "filevault.executablePath": "D:\\code\\filevault\\build\\build\\Release\\bin\\release\\filevault.exe"
}
```


## License

MIT License - See LICENSE file for details.
