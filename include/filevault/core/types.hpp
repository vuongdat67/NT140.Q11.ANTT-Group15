#ifndef FILEVAULT_CORE_TYPES_HPP
#define FILEVAULT_CORE_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace filevault {
namespace core {

/**
 * @brief Enumeration of encryption algorithm types
 */
enum class AlgorithmType {
    // Symmetric ciphers (modern AEAD)
    AES_128_GCM,
    AES_192_GCM,
    AES_256_GCM,
    CHACHA20_POLY1305,
    SERPENT_256_GCM,
    TWOFISH_128_GCM,
    TWOFISH_192_GCM,
    TWOFISH_256_GCM,
    
    // Symmetric ciphers (CBC mode - legacy)
    AES_128_CBC,
    AES_192_CBC,
    AES_256_CBC,
    
    // Classic (educational)
    CAESAR,
    VIGENERE,
    PLAYFAIR,
    SUBSTITUTION,
    HILL
};

/**
 * @brief Enumeration of hash algorithm types
 */
enum class HashType {
    // Legacy (insecure - for compatibility only)
    MD5,
    SHA1,
    
    // SHA-2 family (secure)
    SHA224,
    SHA256,
    SHA384,
    SHA512,
    SHA512_256,  // SHA-512/256
    
    // SHA-3 family (secure)
    SHA3_224,
    SHA3_256,
    SHA3_384,
    SHA3_512,
    
    // BLAKE2 family (secure, fast)
    BLAKE2B_256,
    BLAKE2B_384,
    BLAKE2B_512,
    BLAKE2S_256
};

/**
 * @brief Compression algorithm types
 */
enum class CompressionType {
    NONE,
    ZLIB,
    BZIP2,
    LZMA
};

/**
 * @brief User mode/profile for algorithm selection
 */
enum class UserMode {
    STUDENT,        // Educational - classical ciphers
    PROFESSIONAL,   // Standard - AES-256, Argon2
    ADVANCED        // Maximum security - custom params
};

/**
 * @brief Enumeration of Key Derivation Function types
 */
enum class KDFType {
    ARGON2ID,
    ARGON2I,
    PBKDF2_SHA256,
    PBKDF2_SHA512,
    SCRYPT
};

/**
 * @brief Security level determining algorithm parameters
 */
enum class SecurityLevel {
    WEAK,       // Fast, for testing (KDF: 10k iterations, 16MB memory)
    MEDIUM,     // Balanced (KDF: 100k iterations, 64MB memory)
    STRONG,     // High security (KDF: 200k iterations, 128MB memory)
    PARANOID    // Maximum (KDF: 500k iterations, 256MB memory)
};

/**
 * @brief Configuration for encryption operations
 */
struct EncryptionConfig {
    AlgorithmType algorithm = AlgorithmType::AES_256_GCM;
    KDFType kdf = KDFType::ARGON2ID;
    SecurityLevel level = SecurityLevel::MEDIUM;
    UserMode mode = UserMode::PROFESSIONAL;
    
    // KDF parameters (auto-set based on SecurityLevel)
    uint32_t kdf_iterations = 100000;
    uint32_t kdf_memory_kb = 65536;  // 64MB default
    uint32_t kdf_parallelism = 4;
    
    // Encryption parameters (generated automatically or provided)
    std::vector<uint8_t> salt;
    std::optional<std::vector<uint8_t>> nonce;
    std::optional<std::vector<uint8_t>> tag;
    std::optional<std::vector<uint8_t>> associated_data;
    
    // Compression
    CompressionType compression = CompressionType::NONE;
    int compression_level = 6;  // 1-9 for zlib/bzip2/lzma
    
    // Metadata
    bool include_metadata = true;
    std::string comment;
    
    // Progress reporting
    bool show_progress = true;
    bool verbose = false;
    
    /**
     * @brief Apply security level parameters
     */
    void apply_security_level();
    
    /**
     * @brief Apply user mode defaults
     */
    void apply_user_mode();
};

/**
 * @brief Configuration for hashing operations
 */
struct HashConfig {
    HashType algorithm = HashType::SHA256;
    bool hmac_mode = false;
    std::vector<uint8_t> hmac_key;
    
    // For file hashing
    bool verify_mode = false;
    std::string expected_hash;
    
    // Output format
    bool uppercase = false;
    bool include_filename = true;
};

/**
 * @brief Password strength levels
 */
enum class PasswordStrength {
    VERY_WEAK,
    WEAK,
    FAIR,
    STRONG,
    VERY_STRONG
};

/**
 * @brief Password strength analysis result
 */
struct PasswordAnalysis {
    PasswordStrength strength;
    int score;  // 0-100
    std::vector<std::string> warnings;
    std::vector<std::string> suggestions;
    
    // Detailed metrics
    size_t length;
    bool has_lowercase;
    bool has_uppercase;
    bool has_digits;
    bool has_special;
    bool has_repeated_chars;
    bool is_common_password;
    
    // Estimated crack time
    std::string crack_time_online;   // "< 1 second" or "centuries"
    std::string crack_time_offline;  // "< 1 second" or "centuries"
};

} // namespace core
} // namespace filevault

#endif // FILEVAULT_CORE_TYPES_HPP
