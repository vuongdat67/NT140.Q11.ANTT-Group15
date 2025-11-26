/**
 * @file test_ecc.cpp
 * @brief Tests for ECC (ECDH, ECDSA, ECCHybrid) implementations
 */

#include <catch2/catch_test_macros.hpp>
#include "filevault/algorithms/asymmetric/ecc.hpp"
#include <string>
#include <vector>

using namespace filevault::algorithms::asymmetric;
using namespace filevault::core;

// ============================================================================
// ECDH Tests
// ============================================================================

TEST_CASE("ECDH Key Exchange", "[ecc][ecdh]") {
    SECTION("P-256 key pair generation") {
        ECDH ecdh(ECCurve::SECP256R1);
        
        auto keypair = ecdh.generate_key_pair();
        
        REQUIRE(!keypair.public_key.empty());
        REQUIRE(!keypair.private_key.empty());
        REQUIRE(keypair.curve == ECCurve::SECP256R1);
        REQUIRE(keypair.curve_name == "secp256r1");
    }
    
    SECTION("P-384 key pair generation") {
        ECDH ecdh(ECCurve::SECP384R1);
        
        auto keypair = ecdh.generate_key_pair();
        
        REQUIRE(!keypair.public_key.empty());
        REQUIRE(!keypair.private_key.empty());
        REQUIRE(keypair.curve == ECCurve::SECP384R1);
    }
    
    SECTION("P-521 key pair generation") {
        ECDH ecdh(ECCurve::SECP521R1);
        
        auto keypair = ecdh.generate_key_pair();
        
        REQUIRE(!keypair.public_key.empty());
        REQUIRE(!keypair.private_key.empty());
        REQUIRE(keypair.curve == ECCurve::SECP521R1);
    }
    
    SECTION("Shared secret derivation") {
        ECDH alice(ECCurve::SECP256R1);
        ECDH bob(ECCurve::SECP256R1);
        
        // Generate key pairs
        auto alice_keys = alice.generate_key_pair();
        auto bob_keys = bob.generate_key_pair();
        
        // Derive shared secrets
        auto alice_secret = alice.derive_shared_secret(
            alice_keys.private_key, bob_keys.public_key);
        auto bob_secret = bob.derive_shared_secret(
            bob_keys.private_key, alice_keys.public_key);
        
        // Both should succeed and produce same secret
        REQUIRE(alice_secret.success);
        REQUIRE(bob_secret.success);
        REQUIRE(alice_secret.shared_secret == bob_secret.shared_secret);
        REQUIRE(alice_secret.shared_secret.size() == 32);  // P-256 = 32 bytes
    }
}

// ============================================================================
// ECDSA Tests
// ============================================================================

TEST_CASE("ECDSA Digital Signatures", "[ecc][ecdsa]") {
    SECTION("P-256 sign and verify") {
        ECDSA ecdsa(ECCurve::SECP256R1);
        
        auto keypair = ecdsa.generate_key_pair();
        std::string message = "Hello, ECDSA!";
        std::vector<uint8_t> data(message.begin(), message.end());
        
        // Sign
        auto sign_result = ecdsa.sign(data, keypair.private_key);
        REQUIRE(sign_result.success);
        REQUIRE(!sign_result.signature.empty());
        
        // Verify
        bool valid = ecdsa.verify(data, sign_result.signature, keypair.public_key);
        REQUIRE(valid);
    }
    
    SECTION("Signature fails with wrong key") {
        ECDSA ecdsa(ECCurve::SECP256R1);
        
        auto keypair1 = ecdsa.generate_key_pair();
        auto keypair2 = ecdsa.generate_key_pair();
        
        std::string message = "Test message";
        std::vector<uint8_t> data(message.begin(), message.end());
        
        // Sign with keypair1
        auto sign_result = ecdsa.sign(data, keypair1.private_key);
        REQUIRE(sign_result.success);
        
        // Verify with keypair2's public key should fail
        bool valid = ecdsa.verify(data, sign_result.signature, keypair2.public_key);
        REQUIRE_FALSE(valid);
    }
    
    SECTION("Signature fails with modified data") {
        ECDSA ecdsa(ECCurve::SECP256R1);
        
        auto keypair = ecdsa.generate_key_pair();
        std::string message = "Original message";
        std::vector<uint8_t> data(message.begin(), message.end());
        
        // Sign
        auto sign_result = ecdsa.sign(data, keypair.private_key);
        REQUIRE(sign_result.success);
        
        // Modify data
        data[0] ^= 0xFF;
        
        // Verify should fail
        bool valid = ecdsa.verify(data, sign_result.signature, keypair.public_key);
        REQUIRE_FALSE(valid);
    }
    
    SECTION("P-384 sign and verify") {
        ECDSA ecdsa(ECCurve::SECP384R1);
        
        auto keypair = ecdsa.generate_key_pair();
        std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
        
        auto sign_result = ecdsa.sign(data, keypair.private_key);
        REQUIRE(sign_result.success);
        
        bool valid = ecdsa.verify(data, sign_result.signature, keypair.public_key);
        REQUIRE(valid);
    }
}

// ============================================================================
// ECCHybrid Tests (ECDH + AES-GCM)
// ============================================================================

TEST_CASE("ECCHybrid Encryption", "[ecc][hybrid]") {
    SECTION("P-256 encrypt/decrypt round-trip") {
        ECCHybrid hybrid(ECCurve::SECP256R1);
        
        // Generate recipient's key pair
        auto recipient_keys = hybrid.generate_key_pair();
        
        // Test data
        std::string plaintext = "Hello, ECC Hybrid Encryption!";
        std::vector<uint8_t> data(plaintext.begin(), plaintext.end());
        
        EncryptionConfig config;
        
        // Encrypt with recipient's public key
        auto encrypted = hybrid.encrypt(data, recipient_keys.public_key, config);
        REQUIRE(encrypted.success);
        REQUIRE(!encrypted.data.empty());
        REQUIRE(encrypted.data.size() > data.size());  // Should be larger due to overhead
        
        // Decrypt with recipient's private key
        auto decrypted = hybrid.decrypt(encrypted.data, recipient_keys.private_key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data.size() == data.size());
        REQUIRE(decrypted.data == data);
    }
    
    SECTION("P-384 encrypt/decrypt round-trip") {
        ECCHybrid hybrid(ECCurve::SECP384R1);
        
        auto recipient_keys = hybrid.generate_key_pair();
        
        std::string plaintext = "Testing P-384 curve with hybrid encryption.";
        std::vector<uint8_t> data(plaintext.begin(), plaintext.end());
        
        EncryptionConfig config;
        
        auto encrypted = hybrid.encrypt(data, recipient_keys.public_key, config);
        REQUIRE(encrypted.success);
        
        auto decrypted = hybrid.decrypt(encrypted.data, recipient_keys.private_key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == data);
    }
    
    SECTION("P-521 encrypt/decrypt round-trip") {
        ECCHybrid hybrid(ECCurve::SECP521R1);
        
        auto recipient_keys = hybrid.generate_key_pair();
        
        std::string plaintext = "Maximum security with P-521!";
        std::vector<uint8_t> data(plaintext.begin(), plaintext.end());
        
        EncryptionConfig config;
        
        auto encrypted = hybrid.encrypt(data, recipient_keys.public_key, config);
        REQUIRE(encrypted.success);
        
        auto decrypted = hybrid.decrypt(encrypted.data, recipient_keys.private_key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == data);
    }
    
    SECTION("Decryption fails with wrong key") {
        ECCHybrid hybrid(ECCurve::SECP256R1);
        
        auto recipient1_keys = hybrid.generate_key_pair();
        auto recipient2_keys = hybrid.generate_key_pair();
        
        std::string plaintext = "Secret data";
        std::vector<uint8_t> data(plaintext.begin(), plaintext.end());
        
        EncryptionConfig config;
        
        // Encrypt for recipient1
        auto encrypted = hybrid.encrypt(data, recipient1_keys.public_key, config);
        REQUIRE(encrypted.success);
        
        // Try to decrypt with recipient2's private key - should fail
        auto decrypted = hybrid.decrypt(encrypted.data, recipient2_keys.private_key, config);
        REQUIRE_FALSE(decrypted.success);
    }
    
    SECTION("Large data encryption") {
        ECCHybrid hybrid(ECCurve::SECP256R1);
        
        auto recipient_keys = hybrid.generate_key_pair();
        
        // 1MB of data
        std::vector<uint8_t> large_data(1024 * 1024);
        for (size_t i = 0; i < large_data.size(); ++i) {
            large_data[i] = static_cast<uint8_t>(i & 0xFF);
        }
        
        EncryptionConfig config;
        
        auto encrypted = hybrid.encrypt(large_data, recipient_keys.public_key, config);
        REQUIRE(encrypted.success);
        
        auto decrypted = hybrid.decrypt(encrypted.data, recipient_keys.private_key, config);
        REQUIRE(decrypted.success);
        REQUIRE(decrypted.data == large_data);
    }
    
    SECTION("Algorithm properties") {
        ECCHybrid p256(ECCurve::SECP256R1);
        ECCHybrid p384(ECCurve::SECP384R1);
        ECCHybrid p521(ECCurve::SECP521R1);
        
        REQUIRE(p256.name() == "ECC-secp256r1-AES-GCM");
        REQUIRE(p384.name() == "ECC-secp384r1-AES-GCM");
        REQUIRE(p521.name() == "ECC-secp521r1-AES-GCM");
        
        REQUIRE(p256.type() == AlgorithmType::ECC_P256);
        REQUIRE(p384.type() == AlgorithmType::ECC_P384);
        REQUIRE(p521.type() == AlgorithmType::ECC_P521);
        
        REQUIRE(p256.key_size() == 32);
        REQUIRE(p384.key_size() == 48);
        REQUIRE(p521.key_size() == 66);
    }
}
