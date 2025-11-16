#include "filevault/cli/commands/info_cmd.hpp"
#include "filevault/utils/console.hpp"
#include "filevault/utils/crypto_utils.hpp"
#include <fmt/core.h>
#include <fstream>

namespace filevault {
namespace cli {

InfoCommand::InfoCommand(core::CryptoEngine& engine)
    : engine_(engine) {
}

void InfoCommand::setup(CLI::App& app) {
    auto* cmd = app.add_subcommand(name(), description());
    
    cmd->add_option("input", input_file_, "Encrypted file to inspect")
        ->required()
        ->check(CLI::ExistingFile);
    
    cmd->add_flag("-v,--verbose", verbose_, "Show detailed information");
    
    cmd->callback([this]() { execute(); });
}

int InfoCommand::execute() {
    try {
        utils::Console::header("File Information");
        
        auto info = parse_file(input_file_);
        display_info(info);
        
        return 0;
        
    } catch (const std::exception& e) {
        utils::Console::error(fmt::format("Failed to read file info: {}", e.what()));
        return 1;
    }
}

InfoCommand::FileInfo InfoCommand::parse_file(const std::string& path) {
    FileInfo info;
    
    // Open file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }
    
    info.file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Current file format (basic):
    // [salt (32 bytes)][nonce (12 bytes)][ciphertext + tag]
    // TODO: Update when enhanced file format is implemented
    
    if (info.file_size < 44) {  // Minimum: salt + nonce
        throw std::runtime_error("File too small to be valid encrypted file");
    }
    
    // Read salt
    std::vector<uint8_t> salt(32);
    file.read(reinterpret_cast<char*>(salt.data()), 32);
    info.salt_size = 32;
    
    // Read nonce
    std::vector<uint8_t> nonce(12);
    file.read(reinterpret_cast<char*>(nonce.data()), 12);
    info.nonce_size = 12;
    
    // Remaining is ciphertext + tag
    info.data_size = info.file_size - info.salt_size - info.nonce_size;
    
    // GCM tag is typically 16 bytes, embedded in ciphertext
    info.tag_size = 16;
    
    return info;
}

void InfoCommand::display_info(const FileInfo& info) {
    fmt::print("\n");
    fmt::print("  📄 {:25} : {}\n", "File", input_file_);
    fmt::print("  📦 {:25} : {}\n", "Total Size", 
               utils::CryptoUtils::format_bytes(info.file_size));
    
    fmt::print("\n");
    utils::Console::separator();
    fmt::print("\n");
    
    fmt::print("  🔒 Encryption Details:\n");
    fmt::print("     {:25} : {} bytes\n", "Salt", info.salt_size);
    fmt::print("     {:25} : {} bytes\n", "Nonce", info.nonce_size);
    fmt::print("     {:25} : {} bytes\n", "Auth Tag", info.tag_size);
    fmt::print("     {:25} : {}\n", "Encrypted Data", 
               utils::CryptoUtils::format_bytes(info.data_size - info.tag_size));
    
    if (verbose_) {
        fmt::print("\n");
        fmt::print("  📊 Statistics:\n");
        double overhead = static_cast<double>(info.salt_size + info.nonce_size + info.tag_size);
        double overhead_pct = (overhead / info.file_size) * 100.0;
        fmt::print("     {:25} : {:.2f}%\n", "Metadata Overhead", overhead_pct);
    }
    
    fmt::print("\n");
    utils::Console::warning("Note: This is basic file format. Enhanced format with full metadata coming soon.");
    fmt::print("\n");
}

} // namespace cli
} // namespace filevault
