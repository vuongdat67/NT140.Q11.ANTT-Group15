#ifndef FILEVAULT_FORMAT_FILE_HEADER_HPP
#define FILEVAULT_FORMAT_FILE_HEADER_HPP

#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <span>
#include "filevault/core/types.hpp"
#include "filevault/core/result.hpp"

namespace filevault {
namespace format {

/**
 * @brief FileVault encrypted file format
 * 
 * Structure:
 * - Magic bytes (4): "FVLT"
 * - Version (2): major.minor
 * - Algorithm (1): AlgorithmType enum
 * - KDF (1): KDFType enum
 * - Security level (1): SecurityLevel enum
 * - Salt length (2): uint16_t
 * - Salt (variable)
 * - Nonce length (2): uint16_t  
 * - Nonce (variable)
 * - Tag length (2): uint16_t
 * - Tag (variable, for AEAD)
 * - Original size (8): uint64_t
 * - Encrypted size (8): uint64_t
 * - Timestamp (8): uint64_t (Unix epoch)
 * - Flags (4): compression, etc.
 * - Reserved (16): future use
 * - Encrypted data (variable)
 */
class FileHeader {
public:
    static constexpr uint32_t MAGIC = 0x544C5646; // "FVLT" in little-endian
    static constexpr uint8_t VERSION_MAJOR = 1;
    static constexpr uint8_t VERSION_MINOR = 0;
    static constexpr size_t MIN_HEADER_SIZE = 64;
    
    // Flags
    static constexpr uint32_t FLAG_COMPRESSED = 0x00000001;
    static constexpr uint32_t FLAG_METADATA   = 0x00000002;
    
    FileHeader();
    
    // Setters
    void set_algorithm(core::AlgorithmType algo) { algorithm_ = algo; }
    void set_kdf(core::KDFType kdf) { kdf_ = kdf; }
    void set_security_level(core::SecurityLevel level) { security_level_ = level; }
    void set_salt(const std::vector<uint8_t>& salt) { salt_ = salt; }
    void set_nonce(const std::vector<uint8_t>& nonce) { nonce_ = nonce; }
    void set_tag(const std::vector<uint8_t>& tag) { tag_ = tag; }
    void set_original_size(uint64_t size) { original_size_ = size; }
    void set_encrypted_size(uint64_t size) { encrypted_size_ = size; }
    void set_timestamp(uint64_t ts) { timestamp_ = ts; }
    void set_compressed(bool compressed);
    
    // Getters
    core::AlgorithmType algorithm() const { return algorithm_; }
    core::KDFType kdf() const { return kdf_; }
    core::SecurityLevel security_level() const { return security_level_; }
    const std::vector<uint8_t>& salt() const { return salt_; }
    const std::vector<uint8_t>& nonce() const { return nonce_; }
    const std::vector<uint8_t>& tag() const { return tag_; }
    uint64_t original_size() const { return original_size_; }
    uint64_t encrypted_size() const { return encrypted_size_; }
    uint64_t timestamp() const { return timestamp_; }
    bool is_compressed() const { return (flags_ & FLAG_COMPRESSED) != 0; }
    uint32_t flags() const { return flags_; }
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static core::Result<FileHeader> deserialize(std::span<const uint8_t> data);
    
    // Validation
    bool validate() const;
    size_t total_size() const;

private:
    core::AlgorithmType algorithm_;
    core::KDFType kdf_;
    core::SecurityLevel security_level_;
    std::vector<uint8_t> salt_;
    std::vector<uint8_t> nonce_;
    std::vector<uint8_t> tag_;
    uint64_t original_size_;
    uint64_t encrypted_size_;
    uint64_t timestamp_;
    uint32_t flags_;
    std::array<uint8_t, 16> reserved_;
    
    // Helper functions
    static void write_uint16(std::vector<uint8_t>& buf, uint16_t val);
    static void write_uint32(std::vector<uint8_t>& buf, uint32_t val);
    static void write_uint64(std::vector<uint8_t>& buf, uint64_t val);
    static uint16_t read_uint16(std::span<const uint8_t> data, size_t& offset);
    static uint32_t read_uint32(std::span<const uint8_t> data, size_t& offset);
    static uint64_t read_uint64(std::span<const uint8_t> data, size_t& offset);
};

} // namespace format
} // namespace filevault

#endif // FILEVAULT_FORMAT_FILE_HEADER_HPP
