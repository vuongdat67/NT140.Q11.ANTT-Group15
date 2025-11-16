#pragma once

#include "filevault/core/crypto_algorithm.hpp"
#include <string>

namespace filevault {
namespace algorithms {
namespace classical {

/**
 * @brief Caesar Cipher - Educational only
 * 
 * Simple substitution cipher that shifts characters by a fixed offset.
 * Used by Julius Caesar for military messages (~50 BC).
 * 
 * Security: COMPLETELY BROKEN - Only 26 possible keys!
 * Purpose: Educational demonstration of cryptanalysis
 * 
 * @see https://en.wikipedia.org/wiki/Caesar_cipher
 */
class Caesar : public core::ICryptoAlgorithm {
public:
    explicit Caesar(int shift = 3);
    
    std::string name() const override { return "Caesar"; }
    core::AlgorithmType type() const override { return core::AlgorithmType::CAESAR; }
    size_t key_size() const override { return 4; } // Min for Argon2, use first byte as shift
    
    core::CryptoResult encrypt(
        std::span<const uint8_t> plaintext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config) override;
    
    core::CryptoResult decrypt(
        std::span<const uint8_t> ciphertext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config) override;
    
    bool is_suitable_for(core::SecurityLevel level) const override {
        (void)level; // Educational only - not for real security
        return false;
    }
    
    /**
     * @brief Brute force attack demonstration
     * @param ciphertext Encrypted text
     * @return All 26 possible decryptions
     */
    static std::string brute_force(const std::string& ciphertext);
    
private:
    int shift_;
    
    char shift_char(char ch, int shift) const;
};

} // namespace classical
} // namespace algorithms
} // namespace filevault
