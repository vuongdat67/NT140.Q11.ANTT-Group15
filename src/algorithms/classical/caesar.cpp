#include "filevault/algorithms/classical/caesar.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace filevault {
namespace algorithms {
namespace classical {

Caesar::Caesar(int shift) : shift_(shift % 26) {}

char Caesar::shift_char(char ch, int shift) const {
    if (std::isalpha(ch)) {
        char base = std::isupper(ch) ? 'A' : 'a';
        return base + (ch - base + shift + 26) % 26;
    }
    return ch;
}

core::CryptoResult Caesar::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config)
{
    (void)config;
    auto start = std::chrono::high_resolution_clock::now();
    
    int shift = shift_;
    if (!key.empty()) {
        shift = static_cast<int>(key[0]) % 26;
    }
    
    std::vector<uint8_t> result;
    result.reserve(plaintext.size());
    
    for (uint8_t byte : plaintext) {
        result.push_back(static_cast<uint8_t>(shift_char(static_cast<char>(byte), shift)));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    core::CryptoResult crypto_result;
    crypto_result.success = true;
    crypto_result.data = std::move(result);
    crypto_result.algorithm_used = core::AlgorithmType::CAESAR;
    crypto_result.original_size = plaintext.size();
    crypto_result.final_size = result.size();
    crypto_result.processing_time_ms = duration.count() / 1000.0;
    
    return crypto_result;
}

core::CryptoResult Caesar::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config)
{
    (void)config;
    auto start = std::chrono::high_resolution_clock::now();
    
    int shift = shift_;
    if (!key.empty()) {
        shift = static_cast<int>(key[0]) % 26;
    }
    
    std::vector<uint8_t> result;
    result.reserve(ciphertext.size());
    
    for (uint8_t byte : ciphertext) {
        result.push_back(static_cast<uint8_t>(shift_char(static_cast<char>(byte), -shift)));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    core::CryptoResult crypto_result;
    crypto_result.success = true;
    crypto_result.data = std::move(result);
    crypto_result.algorithm_used = core::AlgorithmType::CAESAR;
    crypto_result.original_size = ciphertext.size();
    crypto_result.final_size = result.size();
    crypto_result.processing_time_ms = duration.count() / 1000.0;
    
    return crypto_result;
}

std::string Caesar::brute_force(const std::string& ciphertext) {
    std::ostringstream result;
    result << "Caesar Brute Force Attack:\n";
    result << "==========================\n\n";
    
    for (int shift = 0; shift < 26; ++shift) {
        result << "Shift " << std::setw(2) << shift << ": ";
        
        std::string decrypted;
        for (char ch : ciphertext) {
            if (std::isalpha(ch)) {
                char base = std::isupper(ch) ? 'A' : 'a';
                decrypted += base + (ch - base - shift + 26) % 26;
            } else {
                decrypted += ch;
            }
        }
        
        result << decrypted << "\n";
    }
    
    return result.str();
}

} // namespace classical
} // namespace algorithms
} // namespace filevault
