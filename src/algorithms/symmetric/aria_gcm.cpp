/**
 * @file aria_gcm.cpp
 * @brief Implementation of ARIA-GCM AEAD encryption
 */

#include "filevault/algorithms/symmetric/aria_gcm.hpp"
#include <botan/auto_rng.h>
#include <botan/hex.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace filevault {
namespace algorithms {
namespace symmetric {

ARIA_GCM::ARIA_GCM(size_t key_bits) : key_bits_(key_bits) {
    if (key_bits != 128 && key_bits != 192 && key_bits != 256) {
        throw std::invalid_argument("ARIA-GCM key size must be 128, 192, or 256 bits");
    }
    
    botan_name_ = "ARIA-" + std::to_string(key_bits) + "/GCM";
    
    switch (key_bits) {
        case 128:
            type_ = core::AlgorithmType::ARIA_128_GCM;
            break;
        case 192:
            type_ = core::AlgorithmType::ARIA_192_GCM;
            break;
        case 256:
        default:
            type_ = core::AlgorithmType::ARIA_256_GCM;
            break;
    }
    
    spdlog::debug("Created ARIA-{}-GCM algorithm", key_bits);
}

std::string ARIA_GCM::name() const {
    return "ARIA-" + std::to_string(key_bits_) + "-GCM";
}

core::AlgorithmType ARIA_GCM::type() const {
    return type_;
}

core::CryptoResult ARIA_GCM::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    core::CryptoResult result;
    
    try {
        // Validate key size
        if (key.size() != key_size()) {
            result.success = false;
            result.error_message = "Invalid key size. Expected " + 
                std::to_string(key_size()) + " bytes, got " + 
                std::to_string(key.size());
            return result;
        }
        
        // Create cipher
        auto cipher = Botan::AEAD_Mode::create(botan_name_, Botan::Cipher_Dir::Encryption);
        if (!cipher) {
            result.success = false;
            result.error_message = "Failed to create ARIA-GCM cipher";
            return result;
        }
        
        // Generate nonce
        Botan::AutoSeeded_RNG rng;
        std::vector<uint8_t> nonce(nonce_size());
        if (config.nonce && !config.nonce->empty()) {
            nonce = *config.nonce;
        } else {
            rng.randomize(nonce.data(), nonce.size());
        }
        
        // Set key and nonce
        cipher->set_key(key.data(), key.size());
        cipher->start(nonce);
        
        // Set associated data if provided
        if (config.associated_data && !config.associated_data->empty()) {
            cipher->set_associated_data(*config.associated_data);
        }
        
        // Encrypt
        Botan::secure_vector<uint8_t> buffer(plaintext.begin(), plaintext.end());
        cipher->finish(buffer);
        
        // Extract tag (last 16 bytes)
        if (buffer.size() < tag_size()) {
            result.success = false;
            result.error_message = "Invalid ciphertext size";
            return result;
        }
        
        size_t ciphertext_len = buffer.size() - tag_size();
        
        // Store ciphertext (without tag)
        result.data.assign(buffer.begin(), buffer.begin() + ciphertext_len);
        
        // Store tag separately
        result.tag = std::vector<uint8_t>(
            buffer.begin() + ciphertext_len,
            buffer.end()
        );
        
        // Store nonce
        result.nonce = std::vector<uint8_t>(nonce.begin(), nonce.end());
        
        result.success = true;
        
        spdlog::debug("ARIA-{}-GCM encryption successful: {} bytes -> {} bytes + {} byte tag",
                      key_bits_, plaintext.size(), result.data.size(), tag_size());
        
    } catch (const Botan::Exception& e) {
        result.success = false;
        result.error_message = std::string("Botan error: ") + e.what();
        spdlog::error("ARIA-GCM encryption failed: {}", e.what());
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Error: ") + e.what();
        spdlog::error("ARIA-GCM encryption failed: {}", e.what());
    }
    
    return result;
}

core::CryptoResult ARIA_GCM::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    core::CryptoResult result;
    
    try {
        // Validate key size
        if (key.size() != key_size()) {
            result.success = false;
            result.error_message = "Invalid key size. Expected " + 
                std::to_string(key_size()) + " bytes, got " + 
                std::to_string(key.size());
            return result;
        }
        
        // Get nonce and tag from config
        if (!config.nonce.has_value() || !config.tag.has_value()) {
            result.success = false;
            result.error_message = "Nonce and tag must be provided in config";
            return result;
        }
        
        auto& nonce = config.nonce.value();
        auto& tag = config.tag.value();
        
        if (nonce.size() != nonce_size()) {
            result.success = false;
            result.error_message = "Invalid nonce size";
            return result;
        }
        
        if (tag.size() != tag_size()) {
            result.success = false;
            result.error_message = "Invalid tag size";
            return result;
        }
        
        // Create cipher
        auto cipher = Botan::AEAD_Mode::create(botan_name_, Botan::Cipher_Dir::Decryption);
        if (!cipher) {
            result.success = false;
            result.error_message = "Failed to create ARIA-GCM cipher";
            return result;
        }
        
        // Set key and nonce
        cipher->set_key(key.data(), key.size());
        cipher->start(nonce);
        
        // Set associated data if provided
        if (config.associated_data && !config.associated_data->empty()) {
            cipher->set_associated_data(*config.associated_data);
        }
        
        // Combine ciphertext + tag
        Botan::secure_vector<uint8_t> buffer;
        buffer.reserve(ciphertext.size() + tag.size());
        buffer.insert(buffer.end(), ciphertext.begin(), ciphertext.end());
        buffer.insert(buffer.end(), tag.begin(), tag.end());
        
        // Decrypt and verify
        cipher->finish(buffer);
        
        result.data.assign(buffer.begin(), buffer.end());
        result.success = true;
        
        spdlog::debug("ARIA-{}-GCM decryption successful: {} bytes -> {} bytes",
                      key_bits_, ciphertext.size(), result.data.size());
        
    } catch (const Botan::Invalid_Authentication_Tag& e) {
        result.success = false;
        result.error_message = "Authentication failed: invalid tag or corrupted data";
        spdlog::warn("ARIA-GCM decryption: authentication tag mismatch");
    } catch (const Botan::Exception& e) {
        result.success = false;
        result.error_message = std::string("Botan error: ") + e.what();
        spdlog::error("ARIA-GCM decryption failed: {}", e.what());
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Error: ") + e.what();
        spdlog::error("ARIA-GCM decryption failed: {}", e.what());
    }
    
    return result;
}

bool ARIA_GCM::is_suitable_for(core::SecurityLevel level) const {
    // ARIA is cryptographically strong, suitable for all levels
    // 256-bit is recommended for STRONG and PARANOID
    switch (level) {
        case core::SecurityLevel::WEAK:
        case core::SecurityLevel::MEDIUM:
            return true;
        case core::SecurityLevel::STRONG:
        case core::SecurityLevel::PARANOID:
            return key_bits_ >= 256;
        default:
            return true;
    }
}

} // namespace symmetric
} // namespace algorithms
} // namespace filevault
