#pragma once

#include "filevault/core/crypto_algorithm.hpp"
#include <string>
#include <vector>

namespace filevault {
namespace algorithms {
namespace classical {

/**
 * @brief Vigenère Cipher - Polyalphabetic substitution
 * 
 * Uses a keyword to create multiple Caesar shifts.
 * Considered "le chiffre indéchiffrable" (unbreakable cipher) until 1863.
 * 
 * Security: BROKEN - Vulnerable to Kasiski examination
 * Purpose: Educational - shows improvement over monoalphabetic ciphers
 * 
 * @see https://en.wikipedia.org/wiki/Vigen%C3%A8re_cipher
 */
class Vigenere : public core::ICryptoAlgorithm {
public:
    explicit Vigenere(const std::string& keyword = "KEY");
    
    std::string name() const override { return "Vigenere"; }
    core::AlgorithmType type() const override { return core::AlgorithmType::VIGENERE; }
    size_t key_size() const override { return 32; } // Standard key size
    
    core::CryptoResult encrypt(
        std::span<const uint8_t> plaintext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config) override;
    
    core::CryptoResult decrypt(
        std::span<const uint8_t> ciphertext,
        std::span<const uint8_t> key,
        const core::EncryptionConfig& config) override;
    
    bool is_suitable_for(core::SecurityLevel level) const override {
        (void)level;
        return false;
    }
    
private:
    std::string keyword_;
};

} // namespace classical
} // namespace algorithms
} // namespace filevault
