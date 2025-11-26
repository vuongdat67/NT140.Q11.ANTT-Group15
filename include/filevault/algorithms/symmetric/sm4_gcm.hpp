/**
 * @file sm4_gcm.hpp
 * @brief SM4-GCM AEAD encryption algorithm
 *
 * SM4 (formerly SMS4) is a Chinese national standard block cipher
 * used in wireless LAN standard WAPI. It was made a national standard
 * (GB/T 32907-2016) in 2016 and is widely used in China.
 * SM4 uses 128-bit key and 128-bit block size.
 */

#ifndef FILEVAULT_ALGORITHMS_SYMMETRIC_SM4_GCM_HPP
#define FILEVAULT_ALGORITHMS_SYMMETRIC_SM4_GCM_HPP

#include "filevault/core/crypto_algorithm.hpp"
#include <botan/aead.h>
#include <memory>

namespace filevault {
namespace algorithms {
namespace symmetric {

/**
 * @brief SM4-GCM AEAD encryption
 * 
 * SM4 only supports 128-bit keys.
 * Uses GCM mode for authenticated encryption.
 */
class SM4_GCM : public core::ICryptoAlgorithm {
public:
    SM4_GCM();
    virtual ~SM4_GCM() = default;
    
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
    
    size_t key_size() const override { return 16; }  // 128 bits only
    size_t nonce_size() const { return 12; }  // GCM standard
    size_t tag_size() const { return 16; }    // 128-bit tag
    
    bool is_suitable_for(core::SecurityLevel level) const override;
};

} // namespace symmetric
} // namespace algorithms
} // namespace filevault

#endif // FILEVAULT_ALGORITHMS_SYMMETRIC_SM4_GCM_HPP
