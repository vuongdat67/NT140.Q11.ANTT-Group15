/**
 * @file test_hash.cpp
 * @brief Unit tests for hash algorithms
 *
 * Tests SHA-2, SHA-3, BLAKE2 family with NIST test vectors
 */

#include <catch2/catch_test_macros.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/mac.h>
#include <string>
#include <vector>

// Helper function to compute hash
std::string compute_hash(const std::string& algo, const std::string& input) {
    auto hash_func = Botan::HashFunction::create(algo);
    REQUIRE(hash_func != nullptr);
    
    hash_func->update(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    auto result = hash_func->final();
    return Botan::hex_encode(result);
}

std::string compute_hash_bytes(const std::string& algo, const std::vector<uint8_t>& input) {
    auto hash_func = Botan::HashFunction::create(algo);
    REQUIRE(hash_func != nullptr);
    
    hash_func->update(input.data(), input.size());
    auto result = hash_func->final();
    return Botan::hex_encode(result);
}

// ===========================================
// SHA-256 Tests (NIST FIPS 180-4)
// ===========================================
TEST_CASE("SHA-256 NIST Test Vectors", "[hash][sha256][nist]") {
    SECTION("Empty string") {
        // NIST test vector for empty input
        std::string result = compute_hash("SHA-256", "");
        REQUIRE(result == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }
    
    SECTION("abc") {
        // NIST test vector for "abc"
        std::string result = compute_hash("SHA-256", "abc");
        REQUIRE(result == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }
    
    SECTION("448-bit message") {
        // "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
        std::string input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        std::string result = compute_hash("SHA-256", input);
        REQUIRE(result == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    }
    
    SECTION("896-bit message") {
        std::string input = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
        std::string result = compute_hash("SHA-256", input);
        REQUIRE(result == "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
    }
}

// ===========================================
// SHA-512 Tests (NIST FIPS 180-4)
// ===========================================
TEST_CASE("SHA-512 NIST Test Vectors", "[hash][sha512][nist]") {
    SECTION("Empty string") {
        std::string result = compute_hash("SHA-512", "");
        REQUIRE(result == "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
    }
    
    SECTION("abc") {
        std::string result = compute_hash("SHA-512", "abc");
        REQUIRE(result == "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    }
    
    SECTION("448-bit message") {
        std::string input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        std::string result = compute_hash("SHA-512", input);
        REQUIRE(result == "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445");
    }
}

// ===========================================
// SHA-384 Tests
// ===========================================
TEST_CASE("SHA-384 Test Vectors", "[hash][sha384]") {
    SECTION("Empty string") {
        std::string result = compute_hash("SHA-384", "");
        REQUIRE(result == "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b");
    }
    
    SECTION("abc") {
        std::string result = compute_hash("SHA-384", "abc");
        REQUIRE(result == "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");
    }
}

// ===========================================
// SHA3-256 Tests (NIST FIPS 202)
// ===========================================
TEST_CASE("SHA3-256 NIST Test Vectors", "[hash][sha3][nist]") {
    SECTION("Empty string") {
        std::string result = compute_hash("SHA-3(256)", "");
        REQUIRE(result == "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
    }
    
    SECTION("abc") {
        std::string result = compute_hash("SHA-3(256)", "abc");
        REQUIRE(result == "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532");
    }
}

// ===========================================
// SHA3-512 Tests
// ===========================================
TEST_CASE("SHA3-512 Test Vectors", "[hash][sha3-512]") {
    SECTION("Empty string") {
        std::string result = compute_hash("SHA-3(512)", "");
        REQUIRE(result == "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26");
    }
    
    SECTION("abc") {
        std::string result = compute_hash("SHA-3(512)", "abc");
        REQUIRE(result == "b751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e10e116e9192af3c91a7ec57647e3934057340b4cf408d5a56592f8274eec53f0");
    }
}

// ===========================================
// BLAKE2b Tests (RFC 7693)
// ===========================================
TEST_CASE("BLAKE2b-512 Test Vectors", "[hash][blake2b]") {
    SECTION("Empty string") {
        std::string result = compute_hash("BLAKE2b(512)", "");
        REQUIRE(result == "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce");
    }
    
    SECTION("abc") {
        std::string result = compute_hash("BLAKE2b(512)", "abc");
        REQUIRE(result == "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d17d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923");
    }
}

TEST_CASE("BLAKE2b-256 Test Vectors", "[hash][blake2b-256]") {
    SECTION("Empty string") {
        std::string result = compute_hash("BLAKE2b(256)", "");
        REQUIRE(result == "0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8");
    }
}

// ===========================================
// HMAC Tests (RFC 4231)
// ===========================================
TEST_CASE("HMAC-SHA256 Test Vectors", "[hmac][sha256][rfc4231]") {
    SECTION("Test Case 1") {
        // Key: 0x0b repeated 20 times
        // Data: "Hi There"
        std::vector<uint8_t> key(20, 0x0b);
        std::string data = "Hi There";
        
        auto hmac = Botan::MessageAuthenticationCode::create("HMAC(SHA-256)");
        REQUIRE(hmac != nullptr);
        
        hmac->set_key(key);
        hmac->update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        auto result = hmac->final();
        
        std::string expected = "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";
        REQUIRE(Botan::hex_encode(result) == expected);
    }
    
    SECTION("Test Case 2") {
        // Key: "Jefe"
        // Data: "what do ya want for nothing?"
        std::string key_str = "Jefe";
        std::vector<uint8_t> key(key_str.begin(), key_str.end());
        std::string data = "what do ya want for nothing?";
        
        auto hmac = Botan::MessageAuthenticationCode::create("HMAC(SHA-256)");
        REQUIRE(hmac != nullptr);
        
        hmac->set_key(key);
        hmac->update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
        auto result = hmac->final();
        
        std::string expected = "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843";
        REQUIRE(Botan::hex_encode(result) == expected);
    }
}

// ===========================================
// Hash Properties Tests
// ===========================================
TEST_CASE("Hash Algorithm Properties", "[hash][properties]") {
    SECTION("SHA-256 output length") {
        auto hash = Botan::HashFunction::create("SHA-256");
        REQUIRE(hash != nullptr);
        REQUIRE(hash->output_length() == 32);  // 256 bits = 32 bytes
    }
    
    SECTION("SHA-512 output length") {
        auto hash = Botan::HashFunction::create("SHA-512");
        REQUIRE(hash != nullptr);
        REQUIRE(hash->output_length() == 64);  // 512 bits = 64 bytes
    }
    
    SECTION("SHA3-256 output length") {
        auto hash = Botan::HashFunction::create("SHA-3(256)");
        REQUIRE(hash != nullptr);
        REQUIRE(hash->output_length() == 32);
    }
    
    SECTION("BLAKE2b-512 output length") {
        auto hash = Botan::HashFunction::create("BLAKE2b(512)");
        REQUIRE(hash != nullptr);
        REQUIRE(hash->output_length() == 64);
    }
}

TEST_CASE("Hash Collision Resistance", "[hash][security]") {
    SECTION("Different inputs produce different hashes") {
        std::string hash1 = compute_hash("SHA-256", "input1");
        std::string hash2 = compute_hash("SHA-256", "input2");
        
        REQUIRE(hash1 != hash2);
    }
    
    SECTION("Similar inputs produce vastly different hashes (avalanche)") {
        std::string hash1 = compute_hash("SHA-256", "test");
        std::string hash2 = compute_hash("SHA-256", "Test");  // Capital T
        
        REQUIRE(hash1 != hash2);
        
        // Count different characters (should be many due to avalanche effect)
        int diff_count = 0;
        for (size_t i = 0; i < hash1.size(); ++i) {
            if (hash1[i] != hash2[i]) diff_count++;
        }
        
        // At least 50% of characters should be different
        REQUIRE(diff_count > static_cast<int>(hash1.size() / 2));
    }
    
    SECTION("Incremental hashing produces same result") {
        // Direct hash
        auto hash1 = Botan::HashFunction::create("SHA-256");
        hash1->update("Hello, World!");
        auto result1 = hash1->final();
        
        // Incremental hash
        auto hash2 = Botan::HashFunction::create("SHA-256");
        hash2->update("Hello, ");
        hash2->update("World!");
        auto result2 = hash2->final();
        
        REQUIRE(result1 == result2);
    }
}

// ===========================================
// Legacy Hash Warnings (MD5, SHA-1)
// ===========================================
TEST_CASE("Legacy Hash Functions", "[hash][legacy][warning]") {
    // These tests exist to show that while MD5/SHA-1 work,
    // they should NOT be used for security purposes
    
    SECTION("MD5 computes correctly (DO NOT USE FOR SECURITY)") {
        // MD5("") = d41d8cd98f00b204e9800998ecf8427e
        std::string result = compute_hash("MD5", "");
        REQUIRE(result == "d41d8cd98f00b204e9800998ecf8427e");
    }
    
    SECTION("SHA-1 computes correctly (DO NOT USE FOR SECURITY)") {
        // SHA-1("abc") = a9993e364706816aba3e25717850c26c9cd0d89d
        std::string result = compute_hash("SHA-1", "abc");
        REQUIRE(result == "a9993e364706816aba3e25717850c26c9cd0d89d");
    }
}
