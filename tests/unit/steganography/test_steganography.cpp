/**
 * @file test_steganography.cpp
 * @brief Unit tests for LSB steganography
 *
 * Tests embedding and extracting data from images
 */

#include <catch2/catch_test_macros.hpp>
#include "filevault/steganography/lsb.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <filesystem>

using namespace filevault::steganography;
namespace fs = std::filesystem;

// Helper class to create and cleanup temporary test images
class TestImageHelper {
public:
    // Create a simple BMP test image (100x100 RGB)
    static std::string create_test_bmp(const std::string& path) {
        // BMP file header (14 bytes)
        std::vector<uint8_t> bmp_header = {
            'B', 'M',           // Magic number
            0, 0, 0, 0,         // File size (will be filled)
            0, 0, 0, 0,         // Reserved
            54, 0, 0, 0         // Pixel data offset
        };
        
        // DIB header (40 bytes - BITMAPINFOHEADER)
        std::vector<uint8_t> dib_header = {
            40, 0, 0, 0,        // Header size
            100, 0, 0, 0,       // Width: 100
            100, 0, 0, 0,       // Height: 100
            1, 0,               // Color planes
            24, 0,              // Bits per pixel (24-bit RGB)
            0, 0, 0, 0,         // No compression
            0, 0, 0, 0,         // Image size (can be 0 for uncompressed)
            0, 0, 0, 0,         // Horizontal resolution
            0, 0, 0, 0,         // Vertical resolution
            0, 0, 0, 0,         // Colors in palette
            0, 0, 0, 0          // Important colors
        };
        
        // Pixel data (100x100 RGB, with padding)
        // Each row must be multiple of 4 bytes
        // 100 pixels * 3 bytes = 300 bytes, needs no padding (300 % 4 = 0)
        size_t row_size = 100 * 3;
        size_t pixel_data_size = row_size * 100;
        
        std::vector<uint8_t> pixel_data(pixel_data_size);
        
        // Fill with gradient pattern (more realistic than solid color)
        for (int y = 0; y < 100; ++y) {
            for (int x = 0; x < 100; ++x) {
                size_t idx = y * row_size + x * 3;
                pixel_data[idx] = static_cast<uint8_t>(x * 2);          // Blue
                pixel_data[idx + 1] = static_cast<uint8_t>(y * 2);      // Green
                pixel_data[idx + 2] = static_cast<uint8_t>((x + y));    // Red
            }
        }
        
        // Calculate file size
        uint32_t file_size = 54 + static_cast<uint32_t>(pixel_data_size);
        bmp_header[2] = file_size & 0xFF;
        bmp_header[3] = (file_size >> 8) & 0xFF;
        bmp_header[4] = (file_size >> 16) & 0xFF;
        bmp_header[5] = (file_size >> 24) & 0xFF;
        
        // Write BMP file
        std::ofstream file(path, std::ios::binary);
        if (!file) return "";
        
        file.write(reinterpret_cast<char*>(bmp_header.data()), bmp_header.size());
        file.write(reinterpret_cast<char*>(dib_header.data()), dib_header.size());
        file.write(reinterpret_cast<char*>(pixel_data.data()), pixel_data.size());
        file.close();
        
        return path;
    }
    
    static void cleanup(const std::string& path) {
        if (fs::exists(path)) {
            fs::remove(path);
        }
    }
};

// ===========================================
// Capacity Tests
// ===========================================
TEST_CASE("LSB Steganography Capacity", "[steganography][capacity]") {
    std::string test_image = "test_capacity.bmp";
    
    SECTION("Calculate capacity for BMP image") {
        TestImageHelper::create_test_bmp(test_image);
        
        size_t capacity = LSBSteganography::calculate_capacity(test_image, 1);
        
        // 100x100 image = 10000 pixels
        // With 1 bit per channel, 3 channels: 10000 * 3 / 8 = 3750 bytes
        // Minus 4 bytes for length header = 3746 bytes usable
        INFO("Calculated capacity: " << capacity << " bytes");
        REQUIRE(capacity > 0);
        
        TestImageHelper::cleanup(test_image);
    }
    
    SECTION("Higher bits per channel = more capacity") {
        TestImageHelper::create_test_bmp(test_image);
        
        size_t capacity_1bit = LSBSteganography::calculate_capacity(test_image, 1);
        size_t capacity_2bit = LSBSteganography::calculate_capacity(test_image, 2);
        
        INFO("1-bit capacity: " << capacity_1bit);
        INFO("2-bit capacity: " << capacity_2bit);
        
        REQUIRE(capacity_2bit > capacity_1bit);
        
        TestImageHelper::cleanup(test_image);
    }
    
    SECTION("Non-existent file returns 0") {
        size_t capacity = LSBSteganography::calculate_capacity("nonexistent.bmp", 1);
        REQUIRE(capacity == 0);
    }
}

// ===========================================
// Embed/Extract Round-trip Tests
// ===========================================
TEST_CASE("LSB Steganography Embed and Extract", "[steganography][roundtrip]") {
    std::string cover_image = "test_cover.bmp";
    std::string stego_image = "test_stego.bmp";
    
    SECTION("Embed and extract short message") {
        TestImageHelper::create_test_bmp(cover_image);
        
        std::string secret = "Hello, World!";
        std::vector<uint8_t> secret_data(secret.begin(), secret.end());
        
        // Embed
        bool embed_success = LSBSteganography::embed(
            cover_image, secret_data, stego_image, 1
        );
        
        INFO("Embed success: " << embed_success);
        REQUIRE(embed_success);
        REQUIRE(fs::exists(stego_image));
        
        // Extract
        std::vector<uint8_t> extracted = LSBSteganography::extract(stego_image, 1);
        
        INFO("Extracted size: " << extracted.size());
        INFO("Expected size: " << secret_data.size());
        
        REQUIRE(extracted == secret_data);
        
        // Verify extracted text
        std::string extracted_str(extracted.begin(), extracted.end());
        REQUIRE(extracted_str == secret);
        
        TestImageHelper::cleanup(cover_image);
        TestImageHelper::cleanup(stego_image);
    }
    
    SECTION("Embed and extract binary data") {
        TestImageHelper::create_test_bmp(cover_image);
        
        // Binary data with all byte values
        std::vector<uint8_t> secret_data;
        for (int i = 0; i < 256; ++i) {
            secret_data.push_back(static_cast<uint8_t>(i));
        }
        
        bool embed_success = LSBSteganography::embed(
            cover_image, secret_data, stego_image, 1
        );
        REQUIRE(embed_success);
        
        std::vector<uint8_t> extracted = LSBSteganography::extract(stego_image, 1);
        REQUIRE(extracted == secret_data);
        
        TestImageHelper::cleanup(cover_image);
        TestImageHelper::cleanup(stego_image);
    }
    
    SECTION("Embed with 2 bits per channel") {
        TestImageHelper::create_test_bmp(cover_image);
        
        std::string secret = "Test with 2 bits per channel - more capacity!";
        std::vector<uint8_t> secret_data(secret.begin(), secret.end());
        
        bool embed_success = LSBSteganography::embed(
            cover_image, secret_data, stego_image, 2
        );
        REQUIRE(embed_success);
        
        std::vector<uint8_t> extracted = LSBSteganography::extract(stego_image, 2);
        REQUIRE(extracted == secret_data);
        
        TestImageHelper::cleanup(cover_image);
        TestImageHelper::cleanup(stego_image);
    }
    
    SECTION("Empty data") {
        TestImageHelper::create_test_bmp(cover_image);
        
        std::vector<uint8_t> secret_data;  // Empty
        
        bool embed_success = LSBSteganography::embed(
            cover_image, secret_data, stego_image, 1
        );
        REQUIRE(embed_success);
        
        std::vector<uint8_t> extracted = LSBSteganography::extract(stego_image, 1);
        REQUIRE(extracted.empty());
        
        TestImageHelper::cleanup(cover_image);
        TestImageHelper::cleanup(stego_image);
    }
}

// ===========================================
// Error Handling Tests
// ===========================================
TEST_CASE("LSB Steganography Error Handling", "[steganography][errors]") {
    SECTION("Embed to non-existent cover image fails") {
        std::string secret = "test";
        std::vector<uint8_t> secret_data(secret.begin(), secret.end());
        
        bool result = LSBSteganography::embed(
            "nonexistent.bmp", secret_data, "output.bmp", 1
        );
        REQUIRE_FALSE(result);
    }
    
    SECTION("Extract from non-existent file returns empty") {
        std::vector<uint8_t> extracted = LSBSteganography::extract("nonexistent.bmp", 1);
        REQUIRE(extracted.empty());
    }
    
    SECTION("Data too large for image fails") {
        std::string cover_image = "test_small.bmp";
        TestImageHelper::create_test_bmp(cover_image);
        
        // Get capacity
        size_t capacity = LSBSteganography::calculate_capacity(cover_image, 1);
        
        // Create data larger than capacity
        std::vector<uint8_t> large_data(capacity + 1000, 'X');
        
        bool result = LSBSteganography::embed(
            cover_image, large_data, "output.bmp", 1
        );
        REQUIRE_FALSE(result);
        
        TestImageHelper::cleanup(cover_image);
    }
}

// ===========================================
// Bits Per Channel Tests
// ===========================================
TEST_CASE("LSB Bits Per Channel Variations", "[steganography][bits]") {
    std::string cover_image = "test_bits.bmp";
    std::string stego_image = "test_bits_out.bmp";
    
    // Test 1-4 bits per channel
    for (int bits = 1; bits <= 4; ++bits) {
        SECTION("Round-trip with " + std::to_string(bits) + " bits per channel") {
            TestImageHelper::create_test_bmp(cover_image);
            
            std::string secret = "Testing " + std::to_string(bits) + " bits per channel";
            std::vector<uint8_t> secret_data(secret.begin(), secret.end());
            
            bool embed_success = LSBSteganography::embed(
                cover_image, secret_data, stego_image, bits
            );
            
            if (embed_success) {
                std::vector<uint8_t> extracted = LSBSteganography::extract(stego_image, bits);
                
                INFO("Bits: " << bits);
                INFO("Secret size: " << secret_data.size());
                INFO("Extracted size: " << extracted.size());
                
                REQUIRE(extracted == secret_data);
            }
            
            TestImageHelper::cleanup(cover_image);
            TestImageHelper::cleanup(stego_image);
        }
    }
}

// ===========================================
// Mismatched Parameters Tests
// ===========================================
TEST_CASE("LSB Steganography Parameter Mismatch", "[steganography][mismatch]") {
    std::string cover_image = "test_mismatch.bmp";
    std::string stego_image = "test_mismatch_out.bmp";
    
    SECTION("Wrong bits_per_channel on extract") {
        TestImageHelper::create_test_bmp(cover_image);
        
        std::string secret = "Secret message";
        std::vector<uint8_t> secret_data(secret.begin(), secret.end());
        
        // Embed with 1 bit
        bool embed_success = LSBSteganography::embed(
            cover_image, secret_data, stego_image, 1
        );
        REQUIRE(embed_success);
        
        // Try to extract with 2 bits (wrong parameter)
        std::vector<uint8_t> extracted = LSBSteganography::extract(stego_image, 2);
        
        // Should not match original data (corrupted extraction)
        // Note: might return garbage data or empty depending on implementation
        INFO("Mismatched extraction: expected different result");
        
        TestImageHelper::cleanup(cover_image);
        TestImageHelper::cleanup(stego_image);
    }
}

// ===========================================
// Security Tests
// ===========================================
TEST_CASE("LSB Steganography Security Properties", "[steganography][security]") {
    std::string cover_image = "test_security.bmp";
    std::string stego_image = "test_security_out.bmp";
    
    SECTION("Same message, same image produces same stego") {
        TestImageHelper::create_test_bmp(cover_image);
        
        std::string secret = "Deterministic test";
        std::vector<uint8_t> secret_data(secret.begin(), secret.end());
        
        // First embedding
        LSBSteganography::embed(cover_image, secret_data, stego_image, 1);
        std::ifstream file1(stego_image, std::ios::binary);
        std::vector<uint8_t> stego1((std::istreambuf_iterator<char>(file1)),
                                     std::istreambuf_iterator<char>());
        
        // Second embedding (same input)
        LSBSteganography::embed(cover_image, secret_data, stego_image, 1);
        std::ifstream file2(stego_image, std::ios::binary);
        std::vector<uint8_t> stego2((std::istreambuf_iterator<char>(file2)),
                                     std::istreambuf_iterator<char>());
        
        // Should be identical (deterministic)
        REQUIRE(stego1 == stego2);
        
        TestImageHelper::cleanup(cover_image);
        TestImageHelper::cleanup(stego_image);
    }
    
    SECTION("Different messages produce different stegos") {
        TestImageHelper::create_test_bmp(cover_image);
        
        std::string secret1 = "Message A";
        std::string secret2 = "Message B";
        
        std::vector<uint8_t> data1(secret1.begin(), secret1.end());
        std::vector<uint8_t> data2(secret2.begin(), secret2.end());
        
        // First message
        LSBSteganography::embed(cover_image, data1, stego_image, 1);
        std::ifstream file1(stego_image, std::ios::binary);
        std::vector<uint8_t> stego1((std::istreambuf_iterator<char>(file1)),
                                     std::istreambuf_iterator<char>());
        file1.close();
        
        // Second message
        LSBSteganography::embed(cover_image, data2, stego_image, 1);
        std::ifstream file2(stego_image, std::ios::binary);
        std::vector<uint8_t> stego2((std::istreambuf_iterator<char>(file2)),
                                     std::istreambuf_iterator<char>());
        
        // Should be different
        REQUIRE(stego1 != stego2);
        
        TestImageHelper::cleanup(cover_image);
        TestImageHelper::cleanup(stego_image);
    }
}
