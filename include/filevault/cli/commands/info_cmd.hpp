#ifndef FILEVAULT_CLI_COMMANDS_INFO_CMD_HPP
#define FILEVAULT_CLI_COMMANDS_INFO_CMD_HPP

#include "filevault/cli/command.hpp"
#include "filevault/core/crypto_engine.hpp"

namespace filevault {
namespace cli {

/**
 * @brief Info command - display encrypted file metadata
 * 
 * Shows information about encrypted files without decrypting:
 * - File size (original, encrypted, compression ratio)
 * - Encryption algorithm
 * - KDF type and parameters
 * - Compression type
 * - Timestamp (if available)
 */
class InfoCommand : public ICommand {
public:
    explicit InfoCommand(core::CryptoEngine& engine);
    
    std::string name() const override { return "info"; }
    std::string description() const override { 
        return "Display information about encrypted file"; 
    }
    
    void setup(CLI::App& app) override;
    int execute() override;

private:
    core::CryptoEngine& engine_;
    std::string input_file_;
    bool verbose_ = false;
    
    /**
     * @brief Parse encrypted file header
     */
    struct FileInfo {
        size_t file_size = 0;
        size_t salt_size = 0;
        size_t nonce_size = 0;
        size_t tag_size = 0;
        size_t data_size = 0;
        
        // TODO: Add when file format is enhanced
        // std::string algorithm;
        // std::string kdf_type;
        // std::string compression;
        // uint32_t version;
    };
    
    FileInfo parse_file(const std::string& path);
    void display_info(const FileInfo& info);
};

} // namespace cli
} // namespace filevault

#endif // FILEVAULT_CLI_COMMANDS_INFO_CMD_HPP
