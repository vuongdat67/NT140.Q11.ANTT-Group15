#include "filevault/core/file_format.hpp"
#include "filevault/exceptions.hpp"
#include <cstring>
#include <fstream>

namespace filevault::core {

FileHeader FileFormatHandler::parse_header(const std::string& file_path) {
    std::ifstream input(file_path, std::ios::binary);
    if (!input) {
        throw FileNotFoundException(file_path);
    }
    
    Bytes data((std::istreambuf_iterator<char>(input)),
                std::istreambuf_iterator<char>());
    
    return parse_header(data);
}

FileHeader FileFormatHandler::parse_header(const Bytes& data) {
    if (data.size() < 8) {
        throw InvalidFormatException("File too small");
    }

    FileHeader header;
    size_t offset = 0;

    // Parse magic bytes
    if (data[offset++] != MAGIC_BYTE_1 || data[offset++] != MAGIC_BYTE_2) {
        throw InvalidFormatException("Invalid magic bytes");
    }

    // Parse version
    header.major_version = data[offset++];
    header.minor_version = data[offset++];

    if (header.major_version != VERSION_MAJOR) {
        throw InvalidFormatException("Unsupported version");
    }

    // Parse cipher and compression types
    header.cipher_type = static_cast<CipherType>(data[offset++]);
    header.cipher_mode = static_cast<CipherMode>(data[offset++]);
    header.kdf_type = static_cast<KDFType>(data[offset++]);
    header.compression_type = static_cast<CompressionType>(data[offset++]);

    // Helper to read uint32_t (little-endian)
    auto read_uint32 = [&data, &offset]() -> uint32_t {
        if (offset + 4 > data.size()) throw InvalidFormatException("Truncated header");
        uint32_t value = static_cast<uint32_t>(data[offset]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;
        return value;
    };

    // Helper to read uint64_t (little-endian)
    auto read_uint64 = [&data, &offset]() -> uint64_t {
        if (offset + 8 > data.size()) throw InvalidFormatException("Truncated header");
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= (static_cast<uint64_t>(data[offset++]) << (i * 8));
        }
        return value;
    };

    // Parse salt (length + data)
    if (offset >= data.size()) throw InvalidFormatException("Missing salt length");
    uint8_t salt_len = data[offset++];
    if (offset + salt_len > data.size()) throw InvalidFormatException("Truncated salt");
    header.salt.assign(data.begin() + offset, data.begin() + offset + salt_len);
    offset += salt_len;

    // Parse IV/nonce (length + data)
    if (offset >= data.size()) throw InvalidFormatException("Missing IV length");
    uint8_t iv_len = data[offset++];
    if (offset + iv_len > data.size()) throw InvalidFormatException("Truncated IV");
    header.iv_or_nonce.assign(data.begin() + offset, data.begin() + offset + iv_len);
    offset += iv_len;

    // Parse KDF parameters (16 bytes)
    header.kdf_params.type = header.kdf_type;
    header.kdf_params.iterations = read_uint32();
    header.kdf_params.memory_kb = read_uint32();
    header.kdf_params.time_cost = read_uint32();
    header.kdf_params.parallelism = read_uint32();

    // Parse sizes
    header.original_size = read_uint32();
    header.compressed_size = read_uint32();

    // Parse timestamp
    header.timestamp = read_uint64();

    // Parse filename (length + data)
    if (offset >= data.size()) throw InvalidFormatException("Missing filename length");
    uint8_t filename_len = data[offset++];
    if (offset + filename_len > data.size()) throw InvalidFormatException("Truncated filename");
    header.filename.assign(data.begin() + offset, data.begin() + offset + filename_len);
    offset += filename_len;

    return header;
}

Bytes FileFormatHandler::serialize_header(const FileHeader& header) {
    Bytes result;
    
    // Reserve approximate size
    result.reserve(128);

    // Write magic bytes (always use constants, not header.magic which may be uninitialized)
    result.push_back(MAGIC_BYTE_1);
    result.push_back(MAGIC_BYTE_2);
    
    // Write version (use header values if set, otherwise defaults)
    result.push_back(header.major_version > 0 ? header.major_version : VERSION_MAJOR);
    result.push_back(header.minor_version);

    // Write cipher and KDF types
    result.push_back(static_cast<uint8_t>(header.cipher_type));
    result.push_back(static_cast<uint8_t>(header.cipher_mode));
    result.push_back(static_cast<uint8_t>(header.kdf_type));
    result.push_back(static_cast<uint8_t>(header.compression_type));

    // Write salt (length + data)
    if (header.salt.size() > 255) {
        throw InvalidArgumentException("Salt too large (max 255 bytes)");
    }
    result.push_back(static_cast<uint8_t>(header.salt.size()));
    result.insert(result.end(), header.salt.begin(), header.salt.end());

    // Write IV/nonce (length + data)
    if (header.iv_or_nonce.size() > 255) {
        throw InvalidArgumentException("IV/nonce too large (max 255 bytes)");
    }
    result.push_back(static_cast<uint8_t>(header.iv_or_nonce.size()));
    result.insert(result.end(), header.iv_or_nonce.begin(), header.iv_or_nonce.end());

    // Write KDF parameters (16 bytes fixed for now)
    // Format: [iterations:4][memory_kb:4][time_cost:4][parallelism:4]
    auto write_uint32 = [&result](uint32_t value) {
        result.push_back(static_cast<uint8_t>(value & 0xFF));
        result.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        result.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    };
    
    write_uint32(header.kdf_params.iterations);
    write_uint32(header.kdf_params.memory_kb);
    write_uint32(header.kdf_params.time_cost);
    write_uint32(header.kdf_params.parallelism);

    // Write original size (uint32_t, little-endian)
    write_uint32(header.original_size);

    // Write compressed size (uint32_t, little-endian)
    write_uint32(header.compressed_size);

    // Write timestamp (uint64_t, little-endian)
    auto write_uint64 = [&result](uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            result.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
        }
    };
    write_uint64(header.timestamp);

    // Write filename (length + data)
    if (header.filename.size() > 255) {
        throw InvalidArgumentException("Filename too long (max 255 bytes)");
    }
    result.push_back(static_cast<uint8_t>(header.filename.size()));
    result.insert(result.end(), header.filename.begin(), header.filename.end());

    return result;
}

bool FileFormatHandler::validate(const std::string& file_path) {
    try {
        parse_header(file_path);
        return true;
    } catch (...) {
        return false;
    }
}

bool FileFormatHandler::validate(const Bytes& data) {
    try {
        parse_header(data);
        return true;
    } catch (...) {
        return false;
    }
}

size_t FileFormatHandler::get_header_size(const FileHeader& header) {
    // Base header: 8 bytes
    size_t size = 8;
    
    // Add variable-length fields
    size += 1 + header.salt.size();        // salt length + salt
    size += 1 + header.iv_or_nonce.size(); // IV length + IV
    size += 16;  // KDF params (approximate)
    size += 8;   // original size (uint64_t)
    size += 4;   // metadata length (uint32_t)
    
    return size;
}

// Builder implementation
FileFormatBuilder::FileFormatBuilder() {
    // Initialize magic bytes and version
    header_.magic[0] = 'F';
    header_.magic[1] = 'V';
    header_.magic[2] = 'L';
    header_.magic[3] = 'T';
    header_.major_version = 1;
    header_.minor_version = 0;
}

FileFormatBuilder& FileFormatBuilder::set_cipher(CipherType type, CipherMode mode) {
    header_.cipher_type = type;
    header_.cipher_mode = mode;
    return *this;
}

FileFormatBuilder& FileFormatBuilder::set_kdf(const crypto::KDFParams& params) {
    header_.kdf_params = params;
    return *this;
}

FileFormatBuilder& FileFormatBuilder::set_compression(CompressionType type) {
    header_.compression_type = type;
    return *this;
}

FileFormatBuilder& FileFormatBuilder::set_iv(const Bytes& iv) {
    header_.iv_or_nonce = iv;
    return *this;
}

FileFormatBuilder& FileFormatBuilder::set_original_size(uint32_t size) {
    header_.original_size = size;
    return *this;
}

FileFormatBuilder& FileFormatBuilder::set_filename(const std::string& name) {
    header_.filename = name;
    return *this;
}

Bytes FileFormatBuilder::build_header() {
    return FileFormatHandler::serialize_header(header_);
}

void FileFormatBuilder::validate() {
    // TODO: Add validation logic
}

} // namespace filevault::core
