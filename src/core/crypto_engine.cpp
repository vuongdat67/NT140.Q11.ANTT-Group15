#include "filevault/core/crypto_engine.hpp"
#include "filevault/core/types.hpp"
#include "filevault/algorithms/symmetric/aes_gcm.hpp"
#include "filevault/algorithms/classical/caesar.hpp"
#include "filevault/algorithms/classical/vigenere.hpp"
#include "filevault/algorithms/classical/playfair.hpp"
#include <botan/auto_rng.h>
#include <botan/argon2.h>
#include <botan/pwdhash.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>

namespace filevault {
namespace core {

CryptoEngine::CryptoEngine() {
    spdlog::debug("CryptoEngine created");
}

CryptoEngine::~CryptoEngine() {
    spdlog::debug("CryptoEngine destroyed");
}

void CryptoEngine::initialize() {
    spdlog::info("Initializing CryptoEngine...");
    
    // Register modern symmetric algorithms
    register_algorithm(std::make_unique<algorithms::symmetric::AES_GCM>(128));
    register_algorithm(std::make_unique<algorithms::symmetric::AES_GCM>(192));
    register_algorithm(std::make_unique<algorithms::symmetric::AES_GCM>(256));
    
    // Register classical ciphers (educational only)
    register_algorithm(std::make_unique<algorithms::classical::Caesar>());
    register_algorithm(std::make_unique<algorithms::classical::Vigenere>());
    register_algorithm(std::make_unique<algorithms::classical::Playfair>());
    
    spdlog::info("CryptoEngine initialized with {} algorithms", algorithms_.size());
}

void CryptoEngine::register_algorithm(std::unique_ptr<ICryptoAlgorithm> algorithm) {
    auto type = algorithm->type();
    algorithms_[type] = std::move(algorithm);
    spdlog::debug("Registered algorithm: {}", algorithm_name(type));
}

ICryptoAlgorithm* CryptoEngine::get_algorithm(AlgorithmType type) {
    auto it = algorithms_.find(type);
    if (it != algorithms_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<uint8_t> CryptoEngine::derive_key(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    const EncryptionConfig& config
) {
    spdlog::debug("Deriving key with {} (iterations: {}, memory: {}KB)",
                  kdf_name(config.kdf), config.kdf_iterations, config.kdf_memory_kb);
    
    // Determine key size based on algorithm
    size_t key_size = 32; // Default 256-bit
    auto* algo = get_algorithm(config.algorithm);
    if (algo) {
        key_size = algo->key_size();
    }
    
    std::vector<uint8_t> key(key_size);
    
    try {
        switch (config.kdf) {
            case KDFType::ARGON2ID:
            case KDFType::ARGON2I: {
                // Use Argon2
                std::string argon_name = (config.kdf == KDFType::ARGON2ID) 
                    ? "Argon2id" 
                    : "Argon2i";
                
                auto pwdhash = Botan::PasswordHashFamily::create(argon_name);
                if (!pwdhash) {
                    throw std::runtime_error("Failed to create Argon2 instance");
                }
                
                auto argon2 = pwdhash->from_params(
                    config.kdf_memory_kb,
                    config.kdf_iterations, 
                    config.kdf_parallelism
                );
                
                argon2->hash(key, password, salt);
                break;
            }
            
            case KDFType::PBKDF2_SHA256:
            case KDFType::PBKDF2_SHA512: {
                // Use PBKDF2
                std::string hash_fn = (config.kdf == KDFType::PBKDF2_SHA256) 
                    ? "HMAC(SHA-256)" 
                    : "HMAC(SHA-512)";
                
                auto pwdhash = Botan::PasswordHashFamily::create(
                    std::string("PBKDF2(") + hash_fn + ")"
                );
                
                if (!pwdhash) {
                    throw std::runtime_error("Failed to create PBKDF2 instance");
                }
                
                auto pbkdf2 = pwdhash->from_params(config.kdf_iterations);
                pbkdf2->hash(
                    key,
                    password,
                    salt
                );
                break;
            }
            
            case KDFType::SCRYPT: {
                // Use scrypt
                auto pwdhash = Botan::PasswordHashFamily::create("Scrypt");
                if (!pwdhash) {
                    throw std::runtime_error("Failed to create Scrypt instance");
                }
                
                // Scrypt parameters: N, r, p
                uint32_t N = config.kdf_iterations;
                uint32_t r = 8;
                uint32_t p = config.kdf_parallelism;
                
                auto scrypt = pwdhash->from_params(N, r, p);
                scrypt->hash(
                    key,
                    password,
                    salt
                );
                break;
            }
            
            default:
                throw std::runtime_error("Unknown KDF type");
        }
        
        spdlog::debug("Key derived successfully ({}  bytes)", key.size());
        return key;
        
    } catch (const Botan::Exception& e) {
        spdlog::error("Botan error in key derivation: {}", e.what());
        throw;
    }
}

std::vector<uint8_t> CryptoEngine::generate_salt(size_t length) {
    Botan::AutoSeeded_RNG rng;
    std::vector<uint8_t> salt(length);
    rng.randomize(salt.data(), salt.size());
    spdlog::debug("Generated random salt ({} bytes)", length);
    return salt;
}

std::vector<uint8_t> CryptoEngine::generate_nonce(size_t length) {
    Botan::AutoSeeded_RNG rng;
    std::vector<uint8_t> nonce(length);
    rng.randomize(nonce.data(), nonce.size());
    spdlog::debug("Generated random nonce ({} bytes)", length);
    return nonce;
}

std::string CryptoEngine::algorithm_name(AlgorithmType type) {
    switch (type) {
        case AlgorithmType::AES_128_GCM: return "AES-128-GCM";
        case AlgorithmType::AES_192_GCM: return "AES-192-GCM";
        case AlgorithmType::AES_256_GCM: return "AES-256-GCM";
        case AlgorithmType::CHACHA20_POLY1305: return "ChaCha20-Poly1305";
        case AlgorithmType::SHA256: return "SHA-256";
        case AlgorithmType::SHA512: return "SHA-512";
        case AlgorithmType::BLAKE2B: return "BLAKE2b";
        case AlgorithmType::CAESAR: return "Caesar";
        case AlgorithmType::VIGENERE: return "Vigenère";
        case AlgorithmType::PLAYFAIR: return "Playfair";
        default: return "Unknown";
    }
}

std::string CryptoEngine::kdf_name(KDFType type) {
    switch (type) {
        case KDFType::ARGON2ID: return "Argon2id";
        case KDFType::ARGON2I: return "Argon2i";
        case KDFType::PBKDF2_SHA256: return "PBKDF2-SHA256";
        case KDFType::PBKDF2_SHA512: return "PBKDF2-SHA512";
        case KDFType::SCRYPT: return "scrypt";
        default: return "Unknown";
    }
}

std::optional<AlgorithmType> CryptoEngine::parse_algorithm(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "aes-128-gcm" || lower == "aes128gcm") return AlgorithmType::AES_128_GCM;
    if (lower == "aes-192-gcm" || lower == "aes192gcm") return AlgorithmType::AES_192_GCM;
    if (lower == "aes-256-gcm" || lower == "aes256gcm") return AlgorithmType::AES_256_GCM;
    if (lower == "chacha20-poly1305" || lower == "chacha20") return AlgorithmType::CHACHA20_POLY1305;
    if (lower == "sha256" || lower == "sha-256") return AlgorithmType::SHA256;
    if (lower == "sha512" || lower == "sha-512") return AlgorithmType::SHA512;
    if (lower == "blake2b" || lower == "blake2") return AlgorithmType::BLAKE2B;
    if (lower == "caesar") return AlgorithmType::CAESAR;
    if (lower == "vigenere" || lower == "vigenère") return AlgorithmType::VIGENERE;
    if (lower == "playfair") return AlgorithmType::PLAYFAIR;
    
    return std::nullopt;
}

std::optional<KDFType> CryptoEngine::parse_kdf(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "argon2id" || lower == "argon2-id") return KDFType::ARGON2ID;
    if (lower == "argon2i" || lower == "argon2-i") return KDFType::ARGON2I;
    if (lower == "pbkdf2-sha256" || lower == "pbkdf2") return KDFType::PBKDF2_SHA256;
    if (lower == "pbkdf2-sha512") return KDFType::PBKDF2_SHA512;
    if (lower == "scrypt") return KDFType::SCRYPT;
    
    return std::nullopt;
}

std::optional<SecurityLevel> CryptoEngine::parse_security_level(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "weak" || lower == "low") return SecurityLevel::WEAK;
    if (lower == "medium" || lower == "normal") return SecurityLevel::MEDIUM;
    if (lower == "strong" || lower == "high") return SecurityLevel::STRONG;
    if (lower == "paranoid" || lower == "maximum") return SecurityLevel::PARANOID;
    
    return std::nullopt;
}

} // namespace core
} // namespace filevault
