#ifndef FILEVAULT_COMPRESSION_COMPRESSOR_HPP
#define FILEVAULT_COMPRESSION_COMPRESSOR_HPP

#include "filevault/core/types.hpp"
#include "filevault/core/result.hpp"
#include <vector>
#include <span>
#include <memory>

namespace filevault {
namespace compression {

/**
 * @brief Compression result
 */
struct CompressionResult {
    bool success = false;
    std::string error_message;
    std::vector<uint8_t> data;
    size_t original_size = 0;
    size_t compressed_size = 0;
    double compression_ratio = 0.0;  // Percentage saved
    double processing_time_ms = 0.0;
};

/**
 * @brief Interface for compression algorithms
 */
class ICompressor {
public:
    virtual ~ICompressor() = default;
    
    /**
     * @brief Get compressor name
     */
    virtual std::string name() const = 0;
    
    /**
     * @brief Compress data
     * @param input Data to compress
     * @param level Compression level (1-9, algorithm-specific)
     * @return Compressed data
     */
    virtual CompressionResult compress(
        std::span<const uint8_t> input,
        int level = 6
    ) = 0;
    
    /**
     * @brief Decompress data
     * @param input Compressed data
     * @return Decompressed data
     */
    virtual CompressionResult decompress(
        std::span<const uint8_t> input
    ) = 0;
};

/**
 * @brief Compression service - factory for compressors
 */
class CompressionService {
public:
    /**
     * @brief Create compressor for algorithm
     */
    static std::unique_ptr<ICompressor> create(core::CompressionType type);
    
    /**
     * @brief Get algorithm name
     */
    static std::string get_algorithm_name(core::CompressionType type);
    
    /**
     * @brief Parse algorithm from string
     */
    static core::CompressionType parse_algorithm(const std::string& name);
};

/**
 * @brief ZLIB compressor (fast, good compression)
 */
class ZlibCompressor : public ICompressor {
public:
    std::string name() const override { return "zlib"; }
    
    CompressionResult compress(
        std::span<const uint8_t> input,
        int level = 6
    ) override;
    
    CompressionResult decompress(
        std::span<const uint8_t> input
    ) override;
};

/**
 * @brief BZIP2 compressor (better ratio, slower)
 */
class Bzip2Compressor : public ICompressor {
public:
    std::string name() const override { return "bzip2"; }
    
    CompressionResult compress(
        std::span<const uint8_t> input,
        int level = 6
    ) override;
    
    CompressionResult decompress(
        std::span<const uint8_t> input
    ) override;
};

/**
 * @brief LZMA compressor (maximum compression, slowest)
 */
class LzmaCompressor : public ICompressor {
public:
    std::string name() const override { return "lzma"; }
    
    CompressionResult compress(
        std::span<const uint8_t> input,
        int level = 6
    ) override;
    
    CompressionResult decompress(
        std::span<const uint8_t> input
    ) override;
};

} // namespace compression
} // namespace filevault

#endif // FILEVAULT_COMPRESSION_COMPRESSOR_HPP
