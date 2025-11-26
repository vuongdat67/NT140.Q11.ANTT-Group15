/**
 * @file test_twofish.cpp
 * @brief Unit tests for Twofish-GCM encryption
 *
 * Tests Twofish-128/192/256-GCM encryption and decryption
 */

#include <catch2/catch_test_macros.hpp>
#include "filevault/algorithms/symmetric/twofish_gcm.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <vector>
#include <string>

using namespace filevault::algorithms::symmetric;
using namespace filevault::core;

// Helper function
std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// ===========================================
// Twofish-256-GCM Basic Tests
// ===========================================
TEST_CASE("Twofish-256-GCM Basic Operations", "[twofish][gcm][256]") {
    Twofish_GCM twofish(256);
    
    SECTION("Name and type") {
        REQUIRE(twofish.name() == "Twofish-256-GCM");
        REQUIRE(twofish.type() == AlgorithmType::TWOFISH_256_GCM);
        REQUIRE(twofish.key_size() == 32);
    }
    
    SECTION("Encrypt and decrypt short message") {
        std::string plaintext = "Hello, Twofish!";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        // Generate key and nonce
        auto key = CryptoEngine::generate_salt(32);  // 256-bit key
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        // Encrypt
        auto encrypted = twofish.encrypt(pt, key, config);
        
        REQUIRE(encrypted.success);
        REQUIRE(encrypted.tag.has_value());
        REQUIRE(encrypted.tag.value().size() == 16);
        REQUIRE(encrypted.nonce.has_value());
        
        // Decrypt
        config.tag = encrypted.tag;
        config.nonce = encrypted.nonce;
        
        auto decrypted = twofish.decrypt(encrypted.data, key, config);
        
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == pt);
        
        std::string result(decrypted.data.begin(), decrypted.data.end());
        REQUIRE(result == plaintext);
    }
    
    SECTION("Wrong key fails authentication") {
        std::string plaintext = "Secret message";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        auto key1 = CryptoEngine::generate_salt(32);
        auto key2 = CryptoEngine::generate_salt(32);  // Different key
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto encrypted = twofish.encrypt(pt, key1, config);
        REQUIRE(encrypted.success);
        
        config.tag = encrypted.tag;
        config.nonce = encrypted.nonce;
        
        // Try to decrypt with wrong key
        auto decrypted = twofish.decrypt(encrypted.data, key2, config);
        REQUIRE_FALSE(decrypted.success);
    }
    
    SECTION("Wrong tag fails authentication") {
        std::string plaintext = "Secret message";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        auto key = CryptoEngine::generate_salt(32);
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto encrypted = twofish.encrypt(pt, key, config);
        REQUIRE(encrypted.success);
        
        // Tamper with tag
        std::vector<uint8_t> wrong_tag = encrypted.tag.value();
        wrong_tag[0] ^= 0xFF;  // Flip bits
        
        config.tag = wrong_tag;
        config.nonce = encrypted.nonce;
        
        auto decrypted = twofish.decrypt(encrypted.data, key, config);
        REQUIRE_FALSE(decrypted.success);
    }
    
    SECTION("Empty plaintext") {
        std::vector<uint8_t> empty_pt;
        
        auto key = CryptoEngine::generate_salt(32);
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto encrypted = twofish.encrypt(empty_pt, key, config);
        REQUIRE(encrypted.success);
        REQUIRE(encrypted.data.empty());  // Empty ciphertext
        REQUIRE(encrypted.tag.has_value());  // But has tag
        
        config.tag = encrypted.tag;
        config.nonce = encrypted.nonce;
        
        auto decrypted = twofish.decrypt(encrypted.data, key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data.empty());
    }
}

// ===========================================
// Twofish-128-GCM Tests
// ===========================================
TEST_CASE("Twofish-128-GCM Basic Operations", "[twofish][gcm][128]") {
    Twofish_GCM twofish(128);
    
    SECTION("Name and type") {
        REQUIRE(twofish.name() == "Twofish-128-GCM");
        REQUIRE(twofish.type() == AlgorithmType::TWOFISH_128_GCM);
        REQUIRE(twofish.key_size() == 16);
    }
    
    SECTION("Encrypt and decrypt") {
        std::string plaintext = "Test 128-bit key";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        auto key = CryptoEngine::generate_salt(16);  // 128-bit key
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto encrypted = twofish.encrypt(pt, key, config);
        REQUIRE(encrypted.success);
        
        config.tag = encrypted.tag;
        config.nonce = encrypted.nonce;
        
        auto decrypted = twofish.decrypt(encrypted.data, key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == pt);
    }
}

// ===========================================
// Twofish-192-GCM Tests
// ===========================================
TEST_CASE("Twofish-192-GCM Basic Operations", "[twofish][gcm][192]") {
    Twofish_GCM twofish(192);
    
    SECTION("Name and type") {
        REQUIRE(twofish.name() == "Twofish-192-GCM");
        REQUIRE(twofish.type() == AlgorithmType::TWOFISH_192_GCM);
        REQUIRE(twofish.key_size() == 24);
    }
    
    SECTION("Encrypt and decrypt") {
        std::string plaintext = "Test 192-bit key";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        auto key = CryptoEngine::generate_salt(24);  // 192-bit key
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto encrypted = twofish.encrypt(pt, key, config);
        REQUIRE(encrypted.success);
        
        config.tag = encrypted.tag;
        config.nonce = encrypted.nonce;
        
        auto decrypted = twofish.decrypt(encrypted.data, key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == pt);
    }
}

// ===========================================
// Key Size Validation Tests
// ===========================================
TEST_CASE("Twofish Key Size Validation", "[twofish][validation]") {
    SECTION("Invalid key size for Twofish-256") {
        Twofish_GCM twofish(256);
        
        std::string plaintext = "Test";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        auto wrong_key = CryptoEngine::generate_salt(16);  // Wrong size
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto result = twofish.encrypt(pt, wrong_key, config);
        REQUIRE_FALSE(result.success);
    }
    
    SECTION("Invalid nonce size for decryption") {
        Twofish_GCM twofish(256);
        
        std::vector<uint8_t> ct = {0x01, 0x02, 0x03};
        auto key = CryptoEngine::generate_salt(32);
        
        EncryptionConfig config;
        config.nonce = std::vector<uint8_t>(8);  // Wrong size (should be 12)
        config.tag = std::vector<uint8_t>(16);
        
        auto result = twofish.decrypt(ct, key, config);
        REQUIRE_FALSE(result.success);
    }
}

// ===========================================
// Security Level Suitability Tests
// ===========================================
TEST_CASE("Twofish Security Level Suitability", "[twofish][security]") {
    SECTION("Twofish-256 suitable for all levels") {
        Twofish_GCM twofish(256);
        
        REQUIRE(twofish.is_suitable_for(SecurityLevel::WEAK));
        REQUIRE(twofish.is_suitable_for(SecurityLevel::MEDIUM));
        REQUIRE(twofish.is_suitable_for(SecurityLevel::STRONG));
        REQUIRE(twofish.is_suitable_for(SecurityLevel::PARANOID));
    }
    
    SECTION("Twofish-192 suitable for STRONG and below") {
        Twofish_GCM twofish(192);
        
        REQUIRE(twofish.is_suitable_for(SecurityLevel::WEAK));
        REQUIRE(twofish.is_suitable_for(SecurityLevel::MEDIUM));
        REQUIRE(twofish.is_suitable_for(SecurityLevel::STRONG));
        REQUIRE_FALSE(twofish.is_suitable_for(SecurityLevel::PARANOID));
    }
    
    SECTION("Twofish-128 suitable for MEDIUM and below") {
        Twofish_GCM twofish(128);
        
        REQUIRE(twofish.is_suitable_for(SecurityLevel::WEAK));
        REQUIRE(twofish.is_suitable_for(SecurityLevel::MEDIUM));
        REQUIRE_FALSE(twofish.is_suitable_for(SecurityLevel::STRONG));
        REQUIRE_FALSE(twofish.is_suitable_for(SecurityLevel::PARANOID));
    }
}

// ===========================================
// Round-trip with Large Data
// ===========================================
TEST_CASE("Twofish Large Data Round-trip", "[twofish][large]") {
    Twofish_GCM twofish(256);
    
    SECTION("1 KB data") {
        std::vector<uint8_t> large_data(1024);
        for (size_t i = 0; i < large_data.size(); ++i) {
            large_data[i] = static_cast<uint8_t>(i % 256);
        }
        
        auto key = CryptoEngine::generate_salt(32);
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto encrypted = twofish.encrypt(large_data, key, config);
        REQUIRE(encrypted.success);
        
        config.tag = encrypted.tag;
        config.nonce = encrypted.nonce;
        
        auto decrypted = twofish.decrypt(encrypted.data, key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == large_data);
    }
    
    SECTION("64 KB data") {
        std::vector<uint8_t> large_data(65536);
        for (size_t i = 0; i < large_data.size(); ++i) {
            large_data[i] = static_cast<uint8_t>(i % 256);
        }
        
        auto key = CryptoEngine::generate_salt(32);
        auto nonce = CryptoEngine::generate_nonce(12);
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        auto encrypted = twofish.encrypt(large_data, key, config);
        REQUIRE(encrypted.success);
        
        config.tag = encrypted.tag;
        config.nonce = encrypted.nonce;
        
        auto decrypted = twofish.decrypt(encrypted.data, key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == large_data);
    }
}

// ===========================================
// Nonce Uniqueness Tests
// ===========================================
TEST_CASE("Twofish Auto-generates Unique Nonces", "[twofish][nonce]") {
    Twofish_GCM twofish(256);
    
    std::string plaintext = "Same message";
    std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
    auto key = CryptoEngine::generate_salt(32);
    
    // Encrypt same message twice without providing nonce
    EncryptionConfig config1, config2;
    
    auto encrypted1 = twofish.encrypt(pt, key, config1);
    auto encrypted2 = twofish.encrypt(pt, key, config2);
    
    REQUIRE(encrypted1.success);
    REQUIRE(encrypted2.success);
    REQUIRE(encrypted1.nonce.has_value());
    REQUIRE(encrypted2.nonce.has_value());
    
    // Nonces should be different (auto-generated)
    REQUIRE(encrypted1.nonce.value() != encrypted2.nonce.value());
    
    // Ciphertexts should also be different
    REQUIRE(encrypted1.data != encrypted2.data);
}
