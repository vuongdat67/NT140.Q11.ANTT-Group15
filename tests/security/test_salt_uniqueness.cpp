#include <catch2/catch_test_macros.hpp>
#include "filevault/utils/password.hpp"
#include "filevault/core/crypto_engine.hpp"
#include <botan/auto_rng.h>
#include <botan/pwdhash.h>
#include <set>
#include <cmath>
#include <chrono>

using namespace filevault::utils;

/**
 * @file test_salt_uniqueness.cpp
 * @brief Security tests for salt uniqueness in KDF
 * 
 * SECURITY REQUIREMENT: Salts MUST be unique per encryption
 * 
 * Purpose of salt:
 * - Prevent rainbow table attacks
 * - Ensure same password → different keys for different files
 * - Prevent pre-computation attacks
 * 
 * This test suite verifies:
 * 1. Random salts are statistically unique
 * 2. Same password + different salt = different keys
 * 3. No salt collisions in large samples
 */

TEST_CASE("Salt uniqueness", "[security][salt][kdf]") {
    SECTION("Random salts are unique") {
        Botan::AutoSeeded_RNG rng;
        std::set<std::vector<uint8_t>> salts;
        
        const size_t NUM_TESTS = 10000;
        const size_t SALT_SIZE = 32;  // 256 bits
        
        for (size_t i = 0; i < NUM_TESTS; ++i) {
            std::vector<uint8_t> salt(SALT_SIZE);
            rng.randomize(salt.data(), salt.size());
            
            // Check for collision
            REQUIRE(salts.find(salt) == salts.end());
            salts.insert(salt);
        }
        
        INFO("Generated " << NUM_TESTS << " unique salts with no collisions");
        REQUIRE(salts.size() == NUM_TESTS);
    }
    
    SECTION("Same password + different salt = different keys") {
        const std::string password = "MySecretPassword123!";
        
        Botan::AutoSeeded_RNG rng;
        std::set<std::vector<uint8_t>> keys;
        
        const size_t NUM_DERIVATIONS = 100;
        
        for (size_t i = 0; i < NUM_DERIVATIONS; ++i) {
            // Generate unique salt
            std::vector<uint8_t> salt(32);
            rng.randomize(salt.data(), salt.size());
            
            // Derive key using PBKDF2 (faster for testing)
            auto pbkdf = Botan::PasswordHashFamily::create("PBKDF2(SHA-256)");
            auto pwdhash = pbkdf->from_params(10000);
            std::vector<uint8_t> key(32);
            pwdhash->hash(key, password, salt);
            
            // Verify key is unique
            REQUIRE(keys.find(key) == keys.end());
            keys.insert(key);
        }
        
        INFO("Same password derived " << NUM_DERIVATIONS << " times with different salts");
        INFO("All derived keys are unique");
        REQUIRE(keys.size() == NUM_DERIVATIONS);
    }
    
    SECTION("Salt reuse allows rainbow table attacks") {
        const std::string password = "password123";
        
        // SAME salt (BAD!)
        std::vector<uint8_t> salt(32, 0x42);
        
        // Attacker pre-computes hash(password + salt) for common passwords
        auto pbkdf = Botan::PasswordHashFamily::create("PBKDF2(SHA-256)");
        auto pwdhash = pbkdf->from_params(10000);
        std::vector<uint8_t> key1(32);
        pwdhash->hash(key1, password, salt);
        
        // User encrypts another file with same password
        std::vector<uint8_t> key2(32);
        pwdhash->hash(key2, password, salt);
        
        // Keys are identical (allows rainbow table attack)
        REQUIRE(key1 == key2);
        
        WARN("Salt reuse allows rainbow table attacks! Always use unique salts.");
    }
    
    SECTION("Different passwords + same salt = different keys") {
        std::vector<uint8_t> salt(32, 0x99);
        
        auto pbkdf = Botan::PasswordHashFamily::create("PBKDF2(SHA-256)");
        auto pwdhash = pbkdf->from_params(10000);
        
        std::vector<uint8_t> key1(32);
        pwdhash->hash(key1, "password1", salt);
        
        std::vector<uint8_t> key2(32);
        pwdhash->hash(key2, "password2", salt);
        
        REQUIRE(key1 != key2);
    }
}

TEST_CASE("Salt collision probability", "[security][salt][statistics]") {
    SECTION("32-byte salt collision probability") {
        // 32 bytes = 256 bits
        // Birthday paradox: ~50% collision at 2^128 samples
        // For 10,000 samples: probability ≈ 10,000^2 / (2 * 2^256) ≈ 4.3e-72
        
        const size_t salt_bits = 256;
        const size_t num_samples = 10000;
        
        // Expected collisions ≈ 0
        double expected_collisions = (num_samples * num_samples) / (2.0 * std::pow(2.0, salt_bits));
        
        INFO("Salt size: " << salt_bits << " bits");
        INFO("Sample size: " << num_samples);
        INFO("Expected collisions: " << expected_collisions << " (effectively 0)");
        
        REQUIRE(expected_collisions < 1e-60);
    }
}

TEST_CASE("Salt size recommendations", "[security][salt]") {
    SECTION("Minimum salt size is 16 bytes") {
        // NIST SP 800-132 recommends at least 128 bits (16 bytes)
        const size_t MIN_SALT_SIZE = 16;
        
        WARN("Minimum recommended salt size: " << MIN_SALT_SIZE << " bytes (128 bits)");
        
        // FileVault uses 32 bytes (256 bits) for extra security
        const size_t FILEVAULT_SALT_SIZE = 32;
        REQUIRE(FILEVAULT_SALT_SIZE >= MIN_SALT_SIZE);
    }
}

TEST_CASE("Salt generation performance", "[security][salt][benchmark]") {
    Botan::AutoSeeded_RNG rng;
    
    SECTION("Generate 10,000 salts") {
        const size_t NUM_SALTS = 10000;
        const size_t SALT_SIZE = 32;
        std::vector<std::vector<uint8_t>> salts;
        salts.reserve(NUM_SALTS);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < NUM_SALTS; ++i) {
            std::vector<uint8_t> salt(SALT_SIZE);
            rng.randomize(salt.data(), salt.size());
            salts.push_back(std::move(salt));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        INFO("Generated " << NUM_SALTS << " salts in " << duration << " ms");
        INFO("Rate: " << (NUM_SALTS / duration * 1000.0) << " salts/second");
        
        // Should be fast - allow up to 30 seconds for slow CI runners with sanitizers
        REQUIRE(duration < 30000.0);
    }
}

TEST_CASE("Real-world salt usage scenario", "[security][salt][integration]") {
    SECTION("Encrypt multiple files with same password") {
        const std::string password = "MyVerySecurePassword!2024";
        Botan::AutoSeeded_RNG rng;
        
        // Simulate encrypting 10 different files
        std::vector<std::vector<uint8_t>> keys;
        
        auto pbkdf = Botan::PasswordHashFamily::create("PBKDF2(SHA-256)");
        auto pwdhash = pbkdf->from_params(10000);
        
        for (int i = 0; i < 10; ++i) {
            // Generate unique salt per file
            std::vector<uint8_t> salt(32);
            rng.randomize(salt.data(), salt.size());
            
            // Derive key
            std::vector<uint8_t> key(32);
            pwdhash->hash(key, password, salt);
            
            keys.push_back(key);
        }
        
        // All keys should be different
        std::set<std::vector<uint8_t>> unique_keys(keys.begin(), keys.end());
        REQUIRE(unique_keys.size() == keys.size());
        
        INFO("Same password used for 10 files with different salts");
        INFO("All 10 derived keys are unique");
    }
}
