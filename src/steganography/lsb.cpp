#include "filevault/steganography/lsb.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG  // Enable user-friendly error messages
#include <stb_image.h>
#include <stb_image_write.h>

namespace filevault::steganography {

bool LSBSteganography::embed(
    const std::string& cover_image_path,
    std::span<const uint8_t> secret_data,
    const std::string& output_path,
    int bits_per_channel
) {
    if (bits_per_channel < 1 || bits_per_channel > 4) {
        return false;
    }
    
    // Load image
    int width, height, channels;
    unsigned char* image_data = stbi_load(cover_image_path.c_str(), &width, &height, &channels, 0);
    
    if (!image_data) {
        // stbi_failure_reason() provides helpful error message
        return false;
    }
    
    // Check capacity
    size_t pixel_count = width * height * channels;
    size_t max_bytes = (pixel_count * bits_per_channel) / 8;
    size_t required_bytes = LENGTH_HEADER_SIZE + secret_data.size();
    
    if (required_bytes > max_bytes) {
        stbi_image_free(image_data);
        return false;
    }
    
    // Embed length header (4 bytes)
    size_t bit_index = 0;
    uint32_t data_length = static_cast<uint32_t>(secret_data.size());
    
    for (int i = 0; i < 4; ++i) {
        uint8_t byte = (data_length >> (i * 8)) & 0xFF;
        embed_byte(image_data, pixel_count, bit_index, byte, bits_per_channel);
    }
    
    // Embed secret data
    for (uint8_t byte : secret_data) {
        embed_byte(image_data, pixel_count, bit_index, byte, bits_per_channel);
    }
    
    // Save stego image
    bool success = false;
    
    // Determine format from extension
    std::string ext = output_path.substr(output_path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "png") {
        success = stbi_write_png(output_path.c_str(), width, height, channels, image_data, width * channels);
    } else if (ext == "bmp") {
        success = stbi_write_bmp(output_path.c_str(), width, height, channels, image_data);
    } else {
        // Default to PNG
        success = stbi_write_png(output_path.c_str(), width, height, channels, image_data, width * channels);
    }
    
    stbi_image_free(image_data);
    return success;
}

std::vector<uint8_t> LSBSteganography::extract(
    const std::string& stego_image_path,
    int bits_per_channel
) {
    if (bits_per_channel < 1 || bits_per_channel > 4) {
        return {};
    }
    
    // Load image
    int width, height, channels;
    unsigned char* image_data = stbi_load(stego_image_path.c_str(), &width, &height, &channels, 0);
    
    if (!image_data) {
        // stbi_failure_reason() provides helpful error message
        return {};
    }
    
    size_t pixel_count = width * height * channels;
    size_t bit_index = 0;
    
    // Extract length header (4 bytes)
    uint32_t data_length = 0;
    for (int i = 0; i < 4; ++i) {
        uint8_t byte = extract_byte(image_data, pixel_count, bit_index, bits_per_channel);
        data_length |= (static_cast<uint32_t>(byte) << (i * 8));
    }
    
    // Validate length
    size_t max_bytes = (pixel_count * bits_per_channel) / 8 - LENGTH_HEADER_SIZE;
    if (data_length == 0 || data_length > max_bytes) {
        stbi_image_free(image_data);
        return {};
    }
    
    // Extract secret data
    std::vector<uint8_t> secret_data(data_length);
    for (size_t i = 0; i < data_length; ++i) {
        secret_data[i] = extract_byte(image_data, pixel_count, bit_index, bits_per_channel);
    }
    
    stbi_image_free(image_data);
    return secret_data;
}

size_t LSBSteganography::calculate_capacity(
    const std::string& image_path,
    int bits_per_channel
) {
    if (bits_per_channel < 1 || bits_per_channel > 4) {
        return 0;
    }
    
    // Get image dimensions without loading full image data (faster)
    int width, height, channels;
    int info_result = stbi_info(image_path.c_str(), &width, &height, &channels);
    
    if (info_result == 0) {
        // Fallback: try loading the full image
        unsigned char* image_data = stbi_load(image_path.c_str(), &width, &height, &channels, 0);
        
        if (!image_data) {
            return 0;
        }
        
        stbi_image_free(image_data);
    }
    
    size_t pixel_count = width * height * channels;
    size_t max_bytes = (pixel_count * bits_per_channel) / 8;
    
    // Subtract header size
    return (max_bytes > LENGTH_HEADER_SIZE) ? (max_bytes - LENGTH_HEADER_SIZE) : 0;
}

void LSBSteganography::embed_byte(
    uint8_t* pixel_data,
    size_t pixel_count,
    size_t& bit_index,
    uint8_t byte,
    int bits_per_channel
) {
    uint8_t mask = (1 << bits_per_channel) - 1;  // e.g., 0b00000001 for 1 bit
    
    for (int bit_pos = 0; bit_pos < 8; bit_pos += bits_per_channel) {
        if (bit_index >= pixel_count) {
            return;  // Out of space
        }
        
        // Extract bits from byte
        uint8_t bits = (byte >> bit_pos) & mask;
        
        // Clear LSBs in pixel
        pixel_data[bit_index] = (pixel_data[bit_index] & ~mask) | bits;
        
        bit_index++;
    }
}

uint8_t LSBSteganography::extract_byte(
    const uint8_t* pixel_data,
    size_t pixel_count,
    size_t& bit_index,
    int bits_per_channel
) {
    uint8_t byte = 0;
    uint8_t mask = (1 << bits_per_channel) - 1;
    
    for (int bit_pos = 0; bit_pos < 8; bit_pos += bits_per_channel) {
        if (bit_index >= pixel_count) {
            return byte;  // Out of data
        }
        
        // Extract LSBs from pixel
        uint8_t bits = pixel_data[bit_index] & mask;
        
        // Place bits in byte
        byte |= (bits << bit_pos);
        
        bit_index++;
    }
    
    return byte;
}

} // namespace filevault::steganography
