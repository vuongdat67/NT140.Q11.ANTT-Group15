/**
 * @file ecc.cpp
 * @brief Elliptic Curve Cryptography implementation
 */

#include "filevault/algorithms/asymmetric/ecc.hpp"
#include <botan/auto_rng.h>
#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/pubkey.h>
#include <botan/kdf.h>
#include <botan/cipher_mode.h>
#include <botan/hex.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <stdexcept>

namespace filevault {
namespace algorithms {
namespace asymmetric {

// Helper to get Botan curve name
static std::string get_botan_curve_name(ECCurve curve) {
    switch (curve) {
        case ECCurve::SECP256R1: return "secp256r1";
        case ECCurve::SECP384R1: return "secp384r1";
        case ECCurve::SECP521R1: return "secp521r1";
        case ECCurve::X25519:    return "curve25519";
        default: return "secp256r1";
    }
}

static size_t get_curve_key_size(ECCurve curve) {
    switch (curve) {
        case ECCurve::SECP256R1: return 32;
        case ECCurve::SECP384R1: return 48;
        case ECCurve::SECP521R1: return 66;
        case ECCurve::X25519:    return 32;
        default: return 32;
    }
}

// ============================================================================
// ECDH Implementation
// ============================================================================

ECDH::ECDH(ECCurve curve) 
    : curve_(curve), botan_curve_name_(get_botan_curve_name(curve)) {
    spdlog::debug("Created ECDH with curve {}", botan_curve_name_);
}

std::string ECDH::name() const {
    return "ECDH-" + botan_curve_name_;
}

std::string ECDH::curve_name() const {
    return botan_curve_name_;
}

size_t ECDH::key_size() const {
    return get_curve_key_size(curve_);
}

ECCKeyPair ECDH::generate_key_pair() {
    ECCKeyPair result;
    result.curve = curve_;
    result.curve_name = botan_curve_name_;
    
    try {
        Botan::AutoSeeded_RNG rng;
        
        if (curve_ == ECCurve::X25519) {
            // X25519 uses different key type - use PrivateKey interface
            // Not implemented in this version - skip X25519 for now
            throw std::runtime_error("X25519 not implemented yet");
        } else {
            Botan::EC_Group group = Botan::EC_Group::from_name(botan_curve_name_);
            Botan::ECDH_PrivateKey private_key(rng, group);
            
            auto priv_encoded = Botan::PKCS8::BER_encode(private_key);
            result.private_key.assign(priv_encoded.begin(), priv_encoded.end());
            
            auto pub_encoded = Botan::X509::BER_encode(private_key);
            result.public_key.assign(pub_encoded.begin(), pub_encoded.end());
        }
        
        spdlog::debug("Generated ECDH key pair for curve {}", botan_curve_name_);
    } catch (const std::exception& e) {
        spdlog::error("Failed to generate ECDH key pair: {}", e.what());
        throw;
    }
    
    return result;
}

ECDHResult ECDH::derive_shared_secret(
    std::span<const uint8_t> own_private_key,
    std::span<const uint8_t> peer_public_key
) {
    ECDHResult result;
    result.success = false;
    
    try {
        Botan::AutoSeeded_RNG rng;
        
        // Load private key
        auto priv_key = Botan::PKCS8::load_key(
            std::vector<uint8_t>(own_private_key.begin(), own_private_key.end())
        );
        
        // Load public key
        auto pub_key = Botan::X509::load_key(
            std::vector<uint8_t>(peer_public_key.begin(), peer_public_key.end())
        );
        
        // Create key agreement
        Botan::PK_Key_Agreement ka(*priv_key, rng, "Raw");
        
        // Get peer's public value
        auto peer_ecdh = dynamic_cast<Botan::ECDH_PublicKey*>(pub_key.get());
        if (!peer_ecdh) {
            result.error_message = "Invalid peer public key type";
            return result;
        }
        
        // Derive shared secret
        auto secret = ka.derive_key(key_size(), peer_ecdh->public_value()).bits_of();
        result.shared_secret.assign(secret.begin(), secret.end());
        result.success = true;
        
        spdlog::debug("Derived ECDH shared secret ({} bytes)", result.shared_secret.size());
        
    } catch (const std::exception& e) {
        result.error_message = std::string("ECDH error: ") + e.what();
        spdlog::error("ECDH derive_shared_secret failed: {}", e.what());
    }
    
    return result;
}

// ============================================================================
// ECDSA Implementation
// ============================================================================

ECDSA::ECDSA(ECCurve curve) 
    : curve_(curve), botan_curve_name_(get_botan_curve_name(curve)) {
    if (curve == ECCurve::X25519) {
        throw std::invalid_argument("X25519 is not supported for ECDSA, use Ed25519 instead");
    }
    spdlog::debug("Created ECDSA with curve {}", botan_curve_name_);
}

std::string ECDSA::name() const {
    return "ECDSA-" + botan_curve_name_;
}

std::string ECDSA::curve_name() const {
    return botan_curve_name_;
}

size_t ECDSA::key_size() const {
    return get_curve_key_size(curve_);
}

size_t ECDSA::signature_size() const {
    // ECDSA signature is 2 * key_size (r, s)
    return 2 * key_size();
}

ECCKeyPair ECDSA::generate_key_pair() {
    ECCKeyPair result;
    result.curve = curve_;
    result.curve_name = botan_curve_name_;
    
    try {
        Botan::AutoSeeded_RNG rng;
        Botan::EC_Group group = Botan::EC_Group::from_name(botan_curve_name_);
        Botan::ECDSA_PrivateKey private_key(rng, group);
        
        auto priv_encoded = Botan::PKCS8::BER_encode(private_key);
        result.private_key.assign(priv_encoded.begin(), priv_encoded.end());
        
        auto pub_encoded = Botan::X509::BER_encode(private_key);
        result.public_key.assign(pub_encoded.begin(), pub_encoded.end());
        
        spdlog::debug("Generated ECDSA key pair for curve {}", botan_curve_name_);
    } catch (const std::exception& e) {
        spdlog::error("Failed to generate ECDSA key pair: {}", e.what());
        throw;
    }
    
    return result;
}

ECDSASignResult ECDSA::sign(
    std::span<const uint8_t> data,
    std::span<const uint8_t> private_key
) {
    ECDSASignResult result;
    result.success = false;
    
    try {
        Botan::AutoSeeded_RNG rng;
        
        // Load private key
        auto priv_key = Botan::PKCS8::load_key(
            std::vector<uint8_t>(private_key.begin(), private_key.end())
        );
        
        // Create signer
        Botan::PK_Signer signer(*priv_key, rng, "SHA-256");
        
        // Sign
        auto sig = signer.sign_message(data.data(), data.size(), rng);
        result.signature.assign(sig.begin(), sig.end());
        result.success = true;
        
        spdlog::debug("ECDSA signed {} bytes, signature {} bytes", 
                      data.size(), result.signature.size());
        
    } catch (const std::exception& e) {
        result.error_message = std::string("ECDSA sign error: ") + e.what();
        spdlog::error("ECDSA sign failed: {}", e.what());
    }
    
    return result;
}

bool ECDSA::verify(
    std::span<const uint8_t> data,
    std::span<const uint8_t> signature,
    std::span<const uint8_t> public_key
) {
    try {
        // Load public key
        auto pub_key = Botan::X509::load_key(
            std::vector<uint8_t>(public_key.begin(), public_key.end())
        );
        
        // Create verifier
        Botan::PK_Verifier verifier(*pub_key, "SHA-256");
        
        // Verify
        bool valid = verifier.verify_message(
            data.data(), data.size(),
            signature.data(), signature.size()
        );
        
        spdlog::debug("ECDSA verification: {}", valid ? "valid" : "invalid");
        return valid;
        
    } catch (const std::exception& e) {
        spdlog::error("ECDSA verify failed: {}", e.what());
        return false;
    }
}

// ============================================================================
// ECCHybrid Implementation (ECDH + AES-GCM)
// ============================================================================

ECCHybrid::ECCHybrid(ECCurve curve) 
    : curve_(curve), 
      botan_curve_name_(get_botan_curve_name(curve)),
      ecdh_(curve) {
    
    switch (curve) {
        case ECCurve::SECP256R1:
            type_ = core::AlgorithmType::ECC_P256;
            break;
        case ECCurve::SECP384R1:
            type_ = core::AlgorithmType::ECC_P384;
            break;
        case ECCurve::SECP521R1:
            type_ = core::AlgorithmType::ECC_P521;
            break;
        default:
            type_ = core::AlgorithmType::ECC_P256;
    }
    
    spdlog::debug("Created ECCHybrid with curve {}", botan_curve_name_);
}

std::string ECCHybrid::name() const {
    return "ECC-" + botan_curve_name_ + "-AES-GCM";
}

core::AlgorithmType ECCHybrid::type() const {
    return type_;
}

size_t ECCHybrid::key_size() const {
    return get_curve_key_size(curve_);
}

ECCKeyPair ECCHybrid::generate_key_pair() {
    return ecdh_.generate_key_pair();
}

core::CryptoResult ECCHybrid::encrypt(
    std::span<const uint8_t> plaintext,
    std::span<const uint8_t> key,  // Recipient's public key
    [[maybe_unused]] const core::EncryptionConfig& config
) {
    auto start = std::chrono::high_resolution_clock::now();
    core::CryptoResult result;
    
    try {
        Botan::AutoSeeded_RNG rng;
        
        // Generate ephemeral key pair
        auto ephemeral = ecdh_.generate_key_pair();
        
        // Derive shared secret with recipient's public key
        auto dh_result = ecdh_.derive_shared_secret(ephemeral.private_key, key);
        if (!dh_result.success) {
            result.success = false;
            result.error_message = "Failed to derive shared secret: " + dh_result.error_message;
            return result;
        }
        
        // Derive AES key from shared secret using HKDF
        auto kdf = Botan::KDF::create("HKDF(SHA-256)");
        std::vector<uint8_t> aes_key(32);  // 256-bit AES key
        kdf->derive_key(aes_key, 
                       std::span<const uint8_t>(dh_result.shared_secret), 
                       std::span<const uint8_t>(),  // salt
                       std::span<const uint8_t>());  // label
        
        // Generate nonce for AES-GCM
        std::vector<uint8_t> nonce(12);
        rng.randomize(nonce.data(), nonce.size());
        
        // Encrypt with AES-256-GCM
        auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", Botan::Cipher_Dir::Encryption);
        cipher->set_key(aes_key.data(), aes_key.size());
        cipher->start(nonce);
        
        Botan::secure_vector<uint8_t> buffer(plaintext.begin(), plaintext.end());
        cipher->finish(buffer);
        
        // Output format: ephemeral_public_key || nonce || ciphertext+tag
        size_t pub_key_len = ephemeral.public_key.size();
        result.data.reserve(2 + pub_key_len + nonce.size() + buffer.size());
        
        // Store public key length (2 bytes, big-endian)
        result.data.push_back(static_cast<uint8_t>(pub_key_len >> 8));
        result.data.push_back(static_cast<uint8_t>(pub_key_len & 0xFF));
        
        // Store ephemeral public key
        result.data.insert(result.data.end(), 
                          ephemeral.public_key.begin(), ephemeral.public_key.end());
        
        // Store nonce
        result.data.insert(result.data.end(), nonce.begin(), nonce.end());
        
        // Store ciphertext + tag
        result.data.insert(result.data.end(), buffer.begin(), buffer.end());
        
        result.nonce = nonce;  // For reference
        result.success = true;
        result.algorithm_used = type_;
        result.original_size = plaintext.size();
        result.final_size = result.data.size();
        
        auto end = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        spdlog::debug("ECCHybrid encryption: {} bytes -> {} bytes in {:.2f}ms",
                      plaintext.size(), result.data.size(), result.processing_time_ms);
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("ECC encryption error: ") + e.what();
        spdlog::error("ECCHybrid encrypt failed: {}", e.what());
    }
    
    return result;
}

core::CryptoResult ECCHybrid::decrypt(
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> key,  // Own private key
    [[maybe_unused]] const core::EncryptionConfig& config
) {
    auto start = std::chrono::high_resolution_clock::now();
    core::CryptoResult result;
    
    try {
        if (ciphertext.size() < 2 + 12 + 16) {  // Min: len + nonce + tag
            result.success = false;
            result.error_message = "Ciphertext too short";
            return result;
        }
        
        // Parse ephemeral public key length
        size_t pub_key_len = (static_cast<size_t>(ciphertext[0]) << 8) | ciphertext[1];
        
        if (ciphertext.size() < 2 + pub_key_len + 12 + 16) {
            result.success = false;
            result.error_message = "Invalid ciphertext format";
            return result;
        }
        
        // Extract ephemeral public key
        std::vector<uint8_t> ephemeral_pub_key(
            ciphertext.begin() + 2, 
            ciphertext.begin() + 2 + pub_key_len
        );
        
        // Extract nonce
        std::vector<uint8_t> nonce(
            ciphertext.begin() + 2 + pub_key_len,
            ciphertext.begin() + 2 + pub_key_len + 12
        );
        
        // Extract ciphertext + tag
        std::vector<uint8_t> encrypted_data(
            ciphertext.begin() + 2 + pub_key_len + 12,
            ciphertext.end()
        );
        
        // Derive shared secret with ephemeral public key
        auto dh_result = ecdh_.derive_shared_secret(key, ephemeral_pub_key);
        if (!dh_result.success) {
            result.success = false;
            result.error_message = "Failed to derive shared secret: " + dh_result.error_message;
            return result;
        }
        
        // Derive AES key from shared secret
        auto kdf = Botan::KDF::create("HKDF(SHA-256)");
        std::vector<uint8_t> aes_key(32);
        kdf->derive_key(aes_key, 
                       std::span<const uint8_t>(dh_result.shared_secret), 
                       std::span<const uint8_t>(),  // salt
                       std::span<const uint8_t>());  // label
        
        // Decrypt with AES-256-GCM
        auto cipher = Botan::Cipher_Mode::create("AES-256/GCM", Botan::Cipher_Dir::Decryption);
        cipher->set_key(aes_key.data(), aes_key.size());
        cipher->start(nonce);
        
        Botan::secure_vector<uint8_t> buffer(encrypted_data.begin(), encrypted_data.end());
        cipher->finish(buffer);
        
        result.data.assign(buffer.begin(), buffer.end());
        result.success = true;
        result.algorithm_used = type_;
        result.original_size = ciphertext.size();
        result.final_size = result.data.size();
        
        auto end = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        spdlog::debug("ECCHybrid decryption: {} bytes -> {} bytes in {:.2f}ms",
                      ciphertext.size(), result.data.size(), result.processing_time_ms);
        
    } catch (const Botan::Invalid_Authentication_Tag& e) {
        result.success = false;
        result.error_message = "Authentication failed: invalid tag or corrupted data";
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("ECC decryption error: ") + e.what();
        spdlog::error("ECCHybrid decrypt failed: {}", e.what());
    }
    
    return result;
}

bool ECCHybrid::is_suitable_for(core::SecurityLevel level) const {
    switch (curve_) {
        case ECCurve::SECP256R1:
            return level <= core::SecurityLevel::STRONG;
        case ECCurve::SECP384R1:
        case ECCurve::SECP521R1:
            return true;  // Suitable for all levels
        default:
            return level <= core::SecurityLevel::STRONG;
    }
}

} // namespace asymmetric
} // namespace algorithms
} // namespace filevault
