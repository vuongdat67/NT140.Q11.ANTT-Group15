/**
 * @file camellia_gcm.cpp
 * @brief Implementation of Camellia-GCM AEAD encryption
 */

#include "filevault/algorithms/symmetric/camellia_gcm.hpp"
#include <botan/auto_rng.h>
#include <botan/hex.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace filevault {
namespace algorithms {
namespace symmetric {

Camellia_GCM::Camellia_GCM(size_t key_bits) : key_bits_(key_bits) {
    if (key_bits != 128 && key_bits != 192 && key_bits != 256) {
        throw std::invalid_argument("Camellia-GCM key size must be 128, 192, or 256 bits");
    }
    
    botan_name_ = "Camellia-" + std::to_string(key_bits) + "/GCM";
    
    switch (key_bits) {
        case 128:
            type_ = core::AlgorithmType::CAMELLIA_128_GCM;
            break;
        case 192:
            type_ = core::AlgorithmType::CAMELLIA_192_GCM;
            break;
        case 256:
        default:
            type_ = core::AlgorithmType::CAMELLIA_256_GCM;
            break;
    }
    
    spdlog::debug("Created Camellia-{}-GCM algorithm", key_bits);
}

std::string Camellia_GCM::name() const {
    return "Camellia-" + std::to_string(key_bits_) + "-GCM";
}

core::AlgorithmType Camellia_GCM::type() const {
    return type_;
}

core::CryptoResult Camellia_GCM::encrypt(
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
            result.error_message = "Failed to create Camellia-GCM cipher";
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
        
        // Prepare result (nonce + ciphertext + tag)
        result.data.reserve(nonce.size() + buffer.size());
        result.data.insert(result.data.end(), nonce.begin(), nonce.end());
        result.data.insert(result.data.end(), buffer.begin(), buffer.end());
        result.success = true;
        
        spdlog::debug("Camellia-{}-GCM encryption successful: {} bytes -> {} bytes",
                      key_bits_, plaintext.size(), result.data.size());
        
    } catch (const Botan::Exception& e) {
        result.success = false;
        result.error_message = std::string("Botan error: ") + e.what();
        spdlog::error("Camellia-GCM encryption failed: {}", e.what());
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Error: ") + e.what();
        spdlog::error("Camellia-GCM encryption failed: {}", e.what());
    }
    
    return result;
}

core::CryptoResult Camellia_GCM::decrypt(
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
        
        // Validate minimum ciphertext size (nonce + tag)
        size_t min_size = nonce_size() + tag_size();
        if (ciphertext.size() < min_size) {
            result.success = false;
            result.error_message = "Ciphertext too short";
            return result;
        }
        
        // Create cipher
        auto cipher = Botan::AEAD_Mode::create(botan_name_, Botan::Cipher_Dir::Decryption);
        if (!cipher) {
            result.success = false;
            result.error_message = "Failed to create Camellia-GCM cipher";
            return result;
        }
        
        // Extract nonce from beginning
        std::vector<uint8_t> nonce(ciphertext.begin(), ciphertext.begin() + nonce_size());
        
        // Extract actual ciphertext (includes tag at end)
        Botan::secure_vector<uint8_t> buffer(
            ciphertext.begin() + nonce_size(),
            ciphertext.end()
        );
        
        // Set key and nonce
        cipher->set_key(key.data(), key.size());
        cipher->start(nonce);
        
        // Set associated data if provided
        if (config.associated_data && !config.associated_data->empty()) {
            cipher->set_associated_data(*config.associated_data);
        }
        
        // Decrypt and verify
        cipher->finish(buffer);
        
        result.data.assign(buffer.begin(), buffer.end());
        result.success = true;
        
        spdlog::debug("Camellia-{}-GCM decryption successful: {} bytes -> {} bytes",
                      key_bits_, ciphertext.size(), result.data.size());
        
    } catch (const Botan::Invalid_Authentication_Tag& e) {
        result.success = false;
        result.error_message = "Authentication failed: invalid tag or corrupted data";
        spdlog::warn("Camellia-GCM decryption: authentication tag mismatch");
    } catch (const Botan::Exception& e) {
        result.success = false;
        result.error_message = std::string("Botan error: ") + e.what();
        spdlog::error("Camellia-GCM decryption failed: {}", e.what());
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Error: ") + e.what();
        spdlog::error("Camellia-GCM decryption failed: {}", e.what());
    }
    
    return result;
}

bool Camellia_GCM::is_suitable_for(core::SecurityLevel level) const {
    // Camellia is cryptographically strong, suitable for all levels
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
