#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <spdlog/spdlog.h>
#include "filevault/core/crypto_engine.hpp"
#include "filevault/algorithms/symmetric/aes_gcm.hpp"

using namespace filevault;

// NIST SP 800-38D Test Vectors for AES-GCM
// https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/cavp-testing-block-cipher-modes

struct NISTTestVector {
    std::string name;
    std::vector<uint8_t> key;
    std::vector<uint8_t> iv;
    std::vector<uint8_t> plaintext;
    std::vector<uint8_t> aad;
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;
};

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (uint8_t byte : bytes) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

int main() {
    spdlog::set_level(spdlog::level::warn); // Reduce noise
    
    std::cout << "========================================\n";
    std::cout << "NIST AES-GCM Test Vectors Validation\n";
    std::cout << "========================================\n\n";
    
    // NIST Test Case 1 - AES-256-GCM
    NISTTestVector test1;
    test1.name = "NIST AES-256-GCM Test Case 1";
    test1.key = hex_to_bytes("0000000000000000000000000000000000000000000000000000000000000000");
    test1.iv = hex_to_bytes("000000000000000000000000");
    test1.plaintext = hex_to_bytes("");
    test1.aad = hex_to_bytes("");
    test1.ciphertext = hex_to_bytes("");
    test1.tag = hex_to_bytes("530f8afbc74536b9a963b4f1c4cb738b");
    
    // NIST Test Case 2 - AES-256-GCM with plaintext
    NISTTestVector test2;
    test2.name = "NIST AES-256-GCM Test Case 2";
    test2.key = hex_to_bytes("0000000000000000000000000000000000000000000000000000000000000000");
    test2.iv = hex_to_bytes("000000000000000000000000");
    test2.plaintext = hex_to_bytes("00000000000000000000000000000000");
    test2.aad = hex_to_bytes("");
    test2.ciphertext = hex_to_bytes("cea7403d4d606b6e074ec5d3baf39d18");
    test2.tag = hex_to_bytes("d0d1c8a799996bf0265b98b5d48ab919");
    
    // NIST Test Case 3 - AES-128-GCM
    NISTTestVector test3;
    test3.name = "NIST AES-128-GCM Test Case 3";
    test3.key = hex_to_bytes("00000000000000000000000000000000");
    test3.iv = hex_to_bytes("000000000000000000000000");
    test3.plaintext = hex_to_bytes("00000000000000000000000000000000");
    test3.aad = hex_to_bytes("");
    test3.ciphertext = hex_to_bytes("0388dace60b6a392f328c2b971b2fe78");
    test3.tag = hex_to_bytes("ab6e47d42cec13bdf53a67b21257bddf");
    
    std::vector<NISTTestVector> tests = {test1, test2, test3};
    
    int passed = 0;
    int failed = 0;
    
    for (auto& test : tests) {
        std::cout << "Testing: " << test.name << "\n";
        std::cout << "  Key:       " << bytes_to_hex(test.key) << "\n";
        std::cout << "  IV:        " << bytes_to_hex(test.iv) << "\n";
        std::cout << "  Plaintext: " << (test.plaintext.empty() ? "(empty)" : bytes_to_hex(test.plaintext)) << "\n";
        
        try {
            // Create AES-GCM algorithm
            size_t key_bits = test.key.size() * 8;
            algorithms::symmetric::AES_GCM aes(key_bits);
            
            // Setup encryption config
            core::EncryptionConfig config;
            config.nonce = test.iv;
            if (!test.aad.empty()) {
                config.associated_data = test.aad;
            }
            
            // Encrypt
            auto result = aes.encrypt(test.plaintext, test.key, config);
            
            if (!result.success) {
                std::cout << "  Result: FAILED (encryption error: " << result.error_message << ")\n\n";
                failed++;
                continue;
            }
            
            // Compare ciphertext
            bool ciphertext_match = (result.data == test.ciphertext);
            
            // Compare tag
            bool tag_match = false;
            if (result.tag.has_value()) {
                tag_match = (result.tag.value() == test.tag);
            }
            
            if (ciphertext_match && tag_match) {
                std::cout << "  Result: [PASS]\n";
                passed++;
            } else {
                std::cout << "  Result: [FAIL]\n";
                if (!ciphertext_match) {
                    std::cout << "    Expected CT: " << bytes_to_hex(test.ciphertext) << "\n";
                    std::cout << "    Got CT:      " << bytes_to_hex(result.data) << "\n";
                }
                if (!tag_match) {
                    std::cout << "    Expected Tag: " << bytes_to_hex(test.tag) << "\n";
                    if (result.tag.has_value()) {
                        std::cout << "    Got Tag:      " << bytes_to_hex(result.tag.value()) << "\n";
                    } else {
                        std::cout << "    Got Tag:      (none)\n";
                    }
                }
                failed++;
            }
            
        } catch (const std::exception& e) {
            std::cout << "  Result: [FAIL] (exception: " << e.what() << ")\n";
            failed++;
        }
        
        std::cout << "\n";
    }
    
    std::cout << "========================================\n";
    std::cout << "Summary:\n";
    std::cout << "  Passed: " << passed << "/" << tests.size() << "\n";
    std::cout << "  Failed: " << failed << "/" << tests.size() << "\n";
    std::cout << "========================================\n";
    
    return (failed == 0) ? 0 : 1;
}
