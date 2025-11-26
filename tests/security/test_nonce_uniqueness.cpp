#include <catch2/catch_test_macros.hpp>
#include "filevault/algorithms/symmetric/aes_gcm.hpp"
#include "filevault/algorithms/symmetric/chacha20_poly1305.hpp"
#include <botan/auto_rng.h>
#include <set>
#include <chrono>
#include <cmath>

using namespace filevault::algorithms::symmetric;
using namespace filevault::core;

/**
 * @file test_nonce_uniqueness.cpp
 * @brief Critical security tests for nonce uniqueness
 * 
 * SECURITY REQUIREMENT: Nonces MUST be unique per encryption
 * Reusing a nonce with the same key in GCM/ChaCha20-Poly1305 is catastrophic:
 * - Breaks confidentiality (attacker can XOR ciphertexts)
 * - Breaks authenticity (attacker can forge messages)
 * 
 * This test suite verifies:
 * 1. Random nonces are statistically unique
 * 2. Same plaintext + same key + different nonce = different ciphertext
 * 3. No nonce collisions in large sample sizes
 */

TEST_CASE("Nonce uniqueness - AES-GCM", "[security][nonce][aes-gcm]") {
    AES_GCM cipher(256);
    EncryptionConfig config;
    
    SECTION("Random nonces are unique") {
        Botan::AutoSeeded_RNG rng;
        std::set<std::vector<uint8_t>> nonces;
        
        const size_t NUM_TESTS = 10000;
        
        for (size_t i = 0; i < NUM_TESTS; ++i) {
            std::vector<uint8_t> nonce(12);
            rng.randomize(nonce.data(), nonce.size());
            
            // Check for collision
            REQUIRE(nonces.find(nonce) == nonces.end());
            nonces.insert(nonce);
        }
        
        INFO("Generated " << NUM_TESTS << " unique nonces with no collisions");
        REQUIRE(nonces.size() == NUM_TESTS);
    }
    
    SECTION("Same data + same key + different nonce = different ciphertext") {
        std::string plaintext = "This is a test message that should encrypt differently each time.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        std::vector<uint8_t> key(32, 0xAB);
        
        Botan::AutoSeeded_RNG rng;
        std::set<std::vector<uint8_t>> ciphertexts;
        
        const size_t NUM_ENCRYPTIONS = 1000;
        
        for (size_t i = 0; i < NUM_ENCRYPTIONS; ++i) {
            // Generate unique nonce
            std::vector<uint8_t> nonce(12);
            rng.randomize(nonce.data(), nonce.size());
            config.nonce = nonce;
            
            auto result = cipher.encrypt(pt, key, config);
            REQUIRE(result.success);
            
            // Verify ciphertext is unique
            REQUIRE(ciphertexts.find(result.data) == ciphertexts.end());
            ciphertexts.insert(result.data);
        }
        
        INFO("Same plaintext encrypted " << NUM_ENCRYPTIONS << " times with different nonces");
        INFO("All ciphertexts are unique");
        REQUIRE(ciphertexts.size() == NUM_ENCRYPTIONS);
    }
    
    SECTION("Nonce reuse detection - catastrophic failure") {
        std::string plaintext1 = "Message 1";
        std::string plaintext2 = "Message 2";
        
        std::vector<uint8_t> pt1(plaintext1.begin(), plaintext1.end());
        std::vector<uint8_t> pt2(plaintext2.begin(), plaintext2.end());
        std::vector<uint8_t> key(32, 0xCD);
        
        // Use SAME nonce for both encryptions (BAD!)
        std::vector<uint8_t> nonce(12, 0x42);
        config.nonce = nonce;
        
        auto encrypted1 = cipher.encrypt(pt1, key, config);
        REQUIRE(encrypted1.success);
        
        auto encrypted2 = cipher.encrypt(pt2, key, config);
        REQUIRE(encrypted2.success);
        
        // XOR the two ciphertexts
        std::vector<uint8_t> xor_result;
        size_t min_len = std::min(encrypted1.data.size(), encrypted2.data.size());
        for (size_t i = 0; i < min_len; ++i) {
            xor_result.push_back(encrypted1.data[i] ^ encrypted2.data[i]);
        }
        
        // XOR of ciphertexts = XOR of plaintexts (information leak!)
        std::vector<uint8_t> expected_xor;
        for (size_t i = 0; i < min_len; ++i) {
            expected_xor.push_back(pt1[i] ^ pt2[i]);
        }
        
        // This demonstrates why nonce reuse is catastrophic
        REQUIRE(xor_result == expected_xor);
        
        WARN("Nonce reuse allows attacker to XOR ciphertexts and learn plaintext XOR!");
    }
}

TEST_CASE("Nonce uniqueness - ChaCha20-Poly1305", "[security][nonce][chacha20]") {
    ChaCha20Poly1305 cipher;
    EncryptionConfig config;
    
    SECTION("Random nonces are unique") {
        Botan::AutoSeeded_RNG rng;
        std::set<std::vector<uint8_t>> nonces;
        
        const size_t NUM_TESTS = 10000;
        
        for (size_t i = 0; i < NUM_TESTS; ++i) {
            std::vector<uint8_t> nonce(12);
            rng.randomize(nonce.data(), nonce.size());
            
            REQUIRE(nonces.find(nonce) == nonces.end());
            nonces.insert(nonce);
        }
        
        INFO("Generated " << NUM_TESTS << " unique nonces for ChaCha20-Poly1305");
        REQUIRE(nonces.size() == NUM_TESTS);
    }
    
    SECTION("Same data + same key + different nonce = different ciphertext") {
        std::string plaintext = "ChaCha20 test message for nonce uniqueness verification.";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        std::vector<uint8_t> key(32, 0xEF);
        
        Botan::AutoSeeded_RNG rng;
        std::set<std::vector<uint8_t>> ciphertexts;
        
        const size_t NUM_ENCRYPTIONS = 1000;
        
        for (size_t i = 0; i < NUM_ENCRYPTIONS; ++i) {
            std::vector<uint8_t> nonce(12);
            rng.randomize(nonce.data(), nonce.size());
            config.nonce = nonce;
            
            auto result = cipher.encrypt(pt, key, config);
            REQUIRE(result.success);
            
            REQUIRE(ciphertexts.find(result.data) == ciphertexts.end());
            ciphertexts.insert(result.data);
        }
        
        INFO("ChaCha20-Poly1305: " << NUM_ENCRYPTIONS << " encryptions, all unique");
        REQUIRE(ciphertexts.size() == NUM_ENCRYPTIONS);
    }
}

TEST_CASE("Nonce collision probability", "[security][nonce][statistics]") {
    SECTION("12-byte nonce collision probability") {
        // 12 bytes = 96 bits
        // Birthday paradox: ~50% collision at 2^48 samples
        // For 10,000 samples: probability ≈ 10,000^2 / (2 * 2^96) ≈ 1.26e-24
        
        const size_t nonce_bits = 96;
        const size_t num_samples = 10000;
        
        // Expected collisions ≈ 0
        double expected_collisions = (num_samples * num_samples) / (2.0 * std::pow(2.0, nonce_bits));
        
        INFO("Nonce size: " << nonce_bits << " bits");
        INFO("Sample size: " << num_samples);
        INFO("Expected collisions: " << expected_collisions << " (effectively 0)");
        
        REQUIRE(expected_collisions < 1e-20);
    }
}

TEST_CASE("Nonce generation performance", "[security][nonce][benchmark]") {
    Botan::AutoSeeded_RNG rng;
    
    SECTION("Generate 100,000 nonces") {
        const size_t NUM_NONCES = 100000;
        std::vector<std::vector<uint8_t>> nonces;
        nonces.reserve(NUM_NONCES);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < NUM_NONCES; ++i) {
            std::vector<uint8_t> nonce(12);
            rng.randomize(nonce.data(), nonce.size());
            nonces.push_back(std::move(nonce));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        INFO("Generated " << NUM_NONCES << " nonces in " << duration << " ms");
        INFO("Rate: " << (NUM_NONCES / duration * 1000.0) << " nonces/second");
        
        // Should be fast - allow up to 60 seconds for slow CI runners with sanitizers
        REQUIRE(duration < 60000.0);
    }
}

TEST_CASE("Cross-algorithm nonce comparison", "[security][nonce]") {
    SECTION("AES-GCM and ChaCha20-Poly1305 use same nonce size") {
        AES_GCM aes(256);
        ChaCha20Poly1305 chacha;
        
        REQUIRE(aes.nonce_size() == 12);
        REQUIRE(chacha.nonce_size() == 12);
        
        // Both can use the same nonce generation strategy
        Botan::AutoSeeded_RNG rng;
        std::vector<uint8_t> nonce(12);
        rng.randomize(nonce.data(), nonce.size());
        
        EncryptionConfig config;
        config.nonce = nonce;
        
        std::string plaintext = "Test message";
        std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
        std::vector<uint8_t> key(32, 0x42);
        
        auto aes_result = aes.encrypt(pt, key, config);
        REQUIRE(aes_result.success);
        
        auto chacha_result = chacha.encrypt(pt, key, config);
        REQUIRE(chacha_result.success);
        
        // Different algorithms produce different ciphertexts
        // even with same nonce (because they use different ciphers)
        REQUIRE(aes_result.data != chacha_result.data);
    }
}
