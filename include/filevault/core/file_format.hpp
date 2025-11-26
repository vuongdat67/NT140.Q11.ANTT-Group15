#ifndef FILEVAULT_CORE_FILE_FORMAT_HPP
#define FILEVAULT_CORE_FILE_FORMAT_HPP

#include "filevault/core/types.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <span>

namespace filevault {
namespace core {

/**
 * @brief FileVault file format version 1.0
 * 
 * Format structure:
 * [Magic:8][Version:2][AlgoID:1][KDFID:1][CompID:1][Reserved:3]
 * [Salt:32][KDF_Params:Variable][NonceSize:1][Nonce:Variable][Compressed_Flag:1]
 * [Ciphertext][Auth_Tag:16 (AEAD only)]
 * 
 * Magic: "FVAULT01" (8 bytes)
 * Version: Major.Minor (1 byte each)
 * AlgoID: Algorithm identifier (1 byte)
 * KDFID: KDF identifier (1 byte)
 * CompID: Compression identifier (1 byte)
 * Reserved: Future use (3 bytes)
 * Salt: Random salt for KDF (32 bytes)
 * KDF_Params: Variable-length KDF parameters
 * NonceSize: Size of nonce/IV in bytes (1 byte)
 * Nonce: Random nonce/IV (variable, e.g. 12 for GCM, 16 for CBC/CTR)
 * Compressed_Flag: 0x00=No, 0x01=Yes (1 byte)
 * Ciphertext: Encrypted data
 * Auth_Tag: GCM authentication tag (16 bytes, only for AEAD)
 */

constexpr uint8_t FILE_FORMAT_MAGIC[8] = {'F', 'V', 'A', 'U', 'L', 'T', '0', '1'};
constexpr uint8_t FILE_FORMAT_VERSION_MAJOR = 1;
constexpr uint8_t FILE_FORMAT_VERSION_MINOR = 0;

/**
 * @brief Algorithm identifiers
 */
enum class AlgorithmID : uint8_t {
    UNKNOWN = 0x00,
    AES_128_GCM = 0x01,
    AES_192_GCM = 0x02,
    AES_256_GCM = 0x03,
    CHACHA20_POLY1305 = 0x04,
    SERPENT_256_GCM = 0x05,
    // Twofish family
    TWOFISH_128_GCM = 0x06,
    TWOFISH_192_GCM = 0x07,
    TWOFISH_256_GCM = 0x08,
    // International standards
    CAMELLIA_128_GCM = 0x09,
    CAMELLIA_192_GCM = 0x0A,
    CAMELLIA_256_GCM = 0x0B,
    ARIA_128_GCM = 0x0C,
    ARIA_192_GCM = 0x0D,
    ARIA_256_GCM = 0x0E,
    SM4_GCM = 0x0F,
    // Classical ciphers
    CAESAR = 0x10,
    VIGENERE = 0x11,
    PLAYFAIR = 0x12,
    SUBSTITUTION = 0x13,
    HILL = 0x14,
    // Non-AEAD modes (CBC, CTR)
    AES_128_CBC = 0x20,
    AES_192_CBC = 0x21,
    AES_256_CBC = 0x22,
    AES_128_CTR = 0x23,
    AES_192_CTR = 0x24,
    AES_256_CTR = 0x25,
    // CFB mode
    AES_128_CFB = 0x26,
    AES_192_CFB = 0x27,
    AES_256_CFB = 0x28,
    // OFB mode
    AES_128_OFB = 0x29,
    AES_192_OFB = 0x2A,
    AES_256_OFB = 0x2B,
    // ECB mode (insecure)
    AES_128_ECB = 0x2C,
    AES_192_ECB = 0x2D,
    AES_256_ECB = 0x2E,
    // XTS mode (disk encryption)
    AES_128_XTS = 0x2F,
    AES_256_XTS = 0x30,
    // Legacy algorithms
    TRIPLE_DES_CBC = 0x40,
    // Asymmetric (RSA)
    RSA_2048 = 0x50,
    RSA_3072 = 0x51,
    RSA_4096 = 0x52,
    // Asymmetric (ECC)
    ECC_P256 = 0x60,
    ECC_P384 = 0x61,
    ECC_P521 = 0x62
};

/**
 * @brief KDF identifiers
 */
enum class KDFID : uint8_t {
    NONE = 0x00,
    ARGON2ID = 0x01,
    ARGON2I = 0x02,
    PBKDF2_SHA256 = 0x03,
    PBKDF2_SHA512 = 0x04,
    SCRYPT = 0x05
};

/**
 * @brief Compression identifiers
 */
enum class CompressionID : uint8_t {
    NONE = 0x00,
    ZLIB = 0x01,
    BZIP2 = 0x02,
    LZMA = 0x03
};

/**
 * @brief KDF parameters for Argon2
 */
struct Argon2Params {
    uint32_t memory_kb = 65536;     // 64 MB default
    uint32_t iterations = 3;
    uint32_t parallelism = 4;
    
    std::vector<uint8_t> serialize() const;
    static Argon2Params deserialize(std::span<const uint8_t> data);
};

/**
 * @brief KDF parameters for PBKDF2
 */
struct PBKDF2Params {
    uint32_t iterations = 100000;
    
    std::vector<uint8_t> serialize() const;
    static PBKDF2Params deserialize(std::span<const uint8_t> data);
};

/**
 * @brief KDF parameters for Scrypt
 */
struct ScryptParams {
    uint32_t n = 32768;             // CPU/memory cost
    uint32_t r = 8;                 // Block size
    uint32_t p = 1;                 // Parallelization
    
    std::vector<uint8_t> serialize() const;
    static ScryptParams deserialize(std::span<const uint8_t> data);
};

/**
 * @brief File format header
 */
struct FileHeader {
    uint8_t magic[8];
    uint8_t version_major;
    uint8_t version_minor;
    AlgorithmID algorithm;
    KDFID kdf;
    CompressionID compression;
    uint8_t reserved[3];
    std::vector<uint8_t> salt;
    std::vector<uint8_t> kdf_params;
    std::vector<uint8_t> nonce;
    bool compressed;
    
    /**
     * @brief Check if magic bytes are valid
     */
    bool is_valid() const;
    
    /**
     * @brief Get total header size in bytes
     */
    size_t size() const;
    
    /**
     * @brief Serialize header to bytes
     */
    std::vector<uint8_t> serialize() const;
    
    /**
     * @brief Deserialize header from bytes
     * @return Header and number of bytes consumed
     */
    static std::pair<FileHeader, size_t> deserialize(std::span<const uint8_t> data);
};

/**
 * @brief File format handler
 */
class FileFormatHandler {
public:
    /**
     * @brief Create header from encryption config
     */
    static FileHeader create_header(
        AlgorithmType algo_type,
        KDFType kdf_type,
        const EncryptionConfig& config,
        std::span<const uint8_t> salt,
        std::span<const uint8_t> nonce,
        bool compressed
    );
    
    /**
     * @brief Write encrypted file with header
     */
    static bool write_file(
        const std::string& path,
        const FileHeader& header,
        std::span<const uint8_t> ciphertext,
        std::span<const uint8_t> auth_tag
    );
    
    /**
     * @brief Read encrypted file and parse header
     */
    static std::tuple<FileHeader, std::vector<uint8_t>, std::vector<uint8_t>> read_file(
        const std::string& path
    );
    
    /**
     * @brief Convert AlgorithmType to AlgorithmID
     */
    static AlgorithmID to_algorithm_id(AlgorithmType type);
    
    /**
     * @brief Convert AlgorithmID to AlgorithmType
     */
    static AlgorithmType from_algorithm_id(AlgorithmID id);
    
    /**
     * @brief Convert KDFType to KDFID
     */
    static KDFID to_kdf_id(KDFType type);
    
    /**
     * @brief Convert KDFID to KDFType
     */
    static KDFType from_kdf_id(KDFID id);
    
    /**
     * @brief Convert compression type to CompressionID
     */
    static CompressionID to_compression_id(const std::string& type);
    
    /**
     * @brief Convert CompressionID to string
     */
    static std::string from_compression_id(CompressionID id);
    
    /**
     * @brief Detect if file is legacy format (no magic bytes)
     */
    static bool is_legacy_format(const std::string& path);
    
    /**
     * @brief Read legacy format file (old format without header)
     */
    static std::tuple<std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>> read_legacy_file(
        const std::string& path
    );
};

} // namespace core
} // namespace filevault

#endif
