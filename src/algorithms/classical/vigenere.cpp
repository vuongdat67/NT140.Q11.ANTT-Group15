#include "filevault/algorithms/classical/vigenere.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>

namespace filevault {
namespace algorithms {
namespace classical {

Vigenere::Vigenere(const std::string& keyword) : keyword_(keyword) {
    std::transform(keyword_.begin(), keyword_.end(), keyword_.begin(), ::toupper);
}

core::CryptoResult Vigenere::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config)
{
    (void)config;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string keyword = keyword_;
    if (!key.empty()) {
        keyword = std::string(key.begin(), key.end());
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);
    }
    
    std::vector<uint8_t> result;
    result.reserve(plaintext.size());
    
    size_t key_index = 0;
    for (uint8_t byte : plaintext) {
        char ch = static_cast<char>(byte);
        
        if (std::isalpha(ch)) {
            char base = std::isupper(ch) ? 'A' : 'a';
            int shift = keyword[key_index % keyword.size()] - 'A';
            ch = base + (ch - base + shift) % 26;
            key_index++;
        }
        
        result.push_back(static_cast<uint8_t>(ch));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    core::CryptoResult crypto_result;
    crypto_result.success = true;
    crypto_result.data = std::move(result);
    crypto_result.algorithm_used = core::AlgorithmType::VIGENERE;
    crypto_result.original_size = plaintext.size();
    crypto_result.final_size = result.size();
    crypto_result.processing_time_ms = duration.count() / 1000.0;
    return crypto_result;
}

core::CryptoResult Vigenere::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config)
{
    (void)config;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string keyword = keyword_;
    if (!key.empty()) {
        keyword = std::string(key.begin(), key.end());
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);
    }
    
    std::vector<uint8_t> result;
    result.reserve(ciphertext.size());
    
    size_t key_index = 0;
    for (uint8_t byte : ciphertext) {
        char ch = static_cast<char>(byte);
        
        if (std::isalpha(ch)) {
            char base = std::isupper(ch) ? 'A' : 'a';
            int shift = keyword[key_index % keyword.size()] - 'A';
            ch = base + (ch - base - shift + 26) % 26;
            key_index++;
        }
        
        result.push_back(static_cast<uint8_t>(ch));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    core::CryptoResult crypto_result;
    crypto_result.success = true;
    crypto_result.data = std::move(result);
    crypto_result.algorithm_used = core::AlgorithmType::VIGENERE;
    crypto_result.original_size = ciphertext.size();
    crypto_result.final_size = result.size();
    crypto_result.processing_time_ms = duration.count() / 1000.0;
    return crypto_result;
}

} // namespace classical
} // namespace algorithms
} // namespace filevault
