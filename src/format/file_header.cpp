#include "filevault/format/file_header.hpp"
#include <cstring>
#include <chrono>
#include <spdlog/spdlog.h>

namespace filevault {
namespace format {

FileHeader::FileHeader() 
    : algorithm_(core::AlgorithmType::AES_256_GCM)
    , kdf_(core::KDFType::ARGON2ID)
    , security_level_(core::SecurityLevel::MEDIUM)
    , original_size_(0)
    , encrypted_size_(0)
    , timestamp_(0)
    , flags_(0)
    , reserved_{} {
}

void FileHeader::set_compressed(bool compressed) {
    if (compressed) {
        flags_ |= FLAG_COMPRESSED;
    } else {
        flags_ &= ~FLAG_COMPRESSED;
    }
}

std::vector<uint8_t> FileHeader::serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(total_size());
    
    // Magic
    write_uint32(buffer, MAGIC);
    
    // Version
    buffer.push_back(VERSION_MAJOR);
    buffer.push_back(VERSION_MINOR);
    
    // Algorithm, KDF, Security
    buffer.push_back(static_cast<uint8_t>(algorithm_));
    buffer.push_back(static_cast<uint8_t>(kdf_));
    buffer.push_back(static_cast<uint8_t>(security_level_));
    
    // Salt
    write_uint16(buffer, static_cast<uint16_t>(salt_.size()));
    buffer.insert(buffer.end(), salt_.begin(), salt_.end());
    
    // Nonce
    write_uint16(buffer, static_cast<uint16_t>(nonce_.size()));
    buffer.insert(buffer.end(), nonce_.begin(), nonce_.end());
    
    // Tag
    write_uint16(buffer, static_cast<uint16_t>(tag_.size()));
    buffer.insert(buffer.end(), tag_.begin(), tag_.end());
    
    // Sizes
    write_uint64(buffer, original_size_);
    write_uint64(buffer, encrypted_size_);
    write_uint64(buffer, timestamp_);
    
    // Flags
    write_uint32(buffer, flags_);
    
    // Reserved
    buffer.insert(buffer.end(), reserved_.begin(), reserved_.end());
    
    return buffer;
}

core::Result<FileHeader> FileHeader::deserialize(std::span<const uint8_t> data) {
    if (data.size() < MIN_HEADER_SIZE) {
        return core::Result<FileHeader>::error("Header too small");
    }
    
    size_t offset = 0;
    FileHeader header;
    
    // Magic
    uint32_t magic = read_uint32(data, offset);
    if (magic != MAGIC) {
        return core::Result<FileHeader>::error("Invalid magic bytes");
    }
    
    // Version
    uint8_t major = data[offset++];
    [[maybe_unused]] uint8_t minor = data[offset++];
    if (major != VERSION_MAJOR) {
        return core::Result<FileHeader>::error("Unsupported version");
    }
    
    // Algorithm, KDF, Security
    header.algorithm_ = static_cast<core::AlgorithmType>(data[offset++]);
    header.kdf_ = static_cast<core::KDFType>(data[offset++]);
    header.security_level_ = static_cast<core::SecurityLevel>(data[offset++]);
    
    // Salt
    uint16_t salt_len = read_uint16(data, offset);
    if (offset + salt_len > data.size()) {
        return core::Result<FileHeader>::error("Invalid salt length");
    }
    header.salt_.assign(data.begin() + offset, data.begin() + offset + salt_len);
    offset += salt_len;
    
    // Nonce
    uint16_t nonce_len = read_uint16(data, offset);
    if (offset + nonce_len > data.size()) {
        return core::Result<FileHeader>::error("Invalid nonce length");
    }
    header.nonce_.assign(data.begin() + offset, data.begin() + offset + nonce_len);
    offset += nonce_len;
    
    // Tag
    uint16_t tag_len = read_uint16(data, offset);
    if (offset + tag_len > data.size()) {
        return core::Result<FileHeader>::error("Invalid tag length");
    }
    header.tag_.assign(data.begin() + offset, data.begin() + offset + tag_len);
    offset += tag_len;
    
    // Sizes
    header.original_size_ = read_uint64(data, offset);
    header.encrypted_size_ = read_uint64(data, offset);
    header.timestamp_ = read_uint64(data, offset);
    
    // Flags
    header.flags_ = read_uint32(data, offset);
    
    // Reserved
    if (offset + 16 > data.size()) {
        return core::Result<FileHeader>::error("Header truncated");
    }
    std::copy_n(data.begin() + static_cast<std::ptrdiff_t>(offset), 16, header.reserved_.begin());
    offset += 16;
    
    return core::Result<FileHeader>::ok(std::move(header));
}

bool FileHeader::validate() const {
    // Check salt
    if (salt_.empty() || salt_.size() > 64) {
        return false;
    }
    
    // Check nonce (GCM needs 12 bytes typically)
    if (nonce_.empty() || nonce_.size() > 32) {
        return false;
    }
    
    // Check tag for AEAD modes
    if (algorithm_ == core::AlgorithmType::AES_128_GCM ||
        algorithm_ == core::AlgorithmType::AES_192_GCM ||
        algorithm_ == core::AlgorithmType::AES_256_GCM ||
        algorithm_ == core::AlgorithmType::CHACHA20_POLY1305) {
        if (tag_.empty() || tag_.size() != 16) {
            return false;
        }
    }
    
    return true;
}

size_t FileHeader::total_size() const {
    return 4 +  // magic
           2 +  // version
           3 +  // algorithm, kdf, security
           2 + salt_.size() +
           2 + nonce_.size() +
           2 + tag_.size() +
           8 +  // original_size
           8 +  // encrypted_size
           8 +  // timestamp
           4 +  // flags
           16;  // reserved
}

// Helper functions
void FileHeader::write_uint16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

void FileHeader::write_uint32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

void FileHeader::write_uint64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
}

uint16_t FileHeader::read_uint16(std::span<const uint8_t> data, size_t& offset) {
    uint16_t val = data[offset] | (data[offset + 1] << 8);
    offset += 2;
    return val;
}

uint32_t FileHeader::read_uint32(std::span<const uint8_t> data, size_t& offset) {
    uint32_t val = data[offset] | 
                   (data[offset + 1] << 8) | 
                   (data[offset + 2] << 16) | 
                   (data[offset + 3] << 24);
    offset += 4;
    return val;
}

uint64_t FileHeader::read_uint64(std::span<const uint8_t> data, size_t& offset) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }
    offset += 8;
    return val;
}

} // namespace format
} // namespace filevault
