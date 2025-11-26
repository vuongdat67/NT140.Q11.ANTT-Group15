/**
 * @file test_archive.cpp
 * @brief Unit tests for archive format
 *
 * Tests creating, listing and extracting archives
 */

#include <catch2/catch_test_macros.hpp>
#include "filevault/archive/archive_format.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

using namespace filevault::archive;
namespace fs = std::filesystem;

// Helper class for test file management
class TestFileHelper {
public:
    static std::string test_dir;
    
    static void setup() {
        test_dir = "test_archive_temp";
        fs::create_directories(test_dir);
    }
    
    static void cleanup() {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
    
    static std::filesystem::path create_test_file(
        const std::string& name, 
        const std::string& content
    ) {
        fs::path filepath = fs::path(test_dir) / name;
        std::ofstream file(filepath, std::ios::binary);
        file << content;
        file.close();
        return filepath;
    }
    
    static std::string read_file(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        return std::string(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );
    }
};

std::string TestFileHelper::test_dir;

// ===========================================
// Archive Creation Tests
// ===========================================
TEST_CASE("Archive Creation", "[archive][create]") {
    TestFileHelper::setup();
    
    SECTION("Create archive from single file") {
        auto file1 = TestFileHelper::create_test_file("file1.txt", "Hello, World!");
        
        std::vector<fs::path> files = { file1 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        INFO("Archive size: " << archive_data.size());
        REQUIRE(!archive_data.empty());
        
        // Check magic number
        REQUIRE(archive_data.size() >= 6);
        std::string magic(archive_data.begin(), archive_data.begin() + 6);
        REQUIRE(magic == "FVARCH");
    }
    
    SECTION("Create archive from multiple files") {
        auto file1 = TestFileHelper::create_test_file("file1.txt", "Content 1");
        auto file2 = TestFileHelper::create_test_file("file2.txt", "Content 2 is longer");
        auto file3 = TestFileHelper::create_test_file("file3.txt", "Content 3");
        
        std::vector<fs::path> files = { file1, file2, file3 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        REQUIRE(!archive_data.empty());
        
        // List files in archive
        auto entries = ArchiveFormat::list_files(archive_data);
        REQUIRE(entries.size() == 3);
    }
    
    SECTION("Create archive with binary content") {
        std::string binary_content;
        for (int i = 0; i < 256; ++i) {
            binary_content += static_cast<char>(i);
        }
        
        auto file1 = TestFileHelper::create_test_file("binary.bin", binary_content);
        
        std::vector<fs::path> files = { file1 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        REQUIRE(!archive_data.empty());
    }
    
    SECTION("Empty file list creates minimal archive") {
        std::vector<fs::path> files;
        auto archive_data = ArchiveFormat::create_archive(files);
        
        // Should have at least header
        REQUIRE(archive_data.size() >= 11);  // Magic(6) + Version(1) + Count(4)
    }
    
    TestFileHelper::cleanup();
}

// ===========================================
// Archive List Tests
// ===========================================
TEST_CASE("Archive List Files", "[archive][list]") {
    TestFileHelper::setup();
    
    SECTION("List files from valid archive") {
        auto file1 = TestFileHelper::create_test_file("doc1.txt", "Document 1 content");
        auto file2 = TestFileHelper::create_test_file("doc2.txt", "Document 2");
        
        std::vector<fs::path> files = { file1, file2 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        auto entries = ArchiveFormat::list_files(archive_data);
        
        REQUIRE(entries.size() == 2);
        
        // Check filenames (should preserve just the filename, not full path)
        bool found_doc1 = false;
        bool found_doc2 = false;
        for (const auto& entry : entries) {
            if (entry.filename.find("doc1.txt") != std::string::npos) {
                found_doc1 = true;
                REQUIRE(entry.file_size == 18);  // "Document 1 content"
            }
            if (entry.filename.find("doc2.txt") != std::string::npos) {
                found_doc2 = true;
                REQUIRE(entry.file_size == 10);  // "Document 2"
            }
        }
        
        REQUIRE(found_doc1);
        REQUIRE(found_doc2);
    }
    
    SECTION("List files from empty archive") {
        std::vector<fs::path> files;
        auto archive_data = ArchiveFormat::create_archive(files);
        
        auto entries = ArchiveFormat::list_files(archive_data);
        REQUIRE(entries.empty());
    }
    
    SECTION("List files from invalid data returns empty") {
        std::vector<uint8_t> invalid_data = { 0x00, 0x01, 0x02 };
        auto entries = ArchiveFormat::list_files(invalid_data);
        REQUIRE(entries.empty());
    }
    
    TestFileHelper::cleanup();
}

// ===========================================
// Archive Extract Tests
// ===========================================
TEST_CASE("Archive Extraction", "[archive][extract]") {
    TestFileHelper::setup();
    
    SECTION("Extract single file") {
        std::string original_content = "This is the original content!";
        auto file1 = TestFileHelper::create_test_file("original.txt", original_content);
        
        std::vector<fs::path> files = { file1 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        // Create extraction directory
        fs::path extract_dir = fs::path(TestFileHelper::test_dir) / "extracted";
        fs::create_directories(extract_dir);
        
        bool success = ArchiveFormat::extract_archive(archive_data, extract_dir);
        REQUIRE(success);
        
        // Verify extracted file
        fs::path extracted_file = extract_dir / "original.txt";
        REQUIRE(fs::exists(extracted_file));
        
        std::string extracted_content = TestFileHelper::read_file(extracted_file);
        REQUIRE(extracted_content == original_content);
    }
    
    SECTION("Extract multiple files") {
        auto file1 = TestFileHelper::create_test_file("a.txt", "Content A");
        auto file2 = TestFileHelper::create_test_file("b.txt", "Content B");
        auto file3 = TestFileHelper::create_test_file("c.txt", "Content C");
        
        std::vector<fs::path> files = { file1, file2, file3 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        fs::path extract_dir = fs::path(TestFileHelper::test_dir) / "multi_extract";
        fs::create_directories(extract_dir);
        
        bool success = ArchiveFormat::extract_archive(archive_data, extract_dir);
        REQUIRE(success);
        
        REQUIRE(TestFileHelper::read_file(extract_dir / "a.txt") == "Content A");
        REQUIRE(TestFileHelper::read_file(extract_dir / "b.txt") == "Content B");
        REQUIRE(TestFileHelper::read_file(extract_dir / "c.txt") == "Content C");
    }
    
    SECTION("Extract binary content correctly") {
        std::string binary_content;
        for (int i = 0; i < 256; ++i) {
            binary_content += static_cast<char>(i);
        }
        
        auto file1 = TestFileHelper::create_test_file("binary.bin", binary_content);
        
        std::vector<fs::path> files = { file1 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        fs::path extract_dir = fs::path(TestFileHelper::test_dir) / "binary_extract";
        fs::create_directories(extract_dir);
        
        bool success = ArchiveFormat::extract_archive(archive_data, extract_dir);
        REQUIRE(success);
        
        std::string extracted = TestFileHelper::read_file(extract_dir / "binary.bin");
        REQUIRE(extracted == binary_content);
    }
    
    SECTION("Extract empty archive succeeds") {
        std::vector<fs::path> files;
        auto archive_data = ArchiveFormat::create_archive(files);
        
        fs::path extract_dir = fs::path(TestFileHelper::test_dir) / "empty_extract";
        fs::create_directories(extract_dir);
        
        bool success = ArchiveFormat::extract_archive(archive_data, extract_dir);
        REQUIRE(success);
    }
    
    TestFileHelper::cleanup();
}

// ===========================================
// Round-trip Tests
// ===========================================
TEST_CASE("Archive Round-trip", "[archive][roundtrip]") {
    TestFileHelper::setup();
    
    SECTION("Create and extract preserves all data") {
        // Create test files with various content
        std::vector<std::pair<std::string, std::string>> test_files = {
            {"test1.txt", "Short content"},
            {"test2.txt", std::string(1000, 'X')},  // 1KB of X
            {"test3.txt", "Line 1\nLine 2\nLine 3\n"},
            {"test4.txt", "Unicode: こんにちは 世界"},
        };
        
        std::vector<fs::path> files;
        for (const auto& [name, content] : test_files) {
            files.push_back(TestFileHelper::create_test_file(name, content));
        }
        
        // Create archive
        auto archive_data = ArchiveFormat::create_archive(files);
        REQUIRE(!archive_data.empty());
        
        // Extract
        fs::path extract_dir = fs::path(TestFileHelper::test_dir) / "roundtrip";
        fs::create_directories(extract_dir);
        
        bool success = ArchiveFormat::extract_archive(archive_data, extract_dir);
        REQUIRE(success);
        
        // Verify all files
        for (const auto& [name, expected_content] : test_files) {
            fs::path extracted_path = extract_dir / name;
            REQUIRE(fs::exists(extracted_path));
            
            std::string actual_content = TestFileHelper::read_file(extracted_path);
            INFO("Checking file: " << name);
            REQUIRE(actual_content == expected_content);
        }
    }
    
    TestFileHelper::cleanup();
}

// ===========================================
// Error Handling Tests
// ===========================================
TEST_CASE("Archive Error Handling", "[archive][errors]") {
    TestFileHelper::setup();
    
    SECTION("Non-existent file in create fails gracefully") {
        std::vector<fs::path> files = { "nonexistent_file.txt" };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        // Should either throw or return empty/partial archive
        // The behavior depends on implementation
        INFO("Archive size with non-existent file: " << archive_data.size());
    }
    
    SECTION("Invalid archive data fails extraction") {
        std::vector<uint8_t> garbage_data = { 
            'G', 'A', 'R', 'B', 'A', 'G', 'E', 0x00, 0x01, 0x02 
        };
        
        fs::path extract_dir = fs::path(TestFileHelper::test_dir) / "garbage_extract";
        fs::create_directories(extract_dir);
        
        bool success = ArchiveFormat::extract_archive(garbage_data, extract_dir);
        REQUIRE_FALSE(success);
    }
    
    SECTION("Truncated archive fails extraction") {
        auto file1 = TestFileHelper::create_test_file("truncate.txt", "Some content");
        
        std::vector<fs::path> files = { file1 };
        auto archive_data = ArchiveFormat::create_archive(files);
        
        // Truncate the archive
        std::vector<uint8_t> truncated(archive_data.begin(), 
                                       archive_data.begin() + archive_data.size() / 2);
        
        fs::path extract_dir = fs::path(TestFileHelper::test_dir) / "truncated_extract";
        fs::create_directories(extract_dir);
        
        bool success = ArchiveFormat::extract_archive(truncated, extract_dir);
        REQUIRE_FALSE(success);
    }
    
    TestFileHelper::cleanup();
}

// ===========================================
// FileEntry Serialization Tests
// ===========================================
TEST_CASE("FileEntry Serialization", "[archive][entry]") {
    SECTION("Serialize and deserialize entry") {
        FileEntry original;
        original.filename = "test_file.txt";
        original.file_size = 12345;
        original.offset = 1000;
        original.modified_time = 1699999999;
        original.permissions = 0644;
        
        auto serialized = original.serialize();
        REQUIRE(!serialized.empty());
        
        size_t offset = 0;
        FileEntry restored = FileEntry::deserialize(serialized, offset);
        
        REQUIRE(restored.filename == original.filename);
        REQUIRE(restored.file_size == original.file_size);
        REQUIRE(restored.offset == original.offset);
        REQUIRE(restored.modified_time == original.modified_time);
        REQUIRE(restored.permissions == original.permissions);
    }
    
    SECTION("Entry with long filename") {
        FileEntry entry;
        entry.filename = std::string(255, 'a') + ".txt";  // Very long filename
        entry.file_size = 100;
        entry.offset = 0;
        entry.modified_time = 0;
        entry.permissions = 0;
        
        auto serialized = entry.serialize();
        size_t offset = 0;
        FileEntry restored = FileEntry::deserialize(serialized, offset);
        
        REQUIRE(restored.filename == entry.filename);
    }
    
    SECTION("Entry with empty filename") {
        FileEntry entry;
        entry.filename = "";
        entry.file_size = 0;
        entry.offset = 0;
        entry.modified_time = 0;
        entry.permissions = 0;
        
        auto serialized = entry.serialize();
        size_t offset = 0;
        FileEntry restored = FileEntry::deserialize(serialized, offset);
        
        REQUIRE(restored.filename == "");
    }
}
