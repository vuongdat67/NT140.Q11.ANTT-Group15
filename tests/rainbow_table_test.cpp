#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <spdlog/spdlog.h>
#include "filevault/core/crypto_engine.hpp"

using namespace filevault;

/**
 * @brief Rainbow Table Protection Test
 * 
 * Verifies that:
 * 1. Same password + same plaintext → DIFFERENT ciphertexts (unique salt/nonce)
 * 2. Salt is random and unique for each encryption
 * 3. Nonce is random and unique for each encryption
 * 4. No two encryptions produce identical output
 */

int main() {
    spdlog::set_level(spdlog::level::warn);
    
    std::cout << "========================================\n";
    std::cout << "Rainbow Table Protection Test\n";
    std::cout << "========================================\n\n";
    
    core::CryptoEngine engine;
    engine.initialize();
    
    const std::string password = "MySecretPassword123!";
    const std::string plaintext_str = "This is a test message for rainbow table protection";
    std::vector<uint8_t> plaintext(plaintext_str.begin(), plaintext_str.end());
    
    const int num_tests = 10;
    
    std::vector<std::vector<uint8_t>> salts;
    std::vector<std::vector<uint8_t>> nonces;
    std::vector<std::vector<uint8_t>> ciphertexts;
    
    std::cout << "Encrypting same plaintext " << num_tests << " times with same password...\n\n";
    
    // Perform multiple encryptions
    for (int i = 0; i < num_tests; i++) {
        // Generate random salt and nonce
        auto salt = engine.generate_salt(16);
        auto nonce = engine.generate_nonce(12);
        
        // Derive key
        core::EncryptionConfig config;
        config.algorithm = core::AlgorithmType::AES_256_GCM;
        config.kdf = core::KDFType::ARGON2ID;
        config.level = core::SecurityLevel::WEAK; // Fast for testing
        config.apply_security_level();
        config.nonce = nonce;
        
        auto key = engine.derive_key(password, salt, config);
        
        // Get algorithm
        auto* algo = engine.get_algorithm(config.algorithm);
        if (!algo) {
            std::cerr << "Failed to get algorithm\n";
            return 1;
        }
        
        // Encrypt
        auto result = algo->encrypt(plaintext, key, config);
        
        if (!result.success) {
            std::cerr << "Encryption failed: " << result.error_message << "\n";
            return 1;
        }
        
        salts.push_back(salt);
        nonces.push_back(nonce);
        ciphertexts.push_back(result.data);
        
        std::cout << "Encryption " << (i + 1) << ":\n";
        std::cout << "  Salt:       " << salt.size() << " bytes\n";
        std::cout << "  Nonce:      " << nonce.size() << " bytes\n";
        std::cout << "  Ciphertext: " << result.data.size() << " bytes\n";
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Verification\n";
    std::cout << "========================================\n\n";
    
    bool all_tests_passed = true;
    
    // Test 1: All salts are unique
    std::set<std::vector<uint8_t>> unique_salts(salts.begin(), salts.end());
    bool salts_unique = (unique_salts.size() == salts.size());
    
    std::cout << "Test 1: Salt Uniqueness\n";
    std::cout << "  Total salts:  " << salts.size() << "\n";
    std::cout << "  Unique salts: " << unique_salts.size() << "\n";
    std::cout << "  Result: " << (salts_unique ? "[PASS]" : "[FAIL]") << "\n\n";
    
    if (!salts_unique) all_tests_passed = false;
    
    // Test 2: All nonces are unique
    std::set<std::vector<uint8_t>> unique_nonces(nonces.begin(), nonces.end());
    bool nonces_unique = (unique_nonces.size() == nonces.size());
    
    std::cout << "Test 2: Nonce Uniqueness\n";
    std::cout << "  Total nonces:  " << nonces.size() << "\n";
    std::cout << "  Unique nonces: " << unique_nonces.size() << "\n";
    std::cout << "  Result: " << (nonces_unique ? "[PASS]" : "[FAIL]") << "\n\n";
    
    if (!nonces_unique) all_tests_passed = false;
    
    // Test 3: All ciphertexts are unique
    std::set<std::vector<uint8_t>> unique_ciphertexts(ciphertexts.begin(), ciphertexts.end());
    bool ciphertexts_unique = (unique_ciphertexts.size() == ciphertexts.size());
    
    std::cout << "Test 3: Ciphertext Uniqueness\n";
    std::cout << "  Total ciphertexts:  " << ciphertexts.size() << "\n";
    std::cout << "  Unique ciphertexts: " << unique_ciphertexts.size() << "\n";
    std::cout << "  Result: " << (ciphertexts_unique ? "[PASS]" : "[FAIL]") << "\n\n";
    
    if (!ciphertexts_unique) all_tests_passed = false;
    
    // Test 4: No ciphertext equals plaintext (sanity check)
    bool no_plaintext_leakage = true;
    for (const auto& ct : ciphertexts) {
        if (ct == plaintext) {
            no_plaintext_leakage = false;
            break;
        }
    }
    
    std::cout << "Test 4: No Plaintext Leakage\n";
    std::cout << "  Result: " << (no_plaintext_leakage ? "[PASS]" : "[FAIL]") << "\n\n";
    
    if (!no_plaintext_leakage) all_tests_passed = false;
    
    std::cout << "========================================\n";
    std::cout << "Summary:\n";
    std::cout << "  Rainbow Table Protection: " << (all_tests_passed ? "[PASS]" : "[FAIL]") << "\n";
    std::cout << "========================================\n";
    
    return all_tests_passed ? 0 : 1;
}
