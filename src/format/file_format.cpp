#include "filevault/core/file_format.hpp"
#include <fstream>
#include <cstring>
#include <stdexcept>

namespace filevault {
namespace core {

// ============================================================================
// Argon2Params
// ============================================================================

std::vector<uint8_t> Argon2Params::serialize() const {
    std::vector<uint8_t> data(12);
    std::memcpy(data.data() + 0, &memory_kb, 4);
    std::memcpy(data.data() + 4, &iterations, 4);
    std::memcpy(data.data() + 8, &parallelism, 4);
    return data;
}

Argon2Params Argon2Params::deserialize(std::span<const uint8_t> data) {
    if (data.size() < 12) {
        throw std::runtime_error("Invalid Argon2 params size");
    }
    
    Argon2Params params;
    std::memcpy(&params.memory_kb, data.data() + 0, 4);
    std::memcpy(&params.iterations, data.data() + 4, 4);
    std::memcpy(&params.parallelism, data.data() + 8, 4);
    return params;
}

// ============================================================================
// PBKDF2Params
// ============================================================================

std::vector<uint8_t> PBKDF2Params::serialize() const {
    std::vector<uint8_t> data(4);
    std::memcpy(data.data(), &iterations, 4);
    return data;
}

PBKDF2Params PBKDF2Params::deserialize(std::span<const uint8_t> data) {
    if (data.size() < 4) {
        throw std::runtime_error("Invalid PBKDF2 params size");
    }
    
    PBKDF2Params params;
    std::memcpy(&params.iterations, data.data(), 4);
    return params;
}

// ============================================================================
// ScryptParams
// ============================================================================

std::vector<uint8_t> ScryptParams::serialize() const {
    std::vector<uint8_t> data(12);
    std::memcpy(data.data() + 0, &n, 4);
    std::memcpy(data.data() + 4, &r, 4);
    std::memcpy(data.data() + 8, &p, 4);
    return data;
}

ScryptParams ScryptParams::deserialize(std::span<const uint8_t> data) {
    if (data.size() < 12) {
        throw std::runtime_error("Invalid Scrypt params size");
    }
    
    ScryptParams params;
    std::memcpy(&params.n, data.data() + 0, 4);
    std::memcpy(&params.r, data.data() + 4, 4);
    std::memcpy(&params.p, data.data() + 8, 4);
    return params;
}

// ============================================================================
// FileHeader
// ============================================================================

bool FileHeader::is_valid() const {
    return std::memcmp(magic, FILE_FORMAT_MAGIC, 8) == 0;
}

size_t FileHeader::size() const {
    return 8 +  // magic
           2 +  // version
           1 +  // algorithm
           1 +  // kdf
           1 +  // compression
           3 +  // reserved
           salt.size() +
           4 +  // kdf_params length prefix
           kdf_params.size() +
           nonce.size() +
           1;   // compressed flag
}

std::vector<uint8_t> FileHeader::serialize() const {
    std::vector<uint8_t> data;
    data.reserve(size());
    
    // Magic bytes
    data.insert(data.end(), magic, magic + 8);
    
    // Version
    data.push_back(version_major);
    data.push_back(version_minor);
    
    // Algorithm, KDF, Compression
    data.push_back(static_cast<uint8_t>(algorithm));
    data.push_back(static_cast<uint8_t>(kdf));
    data.push_back(static_cast<uint8_t>(compression));
    
    // Reserved
    data.insert(data.end(), reserved, reserved + 3);
    
    // Salt
    data.insert(data.end(), salt.begin(), salt.end());
    
    // KDF params (length-prefixed)
    uint32_t kdf_params_len = static_cast<uint32_t>(kdf_params.size());
    uint8_t len_bytes[4];
    std::memcpy(len_bytes, &kdf_params_len, 4);
    data.insert(data.end(), len_bytes, len_bytes + 4);
    data.insert(data.end(), kdf_params.begin(), kdf_params.end());
    
    // Nonce
    data.insert(data.end(), nonce.begin(), nonce.end());
    
    // Compressed flag
    data.push_back(compressed ? 0x01 : 0x00);
    
    return data;
}

std::pair<FileHeader, size_t> FileHeader::deserialize(std::span<const uint8_t> data) {
    FileHeader header;
    size_t offset = 0;
    
    // Check minimum size
    if (data.size() < 16) {
        throw std::runtime_error("File too small to contain valid header");
    }
    
    // Magic bytes
    std::memcpy(header.magic, data.data() + offset, 8);
    offset += 8;
    
    if (!header.is_valid()) {
        throw std::runtime_error("Invalid file format magic bytes");
    }
    
    // Version
    header.version_major = data[offset++];
    header.version_minor = data[offset++];
    
    // Algorithm, KDF, Compression
    header.algorithm = static_cast<AlgorithmID>(data[offset++]);
    header.kdf = static_cast<KDFID>(data[offset++]);
    header.compression = static_cast<CompressionID>(data[offset++]);
    
    // Reserved
    std::memcpy(header.reserved, data.data() + offset, 3);
    offset += 3;
    
    // Salt (32 bytes)
    if (data.size() < offset + 32) {
        throw std::runtime_error("File too small for salt");
    }
    header.salt.assign(data.begin() + offset, data.begin() + offset + 32);
    offset += 32;
    
    // KDF params length
    if (data.size() < offset + 4) {
        throw std::runtime_error("File too small for KDF params length");
    }
    uint32_t kdf_params_len;
    std::memcpy(&kdf_params_len, data.data() + offset, 4);
    offset += 4;
    
    // KDF params
    if (data.size() < offset + kdf_params_len) {
        throw std::runtime_error("File too small for KDF params");
    }
    header.kdf_params.assign(data.begin() + offset, data.begin() + offset + kdf_params_len);
    offset += kdf_params_len;
    
    // Nonce (12 bytes)
    if (data.size() < offset + 12) {
        throw std::runtime_error("File too small for nonce");
    }
    header.nonce.assign(data.begin() + offset, data.begin() + offset + 12);
    offset += 12;
    
    // Compressed flag
    if (data.size() < offset + 1) {
        throw std::runtime_error("File too small for compressed flag");
    }
    header.compressed = (data[offset++] == 0x01);
    
    return {header, offset};
}

// ============================================================================
// FileFormatHandler
// ============================================================================

FileHeader FileFormatHandler::create_header(
    AlgorithmType algo_type,
    KDFType kdf_type,
    const EncryptionConfig& config,
    std::span<const uint8_t> salt,
    std::span<const uint8_t> nonce,
    bool compressed
) {
    FileHeader header;
    
    // Magic and version
    std::memcpy(header.magic, FILE_FORMAT_MAGIC, 8);
    header.version_major = FILE_FORMAT_VERSION_MAJOR;
    header.version_minor = FILE_FORMAT_VERSION_MINOR;
    
    // Algorithm and KDF
    header.algorithm = to_algorithm_id(algo_type);
    header.kdf = to_kdf_id(kdf_type);
    
    // Compression
    std::string comp_str;
    switch (config.compression) {
        case CompressionType::ZLIB: comp_str = "zlib"; break;
        case CompressionType::BZIP2: comp_str = "bzip2"; break;
        case CompressionType::LZMA: comp_str = "lzma"; break;
        default: comp_str = "none"; break;
    }
    header.compression = to_compression_id(comp_str);
    
    // Reserved (zeros)
    std::memset(header.reserved, 0, 3);
    
    // Salt and nonce
    header.salt.assign(salt.begin(), salt.end());
    header.nonce.assign(nonce.begin(), nonce.end());
    
    // Compressed flag
    header.compressed = compressed;
    
    // KDF params
    if (kdf_type == KDFType::ARGON2ID || kdf_type == KDFType::ARGON2I) {
        Argon2Params params;
        params.memory_kb = config.kdf_memory_kb;
        params.iterations = config.kdf_iterations;
        params.parallelism = config.kdf_parallelism;
        header.kdf_params = params.serialize();
    } else if (kdf_type == KDFType::PBKDF2_SHA256 || kdf_type == KDFType::PBKDF2_SHA512) {
        PBKDF2Params params;
        params.iterations = config.kdf_iterations;
        header.kdf_params = params.serialize();
    } else if (kdf_type == KDFType::SCRYPT) {
        ScryptParams params;
        // Use defaults since EncryptionConfig doesn't have scrypt-specific fields
        // params.n, params.r, params.p are already set to defaults in struct
        header.kdf_params = params.serialize();
    }
    
    return header;
}

bool FileFormatHandler::write_file(
    const std::string& path,
    const FileHeader& header,
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> auth_tag
) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        
        // Write header
        auto header_data = header.serialize();
        file.write(reinterpret_cast<const char*>(header_data.data()), header_data.size());
        
        // Write ciphertext
        file.write(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size());
        
        // Write auth tag
        file.write(reinterpret_cast<const char*>(auth_tag.data()), auth_tag.size());
        
        return true;
    } catch (...) {
        return false;
    }
}

std::tuple<FileHeader, std::vector<uint8_t>, std::vector<uint8_t>> FileFormatHandler::read_file(
    const std::string& path
) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }
    
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read entire file
    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    
    // Deserialize header
    auto [header, header_size] = FileHeader::deserialize(file_data);
    
    // Check if algorithm uses authentication tag (AEAD)
    bool has_tag = (header.algorithm == AlgorithmID::AES_128_GCM ||
                    header.algorithm == AlgorithmID::AES_192_GCM ||
                    header.algorithm == AlgorithmID::AES_256_GCM ||
                    header.algorithm == AlgorithmID::CHACHA20_POLY1305 ||
                    header.algorithm == AlgorithmID::SERPENT_256_GCM ||
                    header.algorithm == AlgorithmID::TWOFISH_128_GCM ||
                    header.algorithm == AlgorithmID::TWOFISH_192_GCM ||
                    header.algorithm == AlgorithmID::TWOFISH_256_GCM ||
                    header.algorithm == AlgorithmID::CAMELLIA_128_GCM ||
                    header.algorithm == AlgorithmID::CAMELLIA_192_GCM ||
                    header.algorithm == AlgorithmID::CAMELLIA_256_GCM ||
                    header.algorithm == AlgorithmID::ARIA_128_GCM ||
                    header.algorithm == AlgorithmID::ARIA_192_GCM ||
                    header.algorithm == AlgorithmID::ARIA_256_GCM ||
                    header.algorithm == AlgorithmID::SM4_GCM);
    
    size_t tag_size = has_tag ? 16 : 0;
    
    // Remaining data is ciphertext + tag (if AEAD)
    if (file_size < header_size + tag_size) {
        throw std::runtime_error("File too small for header and ciphertext");
    }
    
    size_t ciphertext_size = file_size - header_size - tag_size;
    
    std::vector<uint8_t> ciphertext(ciphertext_size);
    std::memcpy(ciphertext.data(), file_data.data() + header_size, ciphertext_size);
    
    std::vector<uint8_t> auth_tag;
    if (has_tag) {
        auth_tag.resize(16);
        std::memcpy(auth_tag.data(), file_data.data() + header_size + ciphertext_size, 16);
    }
    
    return {header, ciphertext, auth_tag};
}

AlgorithmID FileFormatHandler::to_algorithm_id(AlgorithmType type) {
    switch (type) {
        case AlgorithmType::AES_128_GCM: return AlgorithmID::AES_128_GCM;
        case AlgorithmType::AES_192_GCM: return AlgorithmID::AES_192_GCM;
        case AlgorithmType::AES_256_GCM: return AlgorithmID::AES_256_GCM;
        case AlgorithmType::CHACHA20_POLY1305: return AlgorithmID::CHACHA20_POLY1305;
        case AlgorithmType::SERPENT_256_GCM: return AlgorithmID::SERPENT_256_GCM;
        case AlgorithmType::TWOFISH_128_GCM: return AlgorithmID::TWOFISH_128_GCM;
        case AlgorithmType::TWOFISH_192_GCM: return AlgorithmID::TWOFISH_192_GCM;
        case AlgorithmType::TWOFISH_256_GCM: return AlgorithmID::TWOFISH_256_GCM;
        case AlgorithmType::CAMELLIA_128_GCM: return AlgorithmID::CAMELLIA_128_GCM;
        case AlgorithmType::CAMELLIA_192_GCM: return AlgorithmID::CAMELLIA_192_GCM;
        case AlgorithmType::CAMELLIA_256_GCM: return AlgorithmID::CAMELLIA_256_GCM;
        case AlgorithmType::ARIA_128_GCM: return AlgorithmID::ARIA_128_GCM;
        case AlgorithmType::ARIA_192_GCM: return AlgorithmID::ARIA_192_GCM;
        case AlgorithmType::ARIA_256_GCM: return AlgorithmID::ARIA_256_GCM;
        case AlgorithmType::SM4_GCM: return AlgorithmID::SM4_GCM;
        case AlgorithmType::CAESAR: return AlgorithmID::CAESAR;
        case AlgorithmType::VIGENERE: return AlgorithmID::VIGENERE;
        case AlgorithmType::PLAYFAIR: return AlgorithmID::PLAYFAIR;
        case AlgorithmType::SUBSTITUTION: return AlgorithmID::SUBSTITUTION;
        case AlgorithmType::HILL: return AlgorithmID::HILL;
        default: return AlgorithmID::UNKNOWN;
    }
}

AlgorithmType FileFormatHandler::from_algorithm_id(AlgorithmID id) {
    switch (id) {
        case AlgorithmID::AES_128_GCM: return AlgorithmType::AES_128_GCM;
        case AlgorithmID::AES_192_GCM: return AlgorithmType::AES_192_GCM;
        case AlgorithmID::AES_256_GCM: return AlgorithmType::AES_256_GCM;
        case AlgorithmID::CHACHA20_POLY1305: return AlgorithmType::CHACHA20_POLY1305;
        case AlgorithmID::SERPENT_256_GCM: return AlgorithmType::SERPENT_256_GCM;
        case AlgorithmID::TWOFISH_128_GCM: return AlgorithmType::TWOFISH_128_GCM;
        case AlgorithmID::TWOFISH_192_GCM: return AlgorithmType::TWOFISH_192_GCM;
        case AlgorithmID::TWOFISH_256_GCM: return AlgorithmType::TWOFISH_256_GCM;
        case AlgorithmID::CAMELLIA_128_GCM: return AlgorithmType::CAMELLIA_128_GCM;
        case AlgorithmID::CAMELLIA_192_GCM: return AlgorithmType::CAMELLIA_192_GCM;
        case AlgorithmID::CAMELLIA_256_GCM: return AlgorithmType::CAMELLIA_256_GCM;
        case AlgorithmID::ARIA_128_GCM: return AlgorithmType::ARIA_128_GCM;
        case AlgorithmID::ARIA_192_GCM: return AlgorithmType::ARIA_192_GCM;
        case AlgorithmID::ARIA_256_GCM: return AlgorithmType::ARIA_256_GCM;
        case AlgorithmID::SM4_GCM: return AlgorithmType::SM4_GCM;
        case AlgorithmID::CAESAR: return AlgorithmType::CAESAR;
        case AlgorithmID::VIGENERE: return AlgorithmType::VIGENERE;
        case AlgorithmID::PLAYFAIR: return AlgorithmType::PLAYFAIR;
        case AlgorithmID::SUBSTITUTION: return AlgorithmType::SUBSTITUTION;
        case AlgorithmID::HILL: return AlgorithmType::HILL;
        default: return AlgorithmType::AES_256_GCM;
    }
}

KDFID FileFormatHandler::to_kdf_id(KDFType type) {
    switch (type) {
        case KDFType::ARGON2ID: return KDFID::ARGON2ID;
        case KDFType::ARGON2I: return KDFID::ARGON2I;
        case KDFType::PBKDF2_SHA256: return KDFID::PBKDF2_SHA256;
        case KDFType::PBKDF2_SHA512: return KDFID::PBKDF2_SHA512;
        case KDFType::SCRYPT: return KDFID::SCRYPT;
        default: return KDFID::NONE;
    }
}

KDFType FileFormatHandler::from_kdf_id(KDFID id) {
    switch (id) {
        case KDFID::ARGON2ID: return KDFType::ARGON2ID;
        case KDFID::ARGON2I: return KDFType::ARGON2I;
        case KDFID::PBKDF2_SHA256: return KDFType::PBKDF2_SHA256;
        case KDFID::PBKDF2_SHA512: return KDFType::PBKDF2_SHA512;
        case KDFID::SCRYPT: return KDFType::SCRYPT;
        default: return KDFType::ARGON2ID;
    }
}

CompressionID FileFormatHandler::to_compression_id(const std::string& type) {
    if (type == "zlib") return CompressionID::ZLIB;
    if (type == "bzip2") return CompressionID::BZIP2;
    if (type == "lzma") return CompressionID::LZMA;
    return CompressionID::NONE;
}

std::string FileFormatHandler::from_compression_id(CompressionID id) {
    switch (id) {
        case CompressionID::ZLIB: return "zlib";
        case CompressionID::BZIP2: return "bzip2";
        case CompressionID::LZMA: return "lzma";
        default: return "none";
    }
}

bool FileFormatHandler::is_legacy_format(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    uint8_t magic[8];
    file.read(reinterpret_cast<char*>(magic), 8);
    
    return std::memcmp(magic, FILE_FORMAT_MAGIC, 8) != 0;
}

std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>> FileFormatHandler::read_legacy_file(
    const std::string& path
) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }
    
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Legacy format: [salt:32][nonce:12][ciphertext+tag]
    if (file_size < 44) {
        throw std::runtime_error("File too small for legacy format");
    }
    
    std::vector<uint8_t> salt(32);
    file.read(reinterpret_cast<char*>(salt.data()), 32);
    
    std::vector<uint8_t> nonce(12);
    file.read(reinterpret_cast<char*>(nonce.data()), 12);
    
    size_t remaining = file_size - 44;
    std::vector<uint8_t> ciphertext_with_tag(remaining);
    file.read(reinterpret_cast<char*>(ciphertext_with_tag.data()), remaining);
    
    return {salt, nonce, ciphertext_with_tag};
}

} // namespace core
} // namespace filevault
