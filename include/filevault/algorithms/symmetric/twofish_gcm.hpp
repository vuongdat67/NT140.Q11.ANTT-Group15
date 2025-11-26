/**
 * @file twofish_gcm.hpp
 * @brief Twofish-256-GCM encryption algorithm
 *
 * Twofish is a 128-bit block cipher designed by Bruce Schneier et al.
 * It was an AES finalist and is known for its flexibility and security.
 *
 * @author FileVault Team
 * @date 2024
 */

#ifndef FILEVAULT_ALGORITHMS_SYMMETRIC_TWOFISH_GCM_HPP
#define FILEVAULT_ALGORITHMS_SYMMETRIC_TWOFISH_GCM_HPP

#include "filevault/core/types.hpp"
#include "filevault/core/crypto_algorithm.hpp"
#include <vector>
#include <span>

namespace filevault {
namespace algorithms {
namespace symmetric {

/**
 * @brief Twofish-256-GCM implementation
 * 
 * Twofish is a 128-bit block cipher designed by Bruce Schneier, John Kelsey,
 * Doug Whiting, David Wagner, Chris Hall, and Niels Ferguson. It was an AES
 * finalist and is known for its conservative design.
 * 
 * References:
 * - Twofish: A 128-Bit Block Cipher
 *   https://www.schneier.com/academic/twofish/
 * 
 * - NIST SP 800-38D: GCM Mode
 *   https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf
 * 
 * Security:
 * - Key size: 256 bits (32 bytes)
 * - Block size: 128 bits (16 bytes)
 * - Nonce size: 96 bits (12 bytes) for GCM
 * - Tag size: 128 bits (16 bytes) for authentication
 * 
 * Features:
 * - Pre-computed S-boxes for performance
 * - Key-dependent S-boxes for security
 * - Supports 128, 192, and 256-bit keys
 * - Patent-free and royalty-free
 * 
 * @note This implementation uses Botan's Twofish/GCM mode
 * @note Twofish-256 provides equivalent security to AES-256
 */
class Twofish_GCM : public core::ICryptoAlgorithm {
public:
    /**
     * @brief Construct Twofish-256-GCM cipher
     * @param key_bits Key size in bits (128, 192, or 256, default 256)
     */
    explicit Twofish_GCM(size_t key_bits = 256);
    
    /**
     * @brief Get algorithm name
     */
    std::string name() const override;
    
    /**
     * @brief Get algorithm type
     */
    core::AlgorithmType type() const override;
    
    /**
     * @brief Encrypt plaintext with Twofish-GCM
     * 
     * @param plaintext Input data to encrypt
     * @param key Encryption key (size depends on key_bits)
     * @param config Encryption configuration with nonce
     * @return Encrypted data with authentication tag
     */
    core::CryptoResult encrypt(
        std::span<const uint8_t> plaintext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    /**
     * @brief Decrypt ciphertext with Twofish-GCM
     * 
     * @param ciphertext Encrypted data
     * @param key Decryption key (size depends on key_bits)
     * @param config Decryption configuration with nonce and tag
     * @return Decrypted plaintext
     */
    core::CryptoResult decrypt(
        std::span<const uint8_t> ciphertext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    /**
     * @brief Get required key size in bytes
     */
    size_t key_size() const override;
    
    /**
     * @brief Get nonce size in bytes (12 for GCM)
     */
    size_t nonce_size() const { return 12; }
    
    /**
     * @brief Get tag size in bytes (16 for GCM)
     */
    size_t tag_size() const { return 16; }
    
    /**
     * @brief Check if algorithm is suitable for security level
     */
    bool is_suitable_for(core::SecurityLevel level) const override;

private:
    size_t key_bits_;
    core::AlgorithmType type_;
    std::string botan_name_;
    
    /**
     * @brief Perform encryption/decryption operation
     */
    std::vector<uint8_t> process_gcm(
        std::span<const uint8_t> input,
        std::span<const uint8_t> key,
        std::span<const uint8_t> nonce,
        std::span<const uint8_t> tag,
        bool encrypt
    );
};

} // namespace symmetric
} // namespace algorithms
} // namespace filevault

#endif // FILEVAULT_ALGORITHMS_SYMMETRIC_TWOFISH_GCM_HPP
