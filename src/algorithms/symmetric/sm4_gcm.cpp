/**
 * @file sm4_gcm.cpp
 * @brief Implementation of SM4-GCM AEAD encryption
 */

#include "filevault/algorithms/symmetric/sm4_gcm.hpp"
#include <botan/auto_rng.h>
#include <botan/hex.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace filevault {
namespace algorithms {
namespace symmetric {

SM4_GCM::SM4_GCM() {
    spdlog::debug("Created SM4-GCM algorithm");
}

std::string SM4_GCM::name() const {
    return "SM4-GCM";
}

core::AlgorithmType SM4_GCM::type() const {
    return core::AlgorithmType::SM4_GCM;
}

core::CryptoResult SM4_GCM::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    core::CryptoResult result;
    
    try {
        // Validate key size
        if (key.size() != key_size()) {
            result.success = false;
            result.error_message = "Invalid key size. SM4 requires 128-bit (16 bytes) key, got " + 
                std::to_string(key.size()) + " bytes";
            return result;
        }
        
        // Create cipher
        auto cipher = Botan::AEAD_Mode::create("SM4/GCM", Botan::Cipher_Dir::Encryption);
        if (!cipher) {
            result.success = false;
            result.error_message = "Failed to create SM4-GCM cipher";
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
        
        spdlog::debug("SM4-GCM encryption successful: {} bytes -> {} bytes",
                      plaintext.size(), result.data.size());
        
    } catch (const Botan::Exception& e) {
        result.success = false;
        result.error_message = std::string("Botan error: ") + e.what();
        spdlog::error("SM4-GCM encryption failed: {}", e.what());
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Error: ") + e.what();
        spdlog::error("SM4-GCM encryption failed: {}", e.what());
    }
    
    return result;
}

core::CryptoResult SM4_GCM::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,
    const core::EncryptionConfig& config
) {
    core::CryptoResult result;
    
    try {
        // Validate key size
        if (key.size() != key_size()) {
            result.success = false;
            result.error_message = "Invalid key size. SM4 requires 128-bit (16 bytes) key, got " + 
                std::to_string(key.size()) + " bytes";
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
        auto cipher = Botan::AEAD_Mode::create("SM4/GCM", Botan::Cipher_Dir::Decryption);
        if (!cipher) {
            result.success = false;
            result.error_message = "Failed to create SM4-GCM cipher";
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
        
        spdlog::debug("SM4-GCM decryption successful: {} bytes -> {} bytes",
                      ciphertext.size(), result.data.size());
        
    } catch (const Botan::Invalid_Authentication_Tag& e) {
        result.success = false;
        result.error_message = "Authentication failed: invalid tag or corrupted data";
        spdlog::warn("SM4-GCM decryption: authentication tag mismatch");
    } catch (const Botan::Exception& e) {
        result.success = false;
        result.error_message = std::string("Botan error: ") + e.what();
        spdlog::error("SM4-GCM decryption failed: {}", e.what());
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Error: ") + e.what();
        spdlog::error("SM4-GCM decryption failed: {}", e.what());
    }
    
    return result;
}

bool SM4_GCM::is_suitable_for(core::SecurityLevel level) const {
    // SM4 only has 128-bit key, suitable for WEAK and MEDIUM
    // For STRONG/PARANOID, prefer 256-bit algorithms
    switch (level) {
        case core::SecurityLevel::WEAK:
        case core::SecurityLevel::MEDIUM:
            return true;
        case core::SecurityLevel::STRONG:
        case core::SecurityLevel::PARANOID:
            return false;  // Prefer 256-bit algorithms
        default:
            return true;
    }
}

} // namespace symmetric
} // namespace algorithms
} // namespace filevault
