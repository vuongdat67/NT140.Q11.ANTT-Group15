#pragma once

#include "filevault/core/crypto_algorithm.hpp"
#include <string>
#include <utility>

namespace filevault {
namespace algorithms {
namespace classical {

/**
 * @brief Playfair Cipher - Digraph substitution
 * 
 * Uses a 5x5 matrix based on a keyword.
 * First practical digraph substitution cipher.
 * 
 * Security: BROKEN - 600 possible digraphs still analyzable
 * Purpose: Educational - shows digraph cryptanalysis
 * 
 * @see https://en.wikipedia.org/wiki/Playfair_cipher
 */
class Playfair : public core::ICryptoAlgorithm {
public:
    explicit Playfair(const std::string& keyword = "KEYWORD");
    
    std::string name() const override { return "Playfair"; }
    core::AlgorithmType type() const override { return core::AlgorithmType::PLAYFAIR; }
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
    char matrix_[5][5];
    
    void build_matrix(const std::string& keyword);
    std::pair<int, int> find_position(char ch) const;
};

} // namespace classical
} // namespace algorithms
} // namespace filevault
