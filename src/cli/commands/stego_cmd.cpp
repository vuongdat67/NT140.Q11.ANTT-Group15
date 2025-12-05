#include "filevault/cli/commands/stego_cmd.hpp"
#include "filevault/steganography/lsb.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/file_io.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>

namespace filevault::cli::commands {

using namespace filevault::steganography;
namespace fs = std::filesystem;

void StegoCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand("stego", description());
    
    // Embed subcommand
    auto* embed_cmd = cmd->add_subcommand("embed", "Hide data in an image");
    embed_cmd->add_option("input", input_file_, "Secret file to hide")
        ->required()
        ->check(CLI::ExistingFile);
    embed_cmd->add_option("cover", cover_image_, "Cover image (PNG/BMP)")
        ->required()
        ->check(CLI::ExistingFile);
    embed_cmd->add_option("output", output_file_, "Output stego image")
        ->required();
    embed_cmd->add_option("-b,--bits", bits_per_channel_, "Bits per channel (1-4, default: 1)")
        ->check(CLI::Range(1, 4));
    embed_cmd->add_flag("-v,--verbose", verbose_, "Show detailed information");
    embed_cmd->callback([this]() { operation_ = "embed"; execute(); });
    
    // Extract subcommand
    auto* extract_cmd = cmd->add_subcommand("extract", "Extract hidden data from an image");
    extract_cmd->add_option("stego", input_file_, "Stego image")
        ->required()
        ->check(CLI::ExistingFile);
    extract_cmd->add_option("output", output_file_, "Output file for extracted data")
        ->required();
    extract_cmd->add_option("-b,--bits", bits_per_channel_, "Bits per channel (1-4, default: 1)")
        ->check(CLI::Range(1, 4));
    extract_cmd->add_flag("-v,--verbose", verbose_, "Show detailed information");
    extract_cmd->callback([this]() { operation_ = "extract"; execute(); });
    
    // Capacity subcommand
    auto* capacity_cmd = cmd->add_subcommand("capacity", "Calculate embedding capacity of an image");
    capacity_cmd->add_option("image", cover_image_, "Image file")
        ->required()
        ->check(CLI::ExistingFile);
    capacity_cmd->add_option("-b,--bits", bits_per_channel_, "Bits per channel (1-4, default: 1)")
        ->check(CLI::Range(1, 4));
    capacity_cmd->callback([this]() { operation_ = "capacity"; execute(); });
    
    cmd->require_subcommand(1);
}

int StegoCommand::execute() {
    if (operation_ == "embed") {
        return do_embed();
    } else if (operation_ == "extract") {
        return do_extract();
    } else if (operation_ == "capacity") {
        return do_capacity();
    }
    
    utils::Console::error("Unknown operation");
    return 1;
}

int StegoCommand::do_embed() {
    try {
        // Read secret data
        auto result = utils::FileIO::read_file(input_file_);
        if (!result.success) {
            utils::Console::error(std::format("Failed to read input file: {}", result.error_message));
            return 1;
        }
        auto secret_data = result.value;
        
        // Extract filename with extension for metadata
        std::string filename = fs::path(input_file_).filename().string();
        
        // Prepend filename metadata: [filename_length(2 bytes)][filename][data]
        std::vector<uint8_t> data_with_metadata;
        uint16_t filename_len = static_cast<uint16_t>(filename.size());
        data_with_metadata.push_back(filename_len & 0xFF);
        data_with_metadata.push_back((filename_len >> 8) & 0xFF);
        data_with_metadata.insert(data_with_metadata.end(), filename.begin(), filename.end());
        data_with_metadata.insert(data_with_metadata.end(), secret_data.begin(), secret_data.end());
        
        if (verbose_) {
            utils::Console::info(std::format("Secret data size: {} bytes", secret_data.size()));
        }
        
        // Check capacity (with metadata overhead)
        size_t capacity = LSBSteganography::calculate_capacity(cover_image_, bits_per_channel_);
        
        if (data_with_metadata.size() > capacity) {
            utils::Console::error(std::format(
                "Secret data ({} bytes) + metadata ({} bytes) exceeds image capacity ({} bytes)",
                secret_data.size(), data_with_metadata.size() - secret_data.size(), capacity
            ));
            utils::Console::info(std::format(
                "Try using --bits {} for more capacity",
                std::min(bits_per_channel_ + 1, 4)
            ));
            return 1;
        }
        
        if (verbose_) {
            utils::Console::info(std::format("Original filename: {}", filename));
            utils::Console::info(std::format("Image capacity: {} bytes", capacity));
            utils::Console::info(std::format("Utilization: {:.1f}%", 
                (static_cast<double>(data_with_metadata.size()) / capacity) * 100.0));
            utils::Console::info(std::format("Bits per channel: {}", bits_per_channel_));
        }
        
        // Embed data with metadata
        utils::Console::info("Embedding data...");
        
        bool success = LSBSteganography::embed(
            cover_image_,
            data_with_metadata,
            output_file_,
            bits_per_channel_
        );
        
        if (!success) {
            utils::Console::error("Failed to embed data");
            return 1;
        }
        
        // Calculate file sizes
        size_t cover_size = fs::file_size(cover_image_);
        size_t stego_size = fs::file_size(output_file_);
        
        utils::Console::success(std::format(
            "Successfully embedded {} bytes into image",
            secret_data.size()
        ));
        
        if (verbose_) {
            utils::Console::info(std::format("Cover image: {} bytes", cover_size));
            utils::Console::info(std::format("Stego image: {} bytes", stego_size));
            utils::Console::info(std::format("Size change: {:+} bytes ({:+.2f}%)",
                static_cast<int64_t>(stego_size) - static_cast<int64_t>(cover_size),
                ((static_cast<double>(stego_size) / cover_size) - 1.0) * 100.0
            ));
        }
        
        utils::Console::info(std::format("Output: {}", output_file_));
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(std::format("Embed failed: {}", e.what()));
        return 1;
    }
}

int StegoCommand::do_extract() {
    try {
        if (verbose_) {
            utils::Console::info(std::format("Extracting from: {}", input_file_));
            utils::Console::info(std::format("Bits per channel: {}", bits_per_channel_));
        }
        
        // Extract data
        utils::Console::info("Extracting hidden data...");
        
        auto extracted_data = LSBSteganography::extract(input_file_, bits_per_channel_);
        
        if (extracted_data.empty()) {
            utils::Console::error("No data found or extraction failed");
            utils::Console::info("Make sure you're using the correct --bits value");
            return 1;
        }
        
        // Parse metadata: [2 bytes len][filename][data]
        std::string original_filename;
        std::vector<uint8_t> actual_data;
        
        if (extracted_data.size() >= 2) {
            uint16_t filename_len = extracted_data[0] | (extracted_data[1] << 8);
            
            // Check if metadata is present and valid
            if (filename_len > 0 && filename_len < 256 && extracted_data.size() > 2 + static_cast<size_t>(filename_len)) {
                // Extract filename
                original_filename.assign(
                    extracted_data.begin() + 2,
                    extracted_data.begin() + 2 + filename_len
                );
                
                // Extract actual data
                actual_data.assign(
                    extracted_data.begin() + 2 + filename_len,
                    extracted_data.end()
                );
                
                if (verbose_) {
                    utils::Console::info(std::format("Original filename: {}", original_filename));
                }
            } else {
                // No metadata or invalid format, use all data
                actual_data = extracted_data;
            }
        } else {
            // Data too small for metadata
            actual_data = extracted_data;
        }
        
        // Determine output filename: use original if found, otherwise use specified output
        std::string final_output = output_file_;
        if (!original_filename.empty()) {
            // Replace the filename part with the original filename, keep the directory
            fs::path output_path(output_file_);
            fs::path output_dir = output_path.parent_path();
            final_output = (output_dir / original_filename).string();
        }
        
        // Write extracted data
        auto write_result = utils::FileIO::write_file(final_output, actual_data);
        if (!write_result.success) {
            utils::Console::error(std::format("Failed to write output file: {}", write_result.error_message));
            return 1;
        }
        
        utils::Console::success(std::format(
            "Successfully extracted {} bytes",
            actual_data.size()
        ));
        utils::Console::info(std::format("Output: {}", final_output));
        
        if (verbose_) {
            // Show first few bytes as hex
            size_t preview_size = std::min<size_t>(16, actual_data.size());
            std::ostringstream hex_stream;
            for (size_t i = 0; i < preview_size; ++i) {
                hex_stream << std::hex << std::setw(2) << std::setfill('0') 
                          << static_cast<int>(actual_data[i]) << " ";
            }
            if (actual_data.size() > preview_size) {
                hex_stream << "...";
            }
            utils::Console::info(std::format("Data preview: {}", hex_stream.str()));
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(std::format("Extract failed: {}", e.what()));
        return 1;
    }
}

int StegoCommand::do_capacity() {
    try {
        size_t capacity = LSBSteganography::calculate_capacity(cover_image_, bits_per_channel_);
        
        if (capacity == 0) {
            utils::Console::error("Failed to calculate capacity");
            return 1;
        }
        
        utils::Console::success(std::format(
            "Image capacity: {} bytes ({:.2f} KB)",
            capacity,
            static_cast<double>(capacity) / 1024.0
        ));
        
        utils::Console::info(std::format("Bits per channel: {}", bits_per_channel_));
        
        // Show capacity for different bit levels
        utils::Console::info("\nCapacity at different bit levels:");
        for (int bits = 1; bits <= 4; ++bits) {
            size_t cap = LSBSteganography::calculate_capacity(cover_image_, bits);
            utils::Console::info(std::format("  {} bit(s): {} bytes ({:.2f} KB)",
                bits, cap, static_cast<double>(cap) / 1024.0));
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(std::format("Capacity calculation failed: {}", e.what()));
        return 1;
    }
}

} // namespace filevault::cli::commands
