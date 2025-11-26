/**
 * @file test_international_ciphers.cpp
 * @brief Unit tests for international standard ciphers
 *
 * Tests Camellia (Japan), ARIA (Korea), and SM4 (China)
 */

#include <catch2/catch_test_macros.hpp>
#include "filevault/algorithms/symmetric/camellia_gcm.hpp"
#include "filevault/algorithms/symmetric/aria_gcm.hpp"
#include "filevault/algorithms/symmetric/sm4_gcm.hpp"
#include "filevault/core/types.hpp"
#include <botan/auto_rng.h>
#include <vector>
#include <string>
#include <set>

using namespace filevault;
using namespace filevault::algorithms::symmetric;

// Helper to generate random bytes
std::vector<uint8_t> generate_random(size_t len) {
    Botan::AutoSeeded_RNG rng;
    std::vector<uint8_t> data(len);
    rng.randomize(data.data(), len);
    return data;
}

// ===========================================
// Camellia-GCM Tests
// ===========================================
TEST_CASE("Camellia-GCM Basic Operations", "[camellia][gcm][aead]") {
    SECTION("Camellia-128-GCM encrypt/decrypt round-trip") {
        Camellia_GCM cipher(128);
        
        REQUIRE(cipher.name() == "Camellia-128-GCM");
        REQUIRE(cipher.key_size() == 16);
        REQUIRE(cipher.type() == core::AlgorithmType::CAMELLIA_128_GCM);
        
        std::string plaintext = "Hello, Camellia! This is a test message.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(16);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(!enc_result.data.empty());
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("Camellia-192-GCM encrypt/decrypt round-trip") {
        Camellia_GCM cipher(192);
        
        REQUIRE(cipher.name() == "Camellia-192-GCM");
        REQUIRE(cipher.key_size() == 24);
        REQUIRE(cipher.type() == core::AlgorithmType::CAMELLIA_192_GCM);
        
        std::string plaintext = "Testing Camellia-192-GCM algorithm.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(24);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("Camellia-256-GCM encrypt/decrypt round-trip") {
        Camellia_GCM cipher(256);
        
        REQUIRE(cipher.name() == "Camellia-256-GCM");
        REQUIRE(cipher.key_size() == 32);
        REQUIRE(cipher.type() == core::AlgorithmType::CAMELLIA_256_GCM);
        
        std::string plaintext = "Testing Camellia-256-GCM - Japanese standard cipher.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(32);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("Invalid key size fails") {
        REQUIRE_THROWS_AS(Camellia_GCM(64), std::invalid_argument);
        REQUIRE_THROWS_AS(Camellia_GCM(512), std::invalid_argument);
    }
    
    SECTION("Wrong key for decryption fails") {
        Camellia_GCM cipher(256);
        
        std::string plaintext = "Secret message";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key1 = generate_random(32);
        auto key2 = generate_random(32);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key1, enc_config);
        REQUIRE(enc_result.success);
        
        // Decrypt with wrong key
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key2, dec_config);
        REQUIRE_FALSE(dec_result.success);
    }
}

// ===========================================
// ARIA-GCM Tests
// ===========================================
TEST_CASE("ARIA-GCM Basic Operations", "[aria][gcm][aead]") {
    SECTION("ARIA-128-GCM encrypt/decrypt round-trip") {
        ARIA_GCM cipher(128);
        
        REQUIRE(cipher.name() == "ARIA-128-GCM");
        REQUIRE(cipher.key_size() == 16);
        REQUIRE(cipher.type() == core::AlgorithmType::ARIA_128_GCM);
        
        std::string plaintext = "Hello, ARIA! Korean standard cipher test.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(16);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(!enc_result.data.empty());
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("ARIA-192-GCM encrypt/decrypt round-trip") {
        ARIA_GCM cipher(192);
        
        REQUIRE(cipher.name() == "ARIA-192-GCM");
        REQUIRE(cipher.key_size() == 24);
        REQUIRE(cipher.type() == core::AlgorithmType::ARIA_192_GCM);
        
        std::string plaintext = "Testing ARIA-192-GCM algorithm.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(24);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("ARIA-256-GCM encrypt/decrypt round-trip") {
        ARIA_GCM cipher(256);
        
        REQUIRE(cipher.name() == "ARIA-256-GCM");
        REQUIRE(cipher.key_size() == 32);
        REQUIRE(cipher.type() == core::AlgorithmType::ARIA_256_GCM);
        
        std::string plaintext = "Testing ARIA-256-GCM - Korean national standard.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(32);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("Invalid key size fails") {
        REQUIRE_THROWS_AS(ARIA_GCM(64), std::invalid_argument);
        REQUIRE_THROWS_AS(ARIA_GCM(384), std::invalid_argument);
    }
}

// ===========================================
// SM4-GCM Tests
// ===========================================
TEST_CASE("SM4-GCM Basic Operations", "[sm4][gcm][aead]") {
    SECTION("SM4-GCM encrypt/decrypt round-trip") {
        SM4_GCM cipher;
        
        REQUIRE(cipher.name() == "SM4-GCM");
        REQUIRE(cipher.key_size() == 16);  // SM4 only supports 128-bit key
        REQUIRE(cipher.type() == core::AlgorithmType::SM4_GCM);
        
        std::string plaintext = "Hello, SM4! Chinese standard cipher test.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(16);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(!enc_result.data.empty());
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("SM4-GCM with binary data") {
        SM4_GCM cipher;
        
        // Binary data including null bytes
        std::vector<uint8_t> pt = {0x00, 0x01, 0xFF, 0xFE, 0x80, 0x7F, 0x00, 0x00};
        auto key = generate_random(16);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        REQUIRE(enc_result.nonce.has_value());
        REQUIRE(enc_result.tag.has_value());
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE(dec_result.success);
        REQUIRE(dec_result.data == pt);
    }
    
    SECTION("SM4-GCM wrong key fails authentication") {
        SM4_GCM cipher;
        
        std::string plaintext = "Secret message";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key1 = generate_random(16);
        auto key2 = generate_random(16);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key1, enc_config);
        REQUIRE(enc_result.success);
        
        // Decrypt with wrong key should fail
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key2, dec_config);
        REQUIRE_FALSE(dec_result.success);
    }
    
    SECTION("SM4-GCM tampered ciphertext fails") {
        SM4_GCM cipher;
        
        std::string plaintext = "Test message for tampering detection";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        auto key = generate_random(16);
        
        core::EncryptionConfig enc_config;
        auto enc_result = cipher.encrypt(pt, key, enc_config);
        REQUIRE(enc_result.success);
        
        // Tamper with ciphertext
        enc_result.data[enc_result.data.size() / 2] ^= 0xFF;
        
        core::EncryptionConfig dec_config;
        dec_config.nonce = enc_result.nonce;
        dec_config.tag = enc_result.tag;
        auto dec_result = cipher.decrypt(enc_result.data, key, dec_config);
        REQUIRE_FALSE(dec_result.success);
    }
}

// ===========================================
// Security Level Tests
// ===========================================
TEST_CASE("International Cipher Security Levels", "[security][levels]") {
    SECTION("Camellia-256 suitable for all levels") {
        Camellia_GCM cipher(256);
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::WEAK));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::MEDIUM));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::STRONG));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::PARANOID));
    }
    
    SECTION("Camellia-128 suitable for WEAK/MEDIUM only") {
        Camellia_GCM cipher(128);
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::WEAK));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::MEDIUM));
        // 128-bit not suitable for STRONG/PARANOID by default
    }
    
    SECTION("ARIA-256 suitable for all levels") {
        ARIA_GCM cipher(256);
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::WEAK));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::MEDIUM));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::STRONG));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::PARANOID));
    }
    
    SECTION("SM4 suitable for WEAK/MEDIUM") {
        SM4_GCM cipher;
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::WEAK));
        REQUIRE(cipher.is_suitable_for(core::SecurityLevel::MEDIUM));
        // SM4 is 128-bit only, not ideal for PARANOID
    }
}

// ===========================================
// Nonce Uniqueness Tests
// ===========================================
TEST_CASE("Nonce Uniqueness for International Ciphers", "[nonce][security]") {
    SECTION("Camellia generates unique nonces") {
        Camellia_GCM cipher(256);
        auto key = generate_random(32);
        std::string plaintext = "Test";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        core::EncryptionConfig config;
        std::set<std::vector<uint8_t>> nonces;
        
        for (int i = 0; i < 100; ++i) {
            auto result = cipher.encrypt(pt, key, config);
            REQUIRE(result.success);
            REQUIRE(result.nonce.has_value());
            
            nonces.insert(result.nonce.value());
        }
        
        // All 100 nonces should be unique
        REQUIRE(nonces.size() == 100);
    }
    
    SECTION("ARIA generates unique nonces") {
        ARIA_GCM cipher(256);
        auto key = generate_random(32);
        std::string plaintext = "Test";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        core::EncryptionConfig config;
        std::set<std::vector<uint8_t>> nonces;
        
        for (int i = 0; i < 100; ++i) {
            auto result = cipher.encrypt(pt, key, config);
            REQUIRE(result.success);
            REQUIRE(result.nonce.has_value());
            
            nonces.insert(result.nonce.value());
        }
        
        REQUIRE(nonces.size() == 100);
    }
    
    SECTION("SM4 generates unique nonces") {
        SM4_GCM cipher;
        auto key = generate_random(16);
        std::string plaintext = "Test";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        
        core::EncryptionConfig config;
        std::set<std::vector<uint8_t>> nonces;
        
        for (int i = 0; i < 100; ++i) {
            auto result = cipher.encrypt(pt, key, config);
            REQUIRE(result.success);
            REQUIRE(result.nonce.has_value());
            
            nonces.insert(result.nonce.value());
        }
        
        REQUIRE(nonces.size() == 100);
    }
}
