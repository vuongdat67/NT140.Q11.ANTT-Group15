#include "filevault/archive/archive_format.hpp"
#include <fstream>
#include <cstring>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
#include <sys/stat.h>
#else
#include <sys/stat.h>
#endif

namespace filevault::archive {

namespace fs = std::filesystem;

// FileEntry serialization
std::vector<uint8_t> FileEntry::serialize() const {
    std::vector<uint8_t> buffer;
    
    // Filename length + filename
    uint32_t name_len = static_cast<uint32_t>(filename.size());
    uint8_t* name_len_bytes = reinterpret_cast<uint8_t*>(&name_len);
    buffer.insert(buffer.end(), name_len_bytes, name_len_bytes + 4);
    buffer.insert(buffer.end(), filename.begin(), filename.end());
    
    // File size
    const uint8_t* size_bytes = reinterpret_cast<const uint8_t*>(&file_size);
    buffer.insert(buffer.end(), size_bytes, size_bytes + 8);
    
    // Offset
    const uint8_t* offset_bytes = reinterpret_cast<const uint8_t*>(&offset);
    buffer.insert(buffer.end(), offset_bytes, offset_bytes + 8);
    
    // Modified time
    const uint8_t* time_bytes = reinterpret_cast<const uint8_t*>(&modified_time);
    buffer.insert(buffer.end(), time_bytes, time_bytes + 8);
    
    // Permissions
    const uint8_t* perm_bytes = reinterpret_cast<const uint8_t*>(&permissions);
    buffer.insert(buffer.end(), perm_bytes, perm_bytes + 4);
    
    return buffer;
}

FileEntry FileEntry::deserialize(std::span<const uint8_t> data, size_t& offset) {
    FileEntry entry;
    
    // Bounds check for filename length
    if (offset + 4 > data.size()) {
        throw std::runtime_error("Truncated archive: cannot read filename length");
    }
    
    // Filename
    uint32_t name_len = *reinterpret_cast<const uint32_t*>(&data[offset]);
    offset += 4;
    
    // Bounds check for filename data
    if (offset + name_len > data.size()) {
        throw std::runtime_error("Truncated archive: cannot read filename");
    }
    entry.filename = std::string(reinterpret_cast<const char*>(&data[offset]), name_len);
    offset += name_len;
    
    // Bounds check for remaining fixed-size fields (8+8+8+4 = 28 bytes)
    if (offset + 28 > data.size()) {
        throw std::runtime_error("Truncated archive: cannot read file entry metadata");
    }
    
    // File size
    entry.file_size = *reinterpret_cast<const uint64_t*>(&data[offset]);
    offset += 8;
    
    // Offset
    entry.offset = *reinterpret_cast<const uint64_t*>(&data[offset]);
    offset += 8;
    
    // Modified time
    entry.modified_time = *reinterpret_cast<const uint64_t*>(&data[offset]);
    offset += 8;
    
    // Permissions
    entry.permissions = *reinterpret_cast<const uint32_t*>(&data[offset]);
    offset += 4;
    
    return entry;
}

// Archive creation
std::vector<uint8_t> ArchiveFormat::create_archive(const std::vector<fs::path>& files) {
    std::vector<uint8_t> archive;
    std::vector<FileEntry> entries;
    
    // Write header: magic + version + entry count
    archive.insert(archive.end(), MAGIC, MAGIC + 6);
    archive.push_back(VERSION);
    
    write_uint32(archive, static_cast<uint32_t>(files.size()));
    
    // Calculate offsets and create entries
    uint64_t current_offset = 0;
    for (const auto& file_path : files) {
        if (!fs::exists(file_path)) {
            throw std::runtime_error("File not found: " + file_path.string());
        }
        
        FileEntry entry;
        entry.filename = file_path.filename().string();
        entry.file_size = fs::file_size(file_path);
        entry.offset = current_offset;
        
        // Get modification time
        auto ftime = fs::last_write_time(file_path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        entry.modified_time = std::chrono::system_clock::to_time_t(sctp);
        
        // Get permissions
#ifdef _WIN32
        struct _stat64 st;
        if (_stat64(file_path.string().c_str(), &st) == 0) {
            entry.permissions = st.st_mode;
        } else {
            entry.permissions = 0644;  // Default
        }
#else
        struct stat st;
        if (stat(file_path.c_str(), &st) == 0) {
            entry.permissions = st.st_mode & 0777;
        } else {
            entry.permissions = 0644;
        }
#endif
        
        entries.push_back(entry);
        current_offset += entry.file_size;
    }
    
    // Write all entry metadata
    for (const auto& entry : entries) {
        auto entry_data = entry.serialize();
        archive.insert(archive.end(), entry_data.begin(), entry_data.end());
    }
    
    // Write file data
    for (size_t i = 0; i < files.size(); ++i) {
        std::ifstream file(files[i], std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open: " + files[i].string());
        }
        
        std::vector<uint8_t> file_data(entries[i].file_size);
        file.read(reinterpret_cast<char*>(file_data.data()), entries[i].file_size);
        
        archive.insert(archive.end(), file_data.begin(), file_data.end());
    }
    
    return archive;
}

// Archive extraction
bool ArchiveFormat::extract_archive(
    std::span<const uint8_t> archive_data,
    const fs::path& output_dir
) {
    size_t offset = 0;
    
    // Verify magic
    if (archive_data.size() < 11 || 
        std::memcmp(archive_data.data(), MAGIC, 6) != 0) {
        return false;
    }
    offset += 6;
    
    // Verify version
    uint8_t version = archive_data[offset++];
    if (version != VERSION) {
        return false;
    }
    
    // Read entry count
    uint32_t entry_count = read_uint32(archive_data, offset);
    
    // Read all entries
    std::vector<FileEntry> entries;
    for (uint32_t i = 0; i < entry_count; ++i) {
        entries.push_back(FileEntry::deserialize(archive_data, offset));
    }
    
    // Data section starts here
    size_t data_section_offset = offset;
    
    // Create output directory
    if (!fs::exists(output_dir)) {
        fs::create_directories(output_dir);
    }
    
    // Extract files
    for (const auto& entry : entries) {
        fs::path output_path = output_dir / entry.filename;
        
        std::ofstream out_file(output_path, std::ios::binary);
        if (!out_file) {
            return false;
        }
        
        size_t file_offset = data_section_offset + entry.offset;
        out_file.write(
            reinterpret_cast<const char*>(&archive_data[file_offset]),
            entry.file_size
        );
        
        out_file.close();
        
        // Restore modification time
        auto ftime = fs::file_time_type::clock::now() + 
            std::chrono::seconds(entry.modified_time) -
            std::chrono::seconds(std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now()
            ));
        fs::last_write_time(output_path, ftime);
        
        // Restore permissions on Unix
#ifndef _WIN32
        chmod(output_path.c_str(), entry.permissions);
#endif
    }
    
    return true;
}

// List files
std::vector<FileEntry> ArchiveFormat::list_files(std::span<const uint8_t> archive_data) {
    std::vector<FileEntry> entries;
    size_t offset = 0;
    
    // Verify magic
    if (archive_data.size() < 11 || 
        std::memcmp(archive_data.data(), MAGIC, 6) != 0) {
        return entries;
    }
    offset += 6;
    
    // Verify version
    uint8_t version = archive_data[offset++];
    if (version != VERSION) {
        return entries;
    }
    
    // Read entry count
    uint32_t entry_count = read_uint32(archive_data, offset);
    
    // Read all entries
    for (uint32_t i = 0; i < entry_count; ++i) {
        entries.push_back(FileEntry::deserialize(archive_data, offset));
    }
    
    return entries;
}

// Helper functions
void ArchiveFormat::write_uint32(std::vector<uint8_t>& buffer, uint32_t value) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    buffer.insert(buffer.end(), bytes, bytes + 4);
}

void ArchiveFormat::write_uint64(std::vector<uint8_t>& buffer, uint64_t value) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    buffer.insert(buffer.end(), bytes, bytes + 8);
}

uint32_t ArchiveFormat::read_uint32(std::span<const uint8_t> data, size_t& offset) {
    uint32_t value = *reinterpret_cast<const uint32_t*>(&data[offset]);
    offset += 4;
    return value;
}

uint64_t ArchiveFormat::read_uint64(std::span<const uint8_t> data, size_t& offset) {
    uint64_t value = *reinterpret_cast<const uint64_t*>(&data[offset]);
    offset += 8;
    return value;
}

std::string ArchiveFormat::read_string(std::span<const uint8_t> data, size_t& offset) {
    uint32_t len = read_uint32(data, offset);
    std::string str(reinterpret_cast<const char*>(&data[offset]), len);
    offset += len;
    return str;
}

} // namespace filevault::archive
