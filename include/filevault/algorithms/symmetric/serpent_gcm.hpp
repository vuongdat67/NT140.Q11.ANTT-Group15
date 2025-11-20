#ifndef FILEVAULT_ALGORITHMS_SYMMETRIC_SERPENT_GCM_HPP
#define FILEVAULT_ALGORITHMS_SYMMETRIC_SERPENT_GCM_HPP

#include "filevault/core/types.hpp"
#include "filevault/core/crypto_algorithm.hpp"
#include <vector>
#include <span>

namespace filevault {
namespace algorithms {
namespace symmetric {

/**
 * @brief Serpent-256-GCM implementation
 * 
 * Serpent is a 128-bit block cipher designed by Ross Anderson, Eli Biham, 
 * and Lars Knudsen. It was an AES finalist and is known for its high security margin.
 * 
 * References:
 * - Serpent: A Candidate Block Cipher for the AES
 *   https://www.cl.cam.ac.uk/~rja14/serpent.html
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
 * @note This implementation uses Botan's Serpent/GCM mode
 * @note Serpent-256 provides very high security margin
 * @note Slower than AES on most platforms but more secure
 */
class Serpent_GCM : public core::ICryptoAlgorithm {
public:
    /**
     * @brief Construct Serpent-256-GCM cipher
     */
    Serpent_GCM();
    
    /**
     * @brief Get algorithm name
     */
    std::string name() const override;
    
    /**
     * @brief Get algorithm type
     */
    core::AlgorithmType type() const override;
    
    /**
     * @brief Encrypt plaintext with Serpent-256-GCM
     * 
     * @param plaintext Input data to encrypt
     * @param key 256-bit encryption key
     * @param config Encryption configuration with nonce
     * @return Encrypted data with authentication tag
     */
    core::CryptoResult encrypt(
        std::span<const uint8_t> plaintext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    /**
     * @brief Decrypt ciphertext with Serpent-256-GCM
     * 
     * @param ciphertext Encrypted data
     * @param key 256-bit decryption key
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
     * @brief Check if algorithm is suitable for security level
     */
    bool is_suitable_for(core::SecurityLevel level) const override;

private:
    /**
     * @brief Perform encryption/decryption operation
     * 
     * @param input Input data
     * @param key Encryption/decryption key
     * @param nonce Unique nonce (96 bits)
     * @param tag Authentication tag (for decryption, empty for encryption)
     * @param encrypt True for encryption, false for decryption
     * @return Encrypted/decrypted data (with tag appended for encryption)
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

#endif // FILEVAULT_ALGORITHMS_SYMMETRIC_SERPENT_GCM_HPP
