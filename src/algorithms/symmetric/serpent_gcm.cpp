#include "filevault/algorithms/symmetric/serpent_gcm.hpp"
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <chrono>

namespace filevault {
namespace algorithms {
namespace symmetric {

Serpent_GCM::Serpent_GCM() {
    spdlog::debug("Serpent_GCM initialized");
}

std::string Serpent_GCM::name() const {
    return "Serpent-256-GCM";
}

core::AlgorithmType Serpent_GCM::type() const {
    return core::AlgorithmType::SERPENT_256_GCM;
}

size_t Serpent_GCM::key_size() const {
    return 32;  // 256 bits
}

bool Serpent_GCM::is_suitable_for(core::SecurityLevel level) const {
    // Serpent is recommended for high security levels
    return level >= core::SecurityLevel::MEDIUM;
}

core::CryptoResult Serpent_GCM::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    try {
        // Validate inputs
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
        
        if (!config.nonce || config.nonce->size() != 12) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Invalid nonce size: {} (expected 12)", 
                                           config.nonce ? config.nonce->size() : 0),
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
        
        if (plaintext.empty()) {
            return core::CryptoResult{
                .success = false,
                .error_message = "Plaintext cannot be empty",
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
        
        // Encrypt
        auto ciphertext_with_tag = process_gcm(
            plaintext,
            key,
            *config.nonce,
            {},  // No tag for encryption
            true
        );
        
        auto end = std::chrono::high_resolution_clock::now();
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Extract tag (last 16 bytes)
        if (ciphertext_with_tag.size() < 16) {
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
        
        size_t ciphertext_len = ciphertext_with_tag.size() - 16;
        std::vector<uint8_t> ciphertext_only(
            ciphertext_with_tag.begin(),
            ciphertext_with_tag.begin() + ciphertext_len
        );
        std::vector<uint8_t> tag_only(
            ciphertext_with_tag.begin() + ciphertext_len,
            ciphertext_with_tag.end()
        );
        
        return core::CryptoResult{
            .success = true,
            .error_message = "",
            .data = std::move(ciphertext_only),
            .algorithm_used = type(),
            .original_size = plaintext.size(),
            .final_size = ciphertext_only.size(),
            .processing_time_ms = time_ms,
            .salt = std::nullopt,
            .nonce = std::vector<uint8_t>(config.nonce->begin(), config.nonce->end()),
            .tag = std::move(tag_only)
        };
        
    } catch (const std::exception& e) {
        return core::CryptoResult{
            .success = false,
            .error_message = fmt::format("Serpent-256-GCM encryption failed: {}", e.what()),
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

core::CryptoResult Serpent_GCM::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    try {
        // Validate inputs
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
        
        if (!config.nonce || config.nonce->size() != 12) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Invalid nonce size: {} (expected 12)", 
                                           config.nonce ? config.nonce->size() : 0),
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
        
        if (!config.tag || config.tag->size() != 16) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Invalid auth tag size: {} (expected 16)", 
                                           config.tag ? config.tag->size() : 0),
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
        
        if (ciphertext.size() < 1) {
            return core::CryptoResult{
                .success = false,
                .error_message = fmt::format("Ciphertext too small: {} bytes", ciphertext.size()),
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
        
        spdlog::debug("Serpent-256-GCM decrypted {} bytes -> {} bytes in {:.2f}ms",
                     ciphertext.size(), plaintext.size(), time_ms);
        
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
            .error_message = fmt::format("Serpent-256-GCM decryption failed: {}", e.what()),
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

std::vector<uint8_t> Serpent_GCM::process_gcm(
    std::span<const uint8_t> input,
    std::span<const uint8_t> key,
    std::span<const uint8_t> nonce,
    std::span<const uint8_t> tag,
    bool encrypt
) {
    // Create cipher mode (Botan uses "Algorithm-KeySize/Mode" format)
    std::string mode_name = "Serpent/GCM";  // Botan accepts "Serpent/GCM" for any key size
    auto direction = encrypt ? Botan::Cipher_Dir::Encryption : Botan::Cipher_Dir::Decryption;
    
    auto cipher = Botan::Cipher_Mode::create(mode_name, direction);
    if (!cipher) {
        throw std::runtime_error("Serpent/GCM not available - Botan not compiled with Serpent support");
    }
    
    // Set key
    cipher->set_key(key.data(), key.size());
    
    // Start processing with nonce
    cipher->start(nonce.data(), nonce.size());
    
    // Prepare output buffer
    std::vector<uint8_t> buffer;
    
    if (encrypt) {
        // For encryption: input -> ciphertext + tag
        // Copy input to buffer first
        buffer.assign(input.begin(), input.end());
        
        // Encrypt in-place and append tag
        // finish() will resize buffer to include tag
        cipher->finish(buffer);
        
    } else {
        // For decryption: input + tag -> plaintext
        // Combine ciphertext and tag
        buffer.reserve(input.size() + tag.size());
        buffer.insert(buffer.end(), input.begin(), input.end());
        buffer.insert(buffer.end(), tag.begin(), tag.end());
        
        // Decrypt and verify
        cipher->finish(buffer);
    }
    
    return buffer;
}

} // namespace symmetric
} // namespace algorithms
} // namespace filevault
