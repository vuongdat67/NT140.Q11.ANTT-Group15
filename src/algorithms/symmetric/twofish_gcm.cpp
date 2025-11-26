/**
 * @file twofish_gcm.cpp
 * @brief Twofish-GCM encryption implementation
 *
 * Uses Botan's Twofish/GCM mode for authenticated encryption
 *
 * @author FileVault Team
 * @date 2024
 */

#include "filevault/algorithms/symmetric/twofish_gcm.hpp"
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <botan/auto_rng.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <chrono>
#include <fmt/format.h>

namespace filevault {
namespace algorithms {
namespace symmetric {

Twofish_GCM::Twofish_GCM(size_t key_bits) : key_bits_(key_bits) {
    // Validate key size
    if (key_bits != 128 && key_bits != 192 && key_bits != 256) {
        throw std::invalid_argument("Invalid Twofish key size: must be 128, 192, or 256 bits");
    }
    
    // Set algorithm type based on key size
    switch (key_bits) {
        case 128:
            type_ = core::AlgorithmType::TWOFISH_128_GCM;
            break;
        case 192:
            type_ = core::AlgorithmType::TWOFISH_192_GCM;
            break;
        case 256:
        default:
            type_ = core::AlgorithmType::TWOFISH_256_GCM;
            break;
    }
    
    // Botan uses "Twofish/GCM" for all key sizes
    botan_name_ = "Twofish/GCM";
    
    spdlog::debug("Twofish_GCM initialized with {} bit key", key_bits_);
}

std::string Twofish_GCM::name() const {
    return fmt::format("Twofish-{}-GCM", key_bits_);
}

core::AlgorithmType Twofish_GCM::type() const {
    return type_;
}

size_t Twofish_GCM::key_size() const {
    return key_bits_ / 8;
}

bool Twofish_GCM::is_suitable_for(core::SecurityLevel level) const {
    // Twofish-256 is suitable for all security levels
    // Twofish-128 is suitable for MEDIUM and below
    switch (key_bits_) {
        case 256:
            return true;
        case 192:
            return level <= core::SecurityLevel::STRONG;
        case 128:
            return level <= core::SecurityLevel::MEDIUM;
        default:
            return false;
    }
}

core::CryptoResult Twofish_GCM::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    try {
        // Validate key size
        if (key.size() != key_size()) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Invalid key size: {} (expected {})", key.size(), key_size()),
                .data = {},
                .algorithm_used = type(),
                .original_size = 0,
                .final_size = 0,
                .processing_time_ms = 0.0,
                .salt = std::nullopt,
                .nonce = std::nullopt,
                .tag = std::nullopt
            };
        }
        
        // Generate or use provided nonce
        std::vector<uint8_t> nonce;
        if (config.nonce.has_value() && config.nonce.value().size() == nonce_size()) {
            nonce = config.nonce.value();
            spdlog::debug("Twofish-GCM: Using provided nonce (testing mode)");
        } else {
            // Generate NEW unique nonce for THIS encryption
            Botan::AutoSeeded_RNG rng;
            nonce.resize(nonce_size());
            rng.randomize(nonce.data(), nonce.size());
            spdlog::debug("Twofish-GCM: Generated new unique nonce ({} bytes)", nonce.size());
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Encrypt
        auto ciphertext_with_tag = process_gcm(
            plaintext,
            key,
            nonce,
            {},  // No tag for encryption
            true
        );
        
        auto end = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Extract tag (last 16 bytes)
        if (ciphertext_with_tag.size() < tag_size()) {
            return core::CryptoResult{
                .success = false,
                .error_message = "Encrypted data too small",
                .data = {},
                .algorithm_used = type(),
                .original_size = 0,
                .final_size = 0,
                .processing_time_ms = 0.0,
                .salt = std::nullopt,
                .nonce = std::nullopt,
                .tag = std::nullopt
            };
        }
        
        size_t ciphertext_len = ciphertext_with_tag.size() - tag_size();
        std::vector<uint8_t> ciphertext_only(
            ciphertext_with_tag.begin(),
            ciphertext_with_tag.begin() + ciphertext_len
        );
        std::vector<uint8_t> tag_only(
            ciphertext_with_tag.begin() + ciphertext_len,
            ciphertext_with_tag.end()
        );
        
        spdlog::debug("Twofish-{}-GCM encrypted {} bytes -> {} bytes in {:.2f}ms",
                     key_bits_, plaintext.size(), ciphertext_only.size(), time_ms);
        
        return core::CryptoResult{
            .success = true,
            .error_message = "",
            .data = std::move(ciphertext_only),
            .algorithm_used = type(),
            .original_size = plaintext.size(),
            .final_size = ciphertext_only.size(),
            .processing_time_ms = time_ms,
            .salt = std::nullopt,
            .nonce = std::move(nonce),
            .tag = std::move(tag_only)
        };
        
    } catch (const std::exception& e) {
        return core::CryptoResult{
            .success = false,
            .error_message = fmt::format("Twofish-{}-GCM encryption failed: {}", key_bits_, e.what()),
            .data = {},
            .algorithm_used = type(),
            .original_size = 0,
            .final_size = 0,
            .processing_time_ms = 0.0,
            .salt = std::nullopt,
            .nonce = std::nullopt,
            .tag = std::nullopt
        };
    }
}

core::CryptoResult Twofish_GCM::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    try {
        // Validate key size
        if (key.size() != key_size()) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Invalid key size: {} (expected {})", key.size(), key_size()),
                .data = {},
                .algorithm_used = type(),
                .original_size = 0,
                .final_size = 0,
                .processing_time_ms = 0.0,
                .salt = std::nullopt,
                .nonce = std::nullopt,
                .tag = std::nullopt
            };
        }
        
        if (!config.nonce || config.nonce->size() != nonce_size()) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Invalid nonce size: {} (expected {})", 
                                           config.nonce ? config.nonce->size() : 0, nonce_size()),
                .data = {},
                .algorithm_used = type(),
                .original_size = 0,
                .final_size = 0,
                .processing_time_ms = 0.0,
                .salt = std::nullopt,
                .nonce = std::nullopt,
                .tag = std::nullopt
            };
        }
        
        if (!config.tag || config.tag->size() != tag_size()) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Invalid auth tag size: {} (expected {})", 
                                           config.tag ? config.tag->size() : 0, tag_size()),
                .data = {},
                .algorithm_used = type(),
                .original_size = 0,
                .final_size = 0,
                .processing_time_ms = 0.0,
                .salt = std::nullopt,
                .nonce = std::nullopt,
                .tag = std::nullopt
            };
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Decrypt
        auto plaintext = process_gcm(
            ciphertext,
            key,
            *config.nonce,
            *config.tag,
            false
        );
        
        auto end = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        spdlog::debug("Twofish-{}-GCM decrypted {} bytes -> {} bytes in {:.2f}ms",
                     key_bits_, ciphertext.size(), plaintext.size(), time_ms);
        
        return core::CryptoResult{
            .success = true,
            .error_message = "",
            .data = std::move(plaintext),
            .algorithm_used = type(),
            .original_size = ciphertext.size(),
            .final_size = plaintext.size(),
            .processing_time_ms = time_ms,
            .salt = std::nullopt,
            .nonce = std::nullopt,
            .tag = std::nullopt
        };
        
    } catch (const Botan::Invalid_Authentication_Tag& e) {
        return core::CryptoResult{
            .success = false,
            .error_message = "Authentication failed: wrong password or corrupted data",
            .data = {},
            .algorithm_used = type(),
            .original_size = 0,
            .final_size = 0,
            .processing_time_ms = 0.0,
            .salt = std::nullopt,
            .nonce = std::nullopt,
            .tag = std::nullopt
        };
    } catch (const std::exception& e) {
        return core::CryptoResult{
            .success = false,
            .error_message = fmt::format("Twofish-{}-GCM decryption failed: {}", key_bits_, e.what()),
            .data = {},
            .algorithm_used = type(),
            .original_size = 0,
            .final_size = 0,
            .processing_time_ms = 0.0,
            .salt = std::nullopt,
            .nonce = std::nullopt,
            .tag = std::nullopt
        };
    }
}

std::vector<uint8_t> Twofish_GCM::process_gcm(
    std::span<const uint8_t> input,
    std::span<const uint8_t> key,
    std::span<const uint8_t> nonce,
    std::span<const uint8_t> tag,
    bool encrypt
) {
    auto direction = encrypt ? Botan::Cipher_Dir::Encryption : Botan::Cipher_Dir::Decryption;
    
    auto cipher = Botan::Cipher_Mode::create(botan_name_, direction);
    if (!cipher) {
        throw std::runtime_error("Twofish/GCM not available - Botan not compiled with Twofish support");
    }
    
    // Set key
    cipher->set_key(key.data(), key.size());
    
    // Start processing with nonce
    cipher->start(nonce.data(), nonce.size());
    
    // Prepare output buffer
    std::vector<uint8_t> buffer;
    
    if (encrypt) {
        buffer.assign(input.begin(), input.end());
        cipher->finish(buffer);
    } else {
        // Combine ciphertext and tag for decryption
        buffer.reserve(input.size() + tag.size());
        buffer.insert(buffer.end(), input.begin(), input.end());
        buffer.insert(buffer.end(), tag.begin(), tag.end());
        cipher->finish(buffer);
    }
    
    return buffer;
}

} // namespace symmetric
} // namespace algorithms
} // namespace filevault
