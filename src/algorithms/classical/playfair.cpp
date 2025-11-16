#include "filevault/algorithms/classical/playfair.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <chrono>

namespace filevault {
namespace algorithms {
namespace classical {

Playfair::Playfair(const std::string& keyword) {
    build_matrix(keyword);
}

void Playfair::build_matrix(const std::string& keyword) {
    std::string key = keyword;
    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    
    // Remove J (combine with I)
    key.erase(std::remove(key.begin(), key.end(), 'J'), key.end());
    
    // Add alphabet
    std::string alphabet = "ABCDEFGHIKLMNOPQRSTUVWXYZ"; // No J
    key += alphabet;
    
    // Remove duplicates
    std::string unique_key;
    std::unordered_map<char, bool> seen;
    
    for (char ch : key) {
        if (!seen[ch] && std::isalpha(ch)) {
            unique_key += ch;
            seen[ch] = true;
        }
    }
    
    // Fill matrix
    for (int i = 0; i < 25; ++i) {
        matrix_[i / 5][i % 5] = unique_key[i];
    }
}

std::pair<int, int> Playfair::find_position(char ch) const {
    ch = std::toupper(ch);
    if (ch == 'J') ch = 'I';
    
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            if (matrix_[i][j] == ch) {
                return {i, j};
            }
        }
    }
    return {0, 0};
}

core::CryptoResult Playfair::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config)
{
    (void)config;
    auto start = std::chrono::high_resolution_clock::now();
    
    if (!key.empty()) {
        std::string keyword(key.begin(), key.end());
        build_matrix(keyword);
    }
    
    // Prepare plaintext: pairs of letters
    std::string text;
    for (uint8_t byte : plaintext) {
        char ch = std::toupper(static_cast<char>(byte));
        if (std::isalpha(ch)) {
            if (ch == 'J') ch = 'I';
            text += ch;
        }
    }
    
    // Add padding X if odd length
    if (text.length() % 2 != 0) {
        text += 'X';
    }
    
    std::vector<uint8_t> result;
    
    // Encrypt pairs
    for (size_t i = 0; i < text.length(); i += 2) {
        auto [r1, c1] = find_position(text[i]);
        auto [r2, c2] = find_position(text[i + 1]);
        
        char ch1, ch2;
        
        if (r1 == r2) {
            // Same row: shift right
            ch1 = matrix_[r1][(c1 + 1) % 5];
            ch2 = matrix_[r2][(c2 + 1) % 5];
        } else if (c1 == c2) {
            // Same column: shift down
            ch1 = matrix_[(r1 + 1) % 5][c1];
            ch2 = matrix_[(r2 + 1) % 5][c2];
        } else {
            // Rectangle: swap columns
            ch1 = matrix_[r1][c2];
            ch2 = matrix_[r2][c1];
        }
        
        result.push_back(static_cast<uint8_t>(ch1));
        result.push_back(static_cast<uint8_t>(ch2));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    core::CryptoResult crypto_result;
    crypto_result.success = true;
    crypto_result.data = std::move(result);
    crypto_result.algorithm_used = core::AlgorithmType::PLAYFAIR;
    crypto_result.original_size = text.length();
    crypto_result.final_size = result.size();
    crypto_result.processing_time_ms = duration.count() / 1000.0;
    return crypto_result;
}

core::CryptoResult Playfair::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config)
{
    (void)config;
    auto start = std::chrono::high_resolution_clock::now();
    
    if (!key.empty()) {
        std::string keyword(key.begin(), key.end());
        build_matrix(keyword);
    }
    
    std::string text(ciphertext.begin(), ciphertext.end());
    std::vector<uint8_t> result;
    
    // Decrypt pairs
    for (size_t i = 0; i < text.length(); i += 2) {
        auto [r1, c1] = find_position(text[i]);
        auto [r2, c2] = find_position(text[i + 1]);
        
        char ch1, ch2;
        
        if (r1 == r2) {
            // Same row: shift left
            ch1 = matrix_[r1][(c1 + 4) % 5];
            ch2 = matrix_[r2][(c2 + 4) % 5];
        } else if (c1 == c2) {
            // Same column: shift up
            ch1 = matrix_[(r1 + 4) % 5][c1];
            ch2 = matrix_[(r2 + 4) % 5][c2];
        } else {
            // Rectangle: swap columns
            ch1 = matrix_[r1][c2];
            ch2 = matrix_[r2][c1];
        }
        
        result.push_back(static_cast<uint8_t>(ch1));
        result.push_back(static_cast<uint8_t>(ch2));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    core::CryptoResult crypto_result;
    crypto_result.success = true;
    crypto_result.data = std::move(result);
    crypto_result.algorithm_used = core::AlgorithmType::PLAYFAIR;
    crypto_result.original_size = ciphertext.size();
    crypto_result.final_size = result.size();
    crypto_result.processing_time_ms = duration.count() / 1000.0;
    return crypto_result;
}

} // namespace classical
} // namespace algorithms
} // namespace filevault
