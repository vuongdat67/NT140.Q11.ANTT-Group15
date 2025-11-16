#include "filevault/compression/compressor.hpp"
#include "filevault/exceptions.hpp"
#include <zlib.h>
#include <cstring>
#include <iostream>

namespace filevault::compression {

// ============================================================================
// ZlibCompressor Implementation
// ============================================================================

ZlibCompressor::ZlibCompressor(int level) : level_(level) {
    // Validate compression level (1-9)
    if (level_ < 1 || level_ > 9) {
        level_ = Z_DEFAULT_COMPRESSION;  // -1 means default (6)
    }
}

Bytes ZlibCompressor::compress(const Bytes& input) {
    if (input.empty()) {
        return Bytes();
    }
    
    try {
        // Calculate maximum compressed size (worst case: slightly larger than input)
        uLongf compressed_size = compressBound(static_cast<uLong>(input.size()));
        Bytes output(compressed_size);
        
        // Compress using zlib's compress2
        int result = compress2(
            output.data(),
            &compressed_size,
            input.data(),
            static_cast<uLong>(input.size()),
            level_
        );
        
        // Handle errors
        switch (result) {
            case Z_OK:
                // Success - resize to actual compressed size
                output.resize(compressed_size);
                return output;
                
            case Z_MEM_ERROR:
                throw CompressionException("Zlib compression failed: out of memory");
                
            case Z_BUF_ERROR:
                throw CompressionException("Zlib compression failed: output buffer too small");
                
            default:
                throw CompressionException("Zlib compression failed with error code: " + std::to_string(result));
        }
        
    } catch (const std::bad_alloc& e) {
        throw CompressionException(std::string("Zlib compression failed: memory allocation error - ") + e.what());
    }
}

Bytes ZlibCompressor::decompress(const Bytes& input) {
    if (input.empty()) {
        return Bytes();
    }
    
    try {
        // Use zlib stream API for more robust decompression
        z_stream stream;
        std::memset(&stream, 0, sizeof(stream));
        
        // Initialize decompression
        int result = inflateInit(&stream);
        if (result != Z_OK) {
            throw CompressionException("Zlib inflateInit failed: " + std::to_string(result));
        }
        
        // Set input
        stream.next_in = const_cast<Bytef*>(input.data());
        stream.avail_in = static_cast<uInt>(input.size());
        
        // Start with reasonable output buffer
        Bytes output;
        output.reserve(input.size() * 4);  // Initial guess
        
        const size_t CHUNK_SIZE = 16384;  // 16KB chunks
        Bytes chunk(CHUNK_SIZE);
        
        // Decompress in chunks
        do {
            stream.next_out = chunk.data();
            stream.avail_out = static_cast<uInt>(CHUNK_SIZE);
            
            result = inflate(&stream, Z_NO_FLUSH);
            
            if (result == Z_STREAM_ERROR || result == Z_NEED_DICT || 
                result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
                inflateEnd(&stream);
                
                if (result == Z_DATA_ERROR) {
                    throw CompressionException("Zlib decompression failed: corrupted or invalid compressed data");
                } else if (result == Z_MEM_ERROR) {
                    throw CompressionException("Zlib decompression failed: out of memory");
                } else {
                    throw CompressionException("Zlib decompression failed with error: " + std::to_string(result));
                }
            }
            
            // Append decompressed chunk
            size_t have = CHUNK_SIZE - stream.avail_out;
            output.insert(output.end(), chunk.begin(), chunk.begin() + have);
            
        } while (result != Z_STREAM_END);
        
        // Clean up
        inflateEnd(&stream);
        return output;
        
    } catch (const std::bad_alloc& e) {
        throw CompressionException(std::string("Zlib decompression failed: memory allocation error - ") + e.what());
    } catch (const CompressionException&) {
        // Re-throw our own exceptions
        throw;
    }
}

// ============================================================================
// ZstdCompressor Implementation (using Botan)
// ============================================================================

ZstdCompressor::ZstdCompressor(int level) : level_(level) {
    // Validate compression level (1-22)
    if (level_ < 1 || level_ > 22) {
        level_ = 3;  // Default zstd level
    }
}

Bytes ZstdCompressor::compress(const Bytes& input) {
    if (input.empty()) {
        return Bytes();
    }
    
    // TODO: Add real Zstd library support
    // For now, use Zlib as a working fallback to prevent crashes
    
    try {
        // Show warning once that we're using Zlib fallback
        static bool warning_shown = false;
        if (!warning_shown) {
            std::cerr << "[INFO] Zstd compression using Zlib fallback (add zstd library for native support)\n";
            warning_shown = true;
        }
        
        // Use Zlib compression with appropriate level
        int zlib_level = (level_ > 9) ? 9 : level_;  // Zlib max level is 9
        
        uLongf compressed_size = compressBound(static_cast<uLong>(input.size()));
        Bytes output(compressed_size);
        
        int result = compress2(
            output.data(),
            &compressed_size,
            input.data(),
            static_cast<uLong>(input.size()),
            zlib_level
        );
        
        if (result != Z_OK) {
            throw CompressionException("Zstd fallback (Zlib) compression failed");
        }
        
        output.resize(compressed_size);
        return output;
        
    } catch (const std::bad_alloc& e) {
        throw CompressionException(std::string("Zstd compression: out of memory - ") + e.what());
    }
}

Bytes ZstdCompressor::decompress(const Bytes& input) {
    if (input.empty()) {
        return Bytes();
    }
    
    // TODO: Add real Zstd library support
    // For now, use Zlib as a working fallback (compatible with Zlib-compressed data)
    
    try {
        // Use zlib stream API for decompression (same as ZlibCompressor)
        z_stream stream;
        std::memset(&stream, 0, sizeof(stream));
        
        int result = inflateInit(&stream);
        if (result != Z_OK) {
            throw CompressionException("Zstd fallback (Zlib) inflateInit failed");
        }
        
        stream.next_in = const_cast<Bytef*>(input.data());
        stream.avail_in = static_cast<uInt>(input.size());
        
        Bytes output;
        output.reserve(input.size() * 4);
        
        const size_t CHUNK_SIZE = 16384;
        Bytes chunk(CHUNK_SIZE);
        
        do {
            stream.next_out = chunk.data();
            stream.avail_out = static_cast<uInt>(CHUNK_SIZE);
            
            result = inflate(&stream, Z_NO_FLUSH);
            
            if (result == Z_STREAM_ERROR || result == Z_NEED_DICT ||
                result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
                inflateEnd(&stream);
                throw CompressionException("Zstd fallback (Zlib) decompression failed");
            }
            
            size_t have = CHUNK_SIZE - stream.avail_out;
            output.insert(output.end(), chunk.begin(), chunk.begin() + have);
            
        } while (result != Z_STREAM_END);
        
        inflateEnd(&stream);
        return output;
        
    } catch (const std::bad_alloc& e) {
        throw CompressionException(std::string("Zstd decompression: out of memory - ") + e.what());
    } catch (const CompressionException&) {
        throw;
    }
}

// ============================================================================
// CompressorFactory Implementation
// ============================================================================

std::unique_ptr<ICompressor> CompressorFactory::create(CompressionType type, int level) {
    switch (type) {
        case CompressionType::NONE:
            return std::make_unique<NullCompressor>();
            
        case CompressionType::ZLIB:
            if (level == -1) level = 6;  // Default zlib level
            return std::make_unique<ZlibCompressor>(level);
            
        case CompressionType::ZSTD:
            if (level == -1) level = 3;  // Default zstd level
            return std::make_unique<ZstdCompressor>(level);
            
        default:
            throw InvalidArgumentException("Unsupported compression type");
    }
}

} // namespace filevault::compression
