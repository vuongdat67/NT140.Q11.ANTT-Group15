#include "filevault/compression/compressor.hpp"
#include <zlib.h>
// #include <bzlib.h>  // TODO: Replace with bzip3 API
#include <lzma.h>
#include <chrono>
#include <stdexcept>
#include <fmt/core.h>

namespace filevault {
namespace compression {

// ============================================================================
// CompressionService
// ============================================================================

std::unique_ptr<ICompressor> CompressionService::create(core::CompressionType type) {
    switch (type) {
        case core::CompressionType::ZLIB:
            return std::make_unique<ZlibCompressor>();
        case core::CompressionType::BZIP2:
            // TODO: Replace with bzip3 API (currently using bzip2 which isn't linked)
            throw std::runtime_error("bzip2 support temporarily disabled - use zlib or lzma");
        case core::CompressionType::LZMA:
            return std::make_unique<LzmaCompressor>();
        case core::CompressionType::NONE:
            throw std::invalid_argument("Cannot create compressor for NONE type");
        default:
            throw std::invalid_argument("Unknown compression type");
    }
}

std::string CompressionService::get_algorithm_name(core::CompressionType type) {
    switch (type) {
        case core::CompressionType::NONE: return "none";
        case core::CompressionType::ZLIB: return "zlib";
        case core::CompressionType::BZIP2: return "bzip2";
        case core::CompressionType::LZMA: return "lzma";
        default: return "unknown";
    }
}

core::CompressionType CompressionService::parse_algorithm(const std::string& name) {
    if (name == "none") return core::CompressionType::NONE;
    if (name == "zlib") return core::CompressionType::ZLIB;
    if (name == "bzip2" || name == "bz2") return core::CompressionType::BZIP2;
    if (name == "lzma" || name == "xz") return core::CompressionType::LZMA;
    
    throw std::invalid_argument("Unknown compression algorithm: " + name);
}

// ============================================================================
// ZlibCompressor
// ============================================================================

CompressionResult ZlibCompressor::compress(
    std::span<const uint8_t> input,
    int level
) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Clamp level to valid range
        level = std::clamp(level, 1, 9);
        
        // Allocate output buffer (worst case: original size + 0.1% + 12 bytes)
        uLongf dest_len = compressBound(input.size());
        result.data.resize(dest_len);
        
        // Compress
        int ret = ::compress2(
            result.data.data(),
            &dest_len,
            input.data(),
            input.size(),
            level
        );
        
        if (ret != Z_OK) {
            result.success = false;
            result.error_message = fmt::format("zlib compression failed: error {}", ret);
            return result;
        }
        
        // Resize to actual size
        result.data.resize(dest_len);
        
        result.success = true;
        result.original_size = input.size();
        result.compressed_size = dest_len;
        result.compression_ratio = 100.0 * (1.0 - static_cast<double>(dest_len) / input.size());
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

CompressionResult ZlibCompressor::decompress(std::span<const uint8_t> input) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Start with 4x size, grow if needed
        uLongf dest_len = input.size() * 4;
        result.data.resize(dest_len);
        
        int ret = Z_BUF_ERROR;
        int attempts = 0;
        
        // Try decompression, growing buffer if needed
        while (ret == Z_BUF_ERROR && attempts < 10) {
            dest_len = result.data.size();
            ret = ::uncompress(
                result.data.data(),
                &dest_len,
                input.data(),
                input.size()
            );
            
            if (ret == Z_BUF_ERROR) {
                result.data.resize(result.data.size() * 2);
                attempts++;
            }
        }
        
        if (ret != Z_OK) {
            result.success = false;
            result.error_message = fmt::format("zlib decompression failed: error {}", ret);
            return result;
        }
        
        result.data.resize(dest_len);
        
        result.success = true;
        result.original_size = input.size();
        result.compressed_size = dest_len;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

// ============================================================================
// Bzip2Compressor - TEMPORARILY DISABLED (need bzip3 API, not bzip2)
// ============================================================================

// TODO: Implement using bzip3 API instead of bzip2
CompressionResult Bzip2Compressor::compress(
    std::span<const uint8_t> input,
    int level
) {
    (void)input;
    (void)level;
    CompressionResult result;
    result.success = false;
    result.error_message = "bzip2 support temporarily disabled - use zlib or lzma";
    return result;
}

CompressionResult Bzip2Compressor::decompress(std::span<const uint8_t> input) {
    (void)input;
    CompressionResult result;
    result.success = false;
    result.error_message = "bzip2 support temporarily disabled - use zlib or lzma";
    return result;
    
    return result;
}

// ============================================================================
// LzmaCompressor
// ============================================================================

CompressionResult LzmaCompressor::compress(
    std::span<const uint8_t> input,
    int level
) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        level = std::clamp(level, 1, 9);
        
        // LZMA properties
        lzma_stream strm = LZMA_STREAM_INIT;
        
        // Initialize encoder
        lzma_ret ret = lzma_easy_encoder(&strm, level, LZMA_CHECK_CRC64);
        if (ret != LZMA_OK) {
            result.success = false;
            result.error_message = "LZMA encoder initialization failed";
            return result;
        }
        
        // Allocate output buffer
        size_t out_size = lzma_stream_buffer_bound(input.size());
        result.data.resize(out_size);
        
        // Setup streams
        strm.next_in = input.data();
        strm.avail_in = input.size();
        strm.next_out = result.data.data();
        strm.avail_out = result.data.size();
        
        // Compress
        ret = lzma_code(&strm, LZMA_FINISH);
        
        if (ret != LZMA_STREAM_END) {
            lzma_end(&strm);
            result.success = false;
            result.error_message = fmt::format("LZMA compression failed: error {}", static_cast<int>(ret));
            return result;
        }
        
        size_t compressed_size = strm.total_out;
        lzma_end(&strm);
        
        result.data.resize(compressed_size);
        
        result.success = true;
        result.original_size = input.size();
        result.compressed_size = compressed_size;
        result.compression_ratio = 100.0 * (1.0 - static_cast<double>(compressed_size) / input.size());
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

CompressionResult LzmaCompressor::decompress(std::span<const uint8_t> input) {
    CompressionResult result;
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        lzma_stream strm = LZMA_STREAM_INIT;
        
        // Initialize decoder
        lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
        if (ret != LZMA_OK) {
            result.success = false;
            result.error_message = "LZMA decoder initialization failed";
            return result;
        }
        
        // Allocate output buffer
        result.data.resize(input.size() * 4);
        
        strm.next_in = input.data();
        strm.avail_in = input.size();
        strm.next_out = result.data.data();
        strm.avail_out = result.data.size();
        
        // Decompress, growing buffer if needed
        while (true) {
            ret = lzma_code(&strm, LZMA_RUN);
            
            if (ret == LZMA_STREAM_END) {
                break;
            }
            
            if (ret == LZMA_OK && strm.avail_out == 0) {
                // Need more output space
                size_t old_size = result.data.size();
                result.data.resize(old_size * 2);
                strm.next_out = result.data.data() + old_size;
                strm.avail_out = old_size;
                continue;
            }
            
            if (ret != LZMA_OK) {
                lzma_end(&strm);
                result.success = false;
                result.error_message = fmt::format("LZMA decompression failed: error {}", static_cast<int>(ret));
                return result;
            }
        }
        
        size_t decompressed_size = strm.total_out;
        lzma_end(&strm);
        
        result.data.resize(decompressed_size);
        
        result.success = true;
        result.original_size = input.size();
        result.compressed_size = decompressed_size;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

} // namespace compression
} // namespace filevault
