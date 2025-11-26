/**
 * @file aria_gcm.hpp
 * @brief ARIA-GCM AEAD encryption algorithm
 *
 * ARIA is a Korean block cipher standard, designed by the Korean
 * Agency for Technology and Standards (KATS). It was adopted as a
 * Korean government standard (KS X 1213) and is ISO/IEC 18033-3 certified.
 */

#ifndef FILEVAULT_ALGORITHMS_SYMMETRIC_ARIA_GCM_HPP
#define FILEVAULT_ALGORITHMS_SYMMETRIC_ARIA_GCM_HPP

#include "filevault/core/crypto_algorithm.hpp"
#include <botan/aead.h>
#include <memory>

namespace filevault {
namespace algorithms {
namespace symmetric {

/**
 * @brief ARIA-GCM AEAD encryption
 * 
 * Supports 128, 192, and 256-bit keys.
 * Uses GCM mode for authenticated encryption.
 */
class ARIA_GCM : public core::ICryptoAlgorithm {
public:
    /**
     * @brief Construct ARIA-GCM with specified key size
     * @param key_bits Key size in bits (128, 192, or 256)
     */
    explicit ARIA_GCM(size_t key_bits = 256);
    virtual ~ARIA_GCM() = default;
    
    std::string name() const override;
    core::AlgorithmType type() const override;
    
    core::CryptoResult encrypt(
        std::span<const uint8_t> plaintext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    core::CryptoResult decrypt(
        std::span<const uint8_t> ciphertext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config
    ) override;
    
    size_t key_size() const override { return key_bits_ / 8; }
    size_t nonce_size() const { return 12; }  // GCM standard
    size_t tag_size() const { return 16; }    // 128-bit tag
    
    bool is_suitable_for(core::SecurityLevel level) const override;

private:
    size_t key_bits_;
    core::AlgorithmType type_;
    std::string botan_name_;
};

} // namespace symmetric
} // namespace algorithms
} // namespace filevault

#endif // FILEVAULT_ALGORITHMS_SYMMETRIC_ARIA_GCM_HPP
