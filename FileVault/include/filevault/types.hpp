#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace filevault {

// Common type aliases
using Bytes = std::vector<uint8_t>;
using SecureBytes = std::vector<uint8_t>; // TODO: Use Botan::secure_vector

// Enumerations
enum class CipherType {
    // Classical
    CAESAR,
    VIGENERE,
    PLAYFAIR,
    
    // Modern symmetric
    AES128,
    AES192,
    AES256,
    DES,
    TRIPLE_DES,
    CHACHA20
};

enum class CipherMode {
    ECB,    // Not recommended
    CBC,
    CTR,
    GCM,    // Recommended (AEAD)
    CCM
};

enum class KDFType {
    PBKDF2,
    ARGON2ID,
    ARGON2I,
    ARGON2D
};

enum class CompressionType {
    NONE,
    ZLIB,
    ZSTD,
    BZIP2,
    LZMA
};

enum class HashAlgorithm {
    MD5,        // Legacy only
    SHA1,       // Legacy only
    SHA256,
    SHA512,
    SHA3_256,
    SHA3_512,
    BLAKE2B
};

// File format version
struct FormatVersion {
    uint8_t major;
    uint8_t minor;
};

// Magic bytes for .fv file format
constexpr uint8_t FILEVAULT_MAGIC[4] = {'F', 'V', 'L', 'T'};
constexpr FormatVersion CURRENT_VERSION = {1, 0};

} // namespace filevault
